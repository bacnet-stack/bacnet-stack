/**
 * @file
 * @brief dimming brightness engine based on lighting commands
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2022
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "linear.h"
#include "debug.h"
/* me! */
#include "lighting_command.h"

/**
 * @brief call the lighting command tracking value callbacks
 * @param data - dimmer data
 * @param old_value - value prior to write
 * @param value - value of the write
 */
static void lighting_command_tracking_value_handler(
    struct bacnet_lighting_command_data *data, float old_value, float value)
{
    struct lighting_command_notification *head;

    head = &data->Notification_Head;
    do {
        if (head->callback) {
            head->callback(data->Key, old_value, value);
        }
        head = head->next;
    } while (head);
}

/**
 * @brief Add a Lighting Command notification callback
 * @param cb - notification callback to be added
 */
void lighting_command_notification_add(
    struct bacnet_lighting_command_data *data,
    struct lighting_command_notification *notification)
{
    struct lighting_command_notification *head;

    head = &data->Notification_Head;
    do {
        if (head->next == notification) {
            /* already here! */
            break;
        } else if (!head->next) {
            /* first available free node */
            head->next = notification;
            break;
        }
        head = head->next;
    } while (head);
}

float lighting_command_clamp_value(
    struct bacnet_lighting_command_data *data, float value)
{
    /* clamp value within trim values, if non-zero */
    if (isless(value, 1.0f)) {
        /* jump target to OFF if below normalized min */
        value = 0.0f;
    } else if (isgreater(value, data->High_Trim_Value)) {
        value = data->High_Trim_Value;
    } else if (isless(value, data->Low_Trim_Value)) {
        value = data->Low_Trim_Value;
    }

    return value;
}

/**
 * @brief Callback for tracking value updates
 * @param  data - dimmer data
 * @param  old_value - value prior to write
 * @param  value - value of the write
 */
static void lighting_command_tracking_value_notify(
    struct bacnet_lighting_command_data *data, float old_value, float value)
{
    if (data->Overridden) {
        value = lighting_command_clamp_value(data, data->Overridden_Value);
        lighting_command_tracking_value_handler(data, old_value, value);
    } else if (!data->Out_Of_Service) {
        value = lighting_command_clamp_value(data, value);
        lighting_command_tracking_value_handler(data, old_value, value);
    } else {
        debug_printf(
            "Lighting-Command[%lu]-Out-of-Service\n", (unsigned long)data->Key);
    }
}

/**
 * Handles the timing for a single Lighting Output object Fade
 *
 * @param data [in] dimmer data
 * @param milliseconds - number of milliseconds elapsed since previously
 * called.  Works best when called about every 10 milliseconds.
 */
static void lighting_command_fade_handler(
    struct bacnet_lighting_command_data *data, uint16_t milliseconds)
{
    float old_value;
    float x1, x2, x3, y1, y3;
    float target_value, min_value, max_value;

    old_value = data->Tracking_Value;
    min_value = data->Min_Actual_Value;
    max_value = data->Max_Actual_Value;
    target_value = data->Target_Level;
    /* clamp target within min/max */
    if (isgreater(target_value, max_value)) {
        target_value = max_value;
    }
    if (isless(target_value, min_value)) {
        target_value = min_value;
    }
    if ((milliseconds >= data->Fade_Time) ||
        (!islessgreater(data->Tracking_Value, target_value))) {
        /* stop fading */
        if (isless(data->Target_Level, 1.0f)) {
            /* jump target to OFF if below normalized min */
            data->Tracking_Value = 0.0f;
        } else {
            data->Tracking_Value = target_value;
        }
        data->In_Progress = BACNET_LIGHTING_IDLE;
        data->Lighting_Operation = BACNET_LIGHTS_STOP;
        data->Fade_Time = 0;
    } else {
        /* fading */
        x1 = 0.0f;
        x2 = (float)milliseconds;
        x3 = (float)data->Fade_Time;
        if (isless(old_value, min_value)) {
            y1 = min_value;
        } else {
            y1 = old_value;
        }
        y3 = target_value;
        data->Tracking_Value = linear_interpolate(x1, x2, x3, y1, y3);
        data->Fade_Time -= milliseconds;
        data->In_Progress = BACNET_LIGHTING_FADE_ACTIVE;
    }
    lighting_command_tracking_value_notify(
        data, old_value, data->Tracking_Value);
}

/**
 * Updates the object tracking value while ramping
 *
 * Commands the dimmer to ramp from the current Tracking_Value to the
 * target-level specified in the command. The ramp operation
 * changes the output from its current value to target-level,
 * at a particular percent per second defined by ramp-rate.
 * While the ramp operation is executing, In_Progress shall be set
 * to RAMP_ACTIVE, and Tracking_Value shall be updated to reflect the current
 * progress of the ramp. <target-level> shall be clamped to
 * Min_Actual_Value and Max_Actual_Value.
 *
 * @param data [in] dimmer data
 * @param milliseconds - number of milliseconds elapsed
 */
static void lighting_command_ramp_handler(
    struct bacnet_lighting_command_data *data, uint16_t milliseconds)
{
    float old_value, target_value, min_value, max_value, step_value, steps;

    old_value = data->Tracking_Value;
    min_value = data->Min_Actual_Value;
    max_value = data->Max_Actual_Value;
    target_value = data->Target_Level;
    /* clamp target within min/max, if needed */
    if (isgreater(target_value, max_value)) {
        target_value = max_value;
    }
    if (isless(target_value, min_value)) {
        target_value = min_value;
    }
    /* determine the number of steps */
    if (milliseconds <= 1000) {
        /* percent per second */
        steps = linear_interpolate(
            0.0f, (float)milliseconds, 1000.0f, 0.0f, data->Ramp_Rate);
    } else {
        steps = ((float)milliseconds * data->Ramp_Rate) / 1000.0f;
    }
    if (!islessgreater(data->Tracking_Value, target_value)) {
        /* stop ramping */
        if (isless(data->Target_Level, 1.0f)) {
            /* jump target to OFF if below normalized min */
            data->Tracking_Value = 0.0f;
        } else {
            data->Tracking_Value = target_value;
        }
        data->In_Progress = BACNET_LIGHTING_IDLE;
        data->Lighting_Operation = BACNET_LIGHTS_STOP;
    } else {
        if (isless(old_value, target_value)) {
            step_value = old_value + steps;
            if (isgreater(step_value, target_value)) {
                /* stop ramping */
                data->Lighting_Operation = BACNET_LIGHTS_STOP;
            }
        } else if (isgreater(old_value, target_value)) {
            if (isgreater(old_value, steps)) {
                step_value = old_value - steps;
            } else {
                step_value = target_value;
            }
            if (isless(step_value, target_value)) {
                /* stop ramping */
                data->Lighting_Operation = BACNET_LIGHTS_STOP;
            }
        } else {
            /* stop ramping */
            step_value = target_value;
            data->Lighting_Operation = BACNET_LIGHTS_STOP;
        }
        /* clamp target within min/max, if needed */
        if (isgreater(step_value, max_value)) {
            step_value = max_value;
        }
        if (isless(step_value, min_value)) {
            step_value = min_value;
        }
        if (data->Lighting_Operation == BACNET_LIGHTS_STOP) {
            if (isless(data->Target_Level, 1.0f)) {
                /* jump target to OFF if below normalized min */
                data->Tracking_Value = 0.0f;
            } else {
                data->Tracking_Value = step_value;
            }
            data->In_Progress = BACNET_LIGHTING_IDLE;
        } else {
            data->Tracking_Value = step_value;
            data->In_Progress = BACNET_LIGHTING_RAMP_ACTIVE;
        }
    }
    lighting_command_tracking_value_notify(
        data, old_value, data->Tracking_Value);
}

/**
 * Clamps the step increment value
 *
 * @param step_increment [in] step increment value
 * @return clamped step increment value
 */
static float lighting_command_step_increment_clamp(float step_increment)
{
    if (isless(step_increment, 0.1f)) {
        step_increment = 0.1f;
    }
    if (isgreater(step_increment, 100.0f)) {
        step_increment = 100.0f;
    }

    return step_increment;
}

/**
 * Updates the object tracking value while stepping
 *
 * Commands the dimmer to a value equal to the Tracking_Value
 * plus the step-increment. The resulting sum shall be clamped to
 * Min_Actual_Value and Max_Actual_Value
 *
 * @param data [in] dimmer data
 */
static void
lighting_command_step_up_handler(struct bacnet_lighting_command_data *data)
{
    float old_value, target_value, min_value, max_value, step_value;

    old_value = data->Tracking_Value;
    min_value = data->Min_Actual_Value;
    max_value = data->Max_Actual_Value;
    step_value = lighting_command_step_increment_clamp(data->Step_Increment);
    /* inhibit ON if the value is already OFF */
    if (isgreaterequal(old_value, min_value)) {
        target_value = old_value + step_value;
        /* clamp target within min/max, if needed */
        if (isgreater(target_value, max_value)) {
            target_value = max_value;
        }
        if (isless(target_value, min_value)) {
            target_value = min_value;
        }
        data->Tracking_Value = target_value;
        data->In_Progress = BACNET_LIGHTING_IDLE;
        data->Lighting_Operation = BACNET_LIGHTS_STOP;
        lighting_command_tracking_value_notify(
            data, old_value, data->Tracking_Value);
    }
}

/**
 * Normalize the value to the min/max range
 * @param value [in] value to normalize
 * @param min_value [in] minimum value
 * @param max_value [in] maximum value
 * @return normalized value
 */
static float
lighting_command_normalize_value(float value, float min_value, float max_value)
{
    float normalized_value;
    /* clamp target within min/max, if needed */
    if (isgreater(value, max_value)) {
        /* clamp target within max */
        normalized_value = max_value;
    } else if (isless(value, 1.0f)) {
        /* jump target to OFF if below normalized min */
        normalized_value = 0.0f;
    } else if (isless(value, min_value)) {
        /* clamp target within min */
        normalized_value = min_value;
    } else {
        normalized_value = value;
    }

    return normalized_value;
}

/**
 * Updates the object tracking value while stepping
 *
 * Commands the dimmer to a value equal to the Tracking_Value
 * plus the step-increment. The resulting sum shall be clamped to
 * Min_Actual_Value and Max_Actual_Value
 *
 * @param data [in] dimmer data
 */
static void
lighting_command_step_down_handler(struct bacnet_lighting_command_data *data)
{
    float old_value, target_value, min_value, max_value, step_value;

    old_value = target_value = data->Tracking_Value;
    min_value = data->Min_Actual_Value;
    max_value = data->Max_Actual_Value;
    step_value = lighting_command_step_increment_clamp(data->Step_Increment);
    if (isgreaterequal(target_value, step_value)) {
        target_value -= step_value;
    } else {
        target_value = 0.0f;
    }
    /* clamp target within min/max, if needed */
    if (isgreater(target_value, max_value)) {
        /* clamp target within max */
        target_value = max_value;
    }
    if (isless(target_value, min_value)) {
        /* clamp target within min */
        target_value = min_value;
    }
    data->Tracking_Value = target_value;
    data->In_Progress = BACNET_LIGHTING_IDLE;
    data->Lighting_Operation = BACNET_LIGHTS_STOP;
    lighting_command_tracking_value_notify(
        data, old_value, data->Tracking_Value);
}

/**
 * Updates the object tracking value while stepping
 *
 * Commands the dimmer to a value equal to the Tracking_Value
 * plus the step-increment. The resulting sum shall be clamped to
 * Min_Actual_Value and Max_Actual_Value
 *
 * @param data [in] dimmer data
 */
static void
lighting_command_step_on_handler(struct bacnet_lighting_command_data *data)
{
    float old_value, target_value, min_value, max_value, step_value;

    old_value = target_value = data->Tracking_Value;
    min_value = data->Min_Actual_Value;
    max_value = data->Max_Actual_Value;
    step_value = lighting_command_step_increment_clamp(data->Step_Increment);
    target_value += step_value;
    data->Tracking_Value =
        lighting_command_normalize_value(target_value, min_value, max_value);
    data->In_Progress = BACNET_LIGHTING_IDLE;
    data->Lighting_Operation = BACNET_LIGHTS_STOP;
    lighting_command_tracking_value_notify(
        data, old_value, data->Tracking_Value);
}

/**
 * Updates the object tracking value while stepping
 *
 * Commands the dimmer to a value equal to the Tracking_Value
 * plus the step-increment. The resulting sum shall be clamped to
 * Min_Actual_Value and Max_Actual_Value
 *
 * @param data [in] dimmer data
 */
static void
lighting_command_step_off_handler(struct bacnet_lighting_command_data *data)
{
    float old_value, target_value, min_value, max_value, step_value;

    old_value = target_value = data->Tracking_Value;
    min_value = data->Min_Actual_Value;
    max_value = data->Max_Actual_Value;
    step_value = lighting_command_step_increment_clamp(data->Step_Increment);
    if (isgreaterequal(target_value, step_value)) {
        target_value -= step_value;
    } else {
        target_value = 0.0f;
    }
    data->Tracking_Value =
        lighting_command_normalize_value(target_value, min_value, max_value);
    data->In_Progress = BACNET_LIGHTING_IDLE;
    data->Lighting_Operation = BACNET_LIGHTS_STOP;
    lighting_command_tracking_value_notify(
        data, old_value, data->Tracking_Value);
}

/**
 * Updates the object tracking value while blinking
 *
 * @note When the value of In_Progress is NOT_CONTROLLED or OTHER,
 *  the value of Tracking_Value shall be a local matter.
 *
 * @note The WARN, WARN_RELINQUISH, and WARN_OFF lighting
 * commands, as well as writing one of the special values to the
 * Present_Value property, cause a blink-warn notification
 * to occur at the specified priority. A blink-warn
 * notification is used to warn the occupants that the lights
 * are about to turn off, giving the occupants the opportunity
 * to exit the space or to override the lights for a period of time.
 *
 * The actual blink-warn notification mechanism shall be a local matter.
 * The physical lights may blink once, multiple times, or
 * repeatedly. They may also go bright, go dim, or signal a notification
 * through some other means. In some circumstances, no blink-warn
 * notification will occur at all. The blink-warn notification
 * shall not be reflected in the priority array or the tracking
 * value.
 *
 * @param data [in] dimmer data
 * @param milliseconds - number of milliseconds elapsed
 */
static void lighting_command_blink_handler(
    struct bacnet_lighting_command_data *data, uint16_t milliseconds)
{
    float old_value, target_value, min_value, max_value;

    old_value = data->Tracking_Value;
    min_value = data->Min_Actual_Value;
    max_value = data->Max_Actual_Value;
    /* detect 'end' operation */
    if (data->Blink.Duration > milliseconds) {
        data->Blink.Duration -= milliseconds;
    } else {
        data->Blink.Duration = 0;
    }
    if (data->Blink.Duration == 0) {
        /* 'end' operation */
        data->In_Progress = BACNET_LIGHTING_IDLE;
        data->Lighting_Operation = BACNET_LIGHTS_STOP;
        target_value = data->Blink.End_Value;
    } else if (data->Blink.Target_Interval == 0) {
        /* only 'on' level */
        target_value = data->Blink.On_Value;
    } else {
        /* 'blink' operation */
        if (data->Blink.State) {
            target_value = data->Blink.On_Value;
        } else {
            target_value = data->Blink.Off_Value;
        }
        /* detect next interval */
        if (data->Blink.Interval > milliseconds) {
            data->Blink.Interval -= milliseconds;
        } else {
            data->Blink.Interval = 0;
        }
        if (data->Blink.Interval == 0) {
            /* next blink */
            data->Blink.Interval = data->Blink.Target_Interval;
            data->Blink.State = !data->Blink.State;
            if (data->Blink.State) {
                /* end of 'off' operation when counting */
                if ((data->Blink.Count > 0) &&
                    (data->Blink.Count != UINT16_MAX)) {
                    data->Blink.Count--;
                }
                if (data->Blink.Count == 0) {
                    /* 'end' operation */
                    data->In_Progress = BACNET_LIGHTING_IDLE;
                    data->Lighting_Operation = BACNET_LIGHTS_STOP;
                    target_value = data->Blink.End_Value;
                }
            }
        }
    }
    target_value =
        lighting_command_normalize_value(target_value, min_value, max_value);
    /* note: The blink-warn notifications shall not be reflected
       in the tracking value. */
    if (data->In_Progress == BACNET_LIGHTING_IDLE) {
        data->Tracking_Value = target_value;
    }
    lighting_command_tracking_value_notify(data, old_value, target_value);
}

/**
 * @brief Overrides the current lighting command if overridden is true
 * @param data [in] dimmer data
 */
void lighting_command_override(struct bacnet_lighting_command_data *data)
{
    if (!data) {
        return;
    }
    if (data->Overridden) {
        lighting_command_tracking_value_notify(
            data, data->Tracking_Value, data->Overridden_Value);
    } else {
        lighting_command_tracking_value_notify(
            data, data->Tracking_Value, data->Tracking_Value);
    }
}

/**
 * @brief Updates the dimmer tracking value per ramp or fade or step
 * @param data [in] dimmer data
 * @param milliseconds - number of milliseconds elapsed since previously
 * called.  Suggest that this is called every 10 milliseconds.
 */
void lighting_command_timer(
    struct bacnet_lighting_command_data *data, uint16_t milliseconds)
{
    if (!data) {
        return;
    }
    switch (data->Lighting_Operation) {
        case BACNET_LIGHTS_NONE:
            data->In_Progress = BACNET_LIGHTING_IDLE;
            break;
        case BACNET_LIGHTS_FADE_TO:
            lighting_command_fade_handler(data, milliseconds);
            break;
        case BACNET_LIGHTS_RAMP_TO:
            lighting_command_ramp_handler(data, milliseconds);
            break;
        case BACNET_LIGHTS_STEP_UP:
            lighting_command_step_up_handler(data);
            break;
        case BACNET_LIGHTS_STEP_DOWN:
            lighting_command_step_down_handler(data);
            break;
        case BACNET_LIGHTS_STEP_ON:
            lighting_command_step_on_handler(data);
            break;
        case BACNET_LIGHTS_STEP_OFF:
            lighting_command_step_off_handler(data);
            break;
        case BACNET_LIGHTS_WARN:
        case BACNET_LIGHTS_WARN_OFF:
        case BACNET_LIGHTS_WARN_RELINQUISH:
            lighting_command_blink_handler(data, milliseconds);
            break;
        case BACNET_LIGHTS_STOP:
            data->In_Progress = BACNET_LIGHTING_IDLE;
            break;
        default:
            break;
    }
}

/**
 * @brief Set the lighting command if the priority is active
 * @param data [in] dimmer data
 * @param value [in] BACnet lighting value
 * @param fade_time [in] BACnet lighting fade time
 */
void lighting_command_fade_to(
    struct bacnet_lighting_command_data *data, float value, uint32_t fade_time)
{
    if (!data) {
        return;
    }
    data->Fade_Time = fade_time;
    data->Lighting_Operation = BACNET_LIGHTS_FADE_TO;
    data->Target_Level = value;
}

/**
 * @brief Set the dimmer command to perform a ramp to value operation
 * @param data [in] dimmer object instance
 * @param value [in] target lighting value
 * @param ramp_rate [in] target ramp rate in percent per second 0.1 to 100.0
 */
void lighting_command_ramp_to(
    struct bacnet_lighting_command_data *data, float value, float ramp_rate)
{
    if (!data) {
        return;
    }
    data->Ramp_Rate = ramp_rate;
    data->Lighting_Operation = BACNET_LIGHTS_RAMP_TO;
    data->Target_Level = value;
}

/**
 * @brief Set the lighting command if the priority is active
 * @param data [in] dimmer object instance
 * @param operation [in] BACnet lighting operation
 * @param step_increment [in] BACnet lighting step increment value
 */
void lighting_command_step(
    struct bacnet_lighting_command_data *data,
    BACNET_LIGHTING_OPERATION operation,
    float step_increment)
{
    if (!data) {
        return;
    }
    data->Lighting_Operation = operation;
    data->Fade_Time = 0;
    data->Ramp_Rate = 0.0f;
    data->Step_Increment = step_increment;
}

/**
 * @brief Set the lighting command to blink mode
 * @param data [in] dimmer object instance
 * @param operation [in] BACnet lighting operation for blink warn
 * @param blink [in] BACnet blink data
 */
void lighting_command_blink_warn(
    struct bacnet_lighting_command_data *data,
    BACNET_LIGHTING_OPERATION operation,
    struct bacnet_lighting_command_warn_data *blink)
{
    if (!data) {
        return;
    }
    data->Lighting_Operation = operation;
    data->Blink.Target_Interval = blink->Interval;
    data->Blink.Duration = blink->Duration;
    data->Blink.Count = blink->Count;
    data->Blink.On_Value = blink->On_Value;
    data->Blink.Off_Value = blink->Off_Value;
    data->Blink.End_Value = blink->End_Value;
    /* start blinking */
    data->In_Progress = BACNET_LIGHTING_OTHER;
    /* configure next interval */
    data->Blink.State = false;
    data->Blink.Interval = blink->Interval;
}

/**
 * @brief Set the lighting command if the priority is active
 * @param object [in] BACnet object instance
 * @param priority [in] BACnet priority array value 1..16
 */
void lighting_command_stop(struct bacnet_lighting_command_data *data)
{
    if (!data) {
        return;
    }
    data->Lighting_Operation = BACNET_LIGHTS_STOP;
}

/**
 * @brief Set the lighting command if the priority is active
 * @param object [in] BACnet object instance
 * @param priority [in] BACnet priority array value 1..16
 */
void lighting_command_none(struct bacnet_lighting_command_data *data)
{
    if (!data) {
        return;
    }
    data->Lighting_Operation = BACNET_LIGHTS_NONE;
}

void lighting_command_init(struct bacnet_lighting_command_data *data)
{
    if (!data) {
        return;
    }
    data->Tracking_Value = 0.0f;
    data->Lighting_Operation = BACNET_LIGHTS_NONE;
    data->In_Progress = BACNET_LIGHTING_IDLE;
    data->Out_Of_Service = false;
    data->Min_Actual_Value = 1.0f;
    data->Max_Actual_Value = 100.0f;
    data->Low_Trim_Value = 1.0f;
    data->High_Trim_Value = 100.0f;
    data->Overridden = false;
    data->Overridden_Value = 0.0f;
    data->Blink.On_Value = 100.0f;
    data->Blink.Off_Value = 0.0f;
    data->Blink.End_Value = 0.0f;
    data->Blink.Target_Interval = 0;
    data->Blink.Count = 0;
    data->Blink.Interval = 0;
    data->Blink.Duration = 0;
    data->Blink.State = false;
    data->Notification_Head.next = NULL;
    data->Notification_Head.callback = NULL;
}
