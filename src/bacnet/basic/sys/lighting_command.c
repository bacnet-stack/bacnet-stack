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
 * @param data - dimmer data structure
 * @param old_value - value prior to write
 * @param value - value of the write
 */
static void lighting_command_tracking_value_notify(
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
 * @param data - dimmer data structure
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

/**
 * @brief call the lighting command tracking value callbacks
 * @param data - dimmer data structure
 * @param milliseconds - number of elapsed milliseconds since the last call
 */
static void lighting_command_timer_notify(
    struct bacnet_lighting_command_data *data, uint16_t milliseconds)
{
    struct lighting_command_timer_notification *head;

    head = &data->Timer_Notification_Head;
    do {
        if (head->callback) {
            head->callback(data, milliseconds);
        }
        head = head->next;
    } while (head);
}

/**
 * @brief Add a Lighting Command notification callback
 * @param data - dimmer data structure
 * @param cb - notification callback to be added
 */
void lighting_command_timer_notfication_add(
    struct bacnet_lighting_command_data *data,
    struct lighting_command_timer_notification *notification)
{
    struct lighting_command_timer_notification *head;

    head = &data->Timer_Notification_Head;
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

/**
 * @brief call the lighting command tracking value callbacks
 * @param data - dimmer data structure
 */
static void
lighting_command_blink_notify(struct bacnet_lighting_command_data *data)
{
    if (data->Blink.Callback) {
        /* do some checking to avoid extra callbacks */
        switch (data->Lighting_Operation) {
            case BACNET_LIGHTS_WARN_OFF:
            case BACNET_LIGHTS_WARN_RELINQUISH:
                if (data->Blink.Priority != 0) {
                    data->Blink.Callback(data);
                }
                break;
            default:
                break;
        }
    }
}

/**
 * @brief Clamps the ramp rate value
 * @param ramp_rate [in] ramp rate value
 * @return clamped ramp rate value
 */
float lighting_command_ramp_rate_clamp(float ramp_rate)
{
    if (isless(ramp_rate, 0.1f)) {
        ramp_rate = 0.1f;
    }
    if (isgreater(ramp_rate, 100.0f)) {
        ramp_rate = 100.0f;
    }

    return ramp_rate;
}

/**
 * @brief Clamps the step increment value
 * @param step_increment [in] step increment value
 * @return clamped step increment value
 */
float lighting_command_step_increment_clamp(float step_increment)
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
 * @brief Calculate the target value for a step down command
 * @param tracking_value [in] current tracking value
 * @param step_increment [in] step increment value
 * @return target value for step down command
 */
float lighting_command_step_down_target_value(
    float tracking_value, float step_increment)
{
    float target_value, step_value;

    step_value = lighting_command_step_increment_clamp(step_increment);
    if (isgreaterequal(tracking_value, step_value)) {
        target_value = tracking_value - step_value;
    } else {
        target_value = 0.0f;
    }

    return target_value;
}

/**
 * @brief Calculate the target value for a step up command
 * @param tracking_value [in] current tracking value
 * @param step_increment [in] step increment value
 * @return target value for step up command
 */
float lighting_command_step_up_target_value(
    float tracking_value, float step_increment)
{
    float target_value, step_value;

    step_value = lighting_command_step_increment_clamp(step_increment);
    target_value = tracking_value + step_value;

    return target_value;
}

/**
 * @brief Clamp the value within the operating range between low and high
 *  end trim values.
 * @details The Operating Range is a subset of the Normalized Range,
 *  that represents the range of acceptable values for control of the object.
 *  The Operating Range is defined by the High_End_Trim and Low_End_Trim
 *  property values. When values are written outside of the Operating Range,
 *  the Tracking_Value will reflect the actual, clamped normalized light
 *  output while the Present_Value will reflect the original target value.
 * @param data - dimmer data structure
 * @param value the value that will be subject to clamping
 */
float lighting_command_operating_range_clamp(
    struct bacnet_lighting_command_data *data, float value)
{
    /* clamp value within trim values, if non-zero */
    if (isless(value, 1.0f)) {
        /* jump target to OFF if below normalized min */
        value = 0.0f;
    } else if (isgreater(value, data->High_Trim_Value)) {
        value = data->High_Trim_Value;
        data->In_Progress = BACNET_LIGHTING_TRIM_ACTIVE;
    } else if (isless(value, data->Low_Trim_Value)) {
        value = data->Low_Trim_Value;
        data->In_Progress = BACNET_LIGHTING_TRIM_ACTIVE;
    }

    return value;
}

/**
 * @brief Clamp the value within the normalized ON range 1% to 100%.
 * @details The physical output level, or non-normalized range,
 *  is specified as the linearized percentage (0..100%)
 *  of the possible light output range with 0.0% being off,
 *  1.0% being dimmest, and 100.0% being brightest.
 *  The actual range represents the subset of physical output levels
 *  defined by Min_Actual_Value and Max_Actual_Value
 *  (or 1.0 to 100.0% if these properties are not present).
 *  The normalized range is always 0.0 to 100.0% where
 *  1.0% = bottom of the actual range and 100.0% = top of the actual range.
 * @param data - dimmer data structure
 * @param value [in] value to normalize
 * @return normalized value within the range defined by Min_Actual_Value
 *  and Max_Actual_Value
 */
float lighting_command_normalized_on_range_clamp(
    struct bacnet_lighting_command_data *data, float value)
{
    /* clamp value within trim values, if non-zero */
    if (isgreater(value, data->Max_Actual_Value)) {
        value = data->Max_Actual_Value;
    } else if (isless(value, data->Min_Actual_Value)) {
        value = data->Min_Actual_Value;
    }

    return value;
}

/**
 * @brief Normalize the value to the min/max range
 * @details The physical output level, or non-normalized range,
 *  is specified as the linearized percentage (0..100%)
 *  of the possible light output range with 0.0% being off,
 *  1.0% being dimmest, and 100.0% being brightest.
 *  The actual range represents the subset of physical output levels
 *  defined by Min_Actual_Value and Max_Actual_Value
 *  (or 1.0 to 100.0% if these properties are not present).
 *  The normalized range is always 0.0 to 100.0% where
 *  1.0% = bottom of the actual range and 100.0% = top of the actual range.
 * @param data - dimmer data structure
 * @param value [in] value to normalize
 * @return normalized value within the range defined by
 *  0.0%, Min_Actual_Value, and Max_Actual_Value
 */
float lighting_command_normalized_range_clamp(
    struct bacnet_lighting_command_data *data, float value)
{
    float normalized_value;

    /* clamp value within normalized values, if non-zero */
    if (isless(value, 1.0f)) {
        /* jump target to OFF if below normalized min */
        normalized_value = 0.0f;
    } else if (isgreater(value, data->Max_Actual_Value)) {
        normalized_value = data->Max_Actual_Value;
    } else if (isless(value, data->Min_Actual_Value)) {
        normalized_value = data->Min_Actual_Value;
    } else {
        normalized_value = value;
    }

    return normalized_value;
}

/**
 * @brief Callback for tracking value updates
 * @param data - dimmer data structure
 * @param  old_value - value prior to write
 * @param  value - value of the write
 */
static void lighting_command_tracking_value_event(
    struct bacnet_lighting_command_data *data, float old_value, float value)
{
    if (data->Overridden) {
        value = lighting_command_operating_range_clamp(data, value);
        lighting_command_tracking_value_notify(data, old_value, value);
        if (data->Overridden_Momentary) {
            data->Overridden = false;
        }
    } else if (!data->Out_Of_Service) {
        data->Overridden_Momentary = false;
        value = lighting_command_operating_range_clamp(data, value);
        lighting_command_tracking_value_notify(data, old_value, value);
    } else {
        debug_printf(
            "Lighting-Command[%lu]-Out-of-Service\n", (unsigned long)data->Key);
    }
}

/**
 * Handles the timing for a single Lighting Output object Fade
 *
 * @param data - dimmer data structure
 * @param milliseconds - number of milliseconds elapsed since previously
 * called.  Works best when called about every 10 milliseconds.
 */
static void lighting_command_fade_handler(
    struct bacnet_lighting_command_data *data, uint16_t milliseconds)
{
    float old_value;
    float x1, x2, x3, y1, y3;
    float target_value;

    old_value = data->Tracking_Value;
    target_value =
        lighting_command_normalized_on_range_clamp(data, data->Target_Level);
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
        if (isless(old_value, data->Min_Actual_Value)) {
            y1 = data->Min_Actual_Value;
        } else {
            y1 = old_value;
        }
        y3 = target_value;
        data->Tracking_Value = linear_interpolate(x1, x2, x3, y1, y3);
        data->Fade_Time -= milliseconds;
        data->In_Progress = BACNET_LIGHTING_FADE_ACTIVE;
    }
    lighting_command_tracking_value_event(
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
 * @param data - dimmer data structure
 * @param milliseconds - number of milliseconds elapsed
 */
static void lighting_command_ramp_handler(
    struct bacnet_lighting_command_data *data, uint16_t milliseconds)
{
    float old_value, target_value, step_value, steps, ramp_rate;

    old_value = data->Tracking_Value;
    target_value =
        lighting_command_normalized_on_range_clamp(data, data->Target_Level);
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
        ramp_rate = lighting_command_ramp_rate_clamp(data->Ramp_Rate);
        /* determine the number of steps */
        if (milliseconds <= 1000) {
            /* percent per second */
            steps = linear_interpolate(
                0.0f, (float)milliseconds, 1000.0f, 0.0f, ramp_rate);
        } else {
            steps = ((float)milliseconds * ramp_rate) / 1000.0f;
        }
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
        step_value =
            lighting_command_normalized_on_range_clamp(data, step_value);
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
    lighting_command_tracking_value_event(
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
lighting_command_step_up_handler(struct bacnet_lighting_command_data *data)
{
    float old_value, target_value;

    old_value = data->Tracking_Value;
    if (isgreaterequal(old_value, data->Min_Actual_Value)) {
        /* inhibit ON if the value is already OFF */
        target_value = lighting_command_step_up_target_value(
            data->Tracking_Value, data->Step_Increment);
        data->Tracking_Value =
            lighting_command_normalized_on_range_clamp(data, target_value);
        data->In_Progress = BACNET_LIGHTING_IDLE;
        data->Lighting_Operation = BACNET_LIGHTS_STOP;
        lighting_command_tracking_value_event(
            data, old_value, data->Tracking_Value);
    }
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
    float old_value, target_value;

    old_value = data->Tracking_Value;
    target_value = lighting_command_step_down_target_value(
        data->Tracking_Value, data->Step_Increment);
    data->Tracking_Value =
        lighting_command_normalized_on_range_clamp(data, target_value);
    data->In_Progress = BACNET_LIGHTING_IDLE;
    data->Lighting_Operation = BACNET_LIGHTS_STOP;
    lighting_command_tracking_value_event(
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
    float old_value, target_value;

    old_value = data->Tracking_Value;
    target_value = lighting_command_step_up_target_value(
        data->Tracking_Value, data->Step_Increment);
    data->Tracking_Value =
        lighting_command_normalized_range_clamp(data, target_value);
    data->In_Progress = BACNET_LIGHTING_IDLE;
    data->Lighting_Operation = BACNET_LIGHTS_STOP;
    lighting_command_tracking_value_event(
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
    float old_value, target_value;

    old_value = data->Tracking_Value;
    target_value = lighting_command_step_down_target_value(
        data->Tracking_Value, data->Step_Increment);
    data->Tracking_Value =
        lighting_command_normalized_range_clamp(data, target_value);
    data->In_Progress = BACNET_LIGHTING_IDLE;
    data->Lighting_Operation = BACNET_LIGHTS_STOP;
    lighting_command_tracking_value_event(
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
 * to occur. A blink-warn notification is used to warn the
 * occupants that the lights are about to turn off, giving
 * the occupants the opportunity to exit the space or to
 * override the lights for a period of time.
 *
 * The actual blink-warn notification mechanism shall be a local matter.
 * The physical lights may blink once, multiple times, or
 * repeatedly. They may also go bright, go dim, or signal a notification
 * through some other means. In some circumstances, no blink-warn
 * notification will occur at all. The blink-warn notification
 * shall not be reflected in the tracking value.
 *
 * @param data [in] dimmer data
 * @param milliseconds - number of milliseconds elapsed
 */
static void lighting_command_blink_handler(
    struct bacnet_lighting_command_data *data, uint16_t milliseconds)
{
    float old_value, target_value;

    old_value = data->Tracking_Value;
    /* detect 'end' operation */
    if (data->Blink.Duration > milliseconds) {
        data->Blink.Duration -= milliseconds;
    } else {
        data->Blink.Duration = 0;
    }
    if (data->Blink.Duration == 0) {
        /* 'end' operation */
        lighting_command_blink_notify(data);
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
                    lighting_command_blink_notify(data);
                    data->In_Progress = BACNET_LIGHTING_IDLE;
                    data->Lighting_Operation = BACNET_LIGHTS_STOP;
                    target_value = data->Blink.End_Value;
                }
            }
        }
    }
    target_value = lighting_command_normalized_range_clamp(data, target_value);
    /* note: The blink-warn notifications shall not be reflected
       in the tracking value. */
    if (data->In_Progress == BACNET_LIGHTING_IDLE) {
        data->Tracking_Value = target_value;
    }
    lighting_command_tracking_value_event(data, old_value, target_value);
}

/**
 * @brief Overrides the current lighting command if overridden is true
 * @param data [in] dimmer data
 */
void lighting_command_override(
    struct bacnet_lighting_command_data *data, float value)
{
    float old_value;

    if (!data) {
        return;
    }
    old_value = data->Tracking_Value;
    data->Tracking_Value = value;
    lighting_command_tracking_value_event(data, old_value, value);
}

/**
 * @brief Refreshes the current lighting command at the tracking value
 * @param data [in] dimmer data
 */
void lighting_command_refresh(struct bacnet_lighting_command_data *data)
{
    float value;

    if (!data) {
        return;
    }
    value = data->Tracking_Value;
    lighting_command_tracking_value_event(data, value, value);
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
    if (data->Overridden) {
        data->Lighting_Operation = BACNET_LIGHTS_NONE;
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
        case BACNET_LIGHTS_RESTORE_ON:
        case BACNET_LIGHTS_DEFAULT_ON:
        case BACNET_LIGHTS_TOGGLE_RESTORE:
        case BACNET_LIGHTS_TOGGLE_DEFAULT:
            lighting_command_fade_handler(data, milliseconds);
            break;
        default:
            break;
    }
    lighting_command_timer_notify(data, milliseconds);
}

/**
 * @brief Set the lighting command to perform a fade to value operation
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
    /* cancel any warn in progress */
    lighting_command_blink_notify(data);
    /* configure the lighting operation */
    data->Fade_Time = fade_time;
    data->Lighting_Operation = BACNET_LIGHTS_FADE_TO;
    data->Target_Level = value;
    if (isgreaterequal(value, 1.0)) {
        /* the last value that was greater than or equal to 1.0%.*/
        data->Last_On_Value = value;
    }
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
    /* cancel any warn in progress */
    lighting_command_blink_notify(data);
    /* configure the lighting operation */
    data->Ramp_Rate = lighting_command_ramp_rate_clamp(ramp_rate);
    data->Lighting_Operation = BACNET_LIGHTS_RAMP_TO;
    data->Target_Level = value;
    if (isgreaterequal(value, 1.0)) {
        /* the last value that was greater than or equal to 1.0%.*/
        data->Last_On_Value = value;
    }
}

/**
 * @brief Set the lighting command to perform a step increment operation
 * @param data [in] dimmer object instance
 * @param operation [in] BACnet lighting operation
 * @param step_increment [in] BACnet lighting step increment value
 */
void lighting_command_step(
    struct bacnet_lighting_command_data *data,
    BACNET_LIGHTING_OPERATION operation,
    float step_increment)
{
    float target_value;

    if (!data) {
        return;
    }
    /* cancel any warn in progress */
    lighting_command_blink_notify(data);
    /* configure the lighting operation */
    if (((operation == BACNET_LIGHTS_STEP_UP) ||
         (operation == BACNET_LIGHTS_STEP_DOWN)) &&
        (!islessgreater(data->Tracking_Value, 0.0))) {
        /* If the starting level of Tracking_Value is 0.0%,
        then this operation is ignored. */
        return;
    }
    data->Lighting_Operation = operation;
    data->Fade_Time = 0;
    data->Step_Increment = step_increment;
    /* determine the last-on-value for the given step operation */
    if (operation == BACNET_LIGHTS_STEP_UP) {
        target_value = lighting_command_step_up_target_value(
            data->Tracking_Value, data->Step_Increment);
        target_value =
            lighting_command_normalized_on_range_clamp(data, target_value);
    } else if (operation == BACNET_LIGHTS_STEP_DOWN) {
        target_value = lighting_command_step_down_target_value(
            data->Tracking_Value, data->Step_Increment);
        target_value =
            lighting_command_normalized_on_range_clamp(data, target_value);
    } else if (operation == BACNET_LIGHTS_STEP_ON) {
        target_value = lighting_command_step_up_target_value(
            data->Tracking_Value, data->Step_Increment);
        target_value =
            lighting_command_normalized_range_clamp(data, target_value);
    } else if (operation == BACNET_LIGHTS_STEP_OFF) {
        target_value = lighting_command_step_down_target_value(
            data->Tracking_Value, data->Step_Increment);
        target_value =
            lighting_command_normalized_range_clamp(data, target_value);
    } else {
        return;
    }
    if (isgreaterequal(target_value, 1.0)) {
        /* the last value that was greater than or equal to 1.0%.*/
        data->Last_On_Value = target_value;
    }
}

/**
 * @brief Set the lighting command to perform a blink operation
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
    /* cancel any warn in progress */
    lighting_command_blink_notify(data);
    /* configure the new warning */
    data->Lighting_Operation = operation;
    data->Blink.Target_Interval = blink->Interval;
    data->Blink.Duration = blink->Duration;
    data->Blink.Priority = blink->Priority;
    data->Blink.Callback = blink->Callback;
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
 * @brief Set the lighting command to perform a stop operation
 * @param object [in] BACnet object instance
 * @param priority [in] BACnet priority array value 1..16
 */
void lighting_command_stop(struct bacnet_lighting_command_data *data)
{
    if (!data) {
        return;
    }
    /* cancel any warn in progress */
    lighting_command_blink_notify(data);
    /* configure the lighting operation */
    data->Lighting_Operation = BACNET_LIGHTS_STOP;
    if (isgreaterequal(data->Tracking_Value, 1.0)) {
        /* the last value that was greater than or equal to 1.0%.*/
        data->Last_On_Value = data->Tracking_Value;
    }
}

/**
 * @brief Set the lighting command to perform no operations
 * @param object [in] BACnet object instance
 * @param priority [in] BACnet priority array value 1..16
 */
void lighting_command_none(struct bacnet_lighting_command_data *data)
{
    if (!data) {
        return;
    }
    /* cancel any warn in progress */
    lighting_command_blink_notify(data);
    /* configure the lighting operation */
    data->Lighting_Operation = BACNET_LIGHTS_NONE;
}

/**
 * @brief Set the lighting command to perform a restore-on operation
 * @param data [in] dimmer data
 * @param fade_time [in] BACnet lighting fade time
 */
void lighting_command_restore_on(
    struct bacnet_lighting_command_data *data, uint32_t fade_time)
{
    if (!data) {
        return;
    }
    /* cancel any warn in progress */
    lighting_command_blink_notify(data);
    /* configure the lighting operation */
    data->Fade_Time = fade_time;
    data->Lighting_Operation = BACNET_LIGHTS_RESTORE_ON;
    data->Target_Level = data->Last_On_Value;
}

/**
 * @brief Set the lighting command to perform a default-on operation
 * @param data [in] dimmer data
 * @param fade_time [in] BACnet lighting fade time
 */
void lighting_command_default_on(
    struct bacnet_lighting_command_data *data, uint32_t fade_time)
{
    if (!data) {
        return;
    }
    /* cancel any warn in progress */
    lighting_command_blink_notify(data);
    /* configure the lighting operation */
    data->Fade_Time = fade_time;
    data->Lighting_Operation = BACNET_LIGHTS_DEFAULT_ON;
    data->Target_Level = data->Default_On_Value;
}

/**
 * @brief Set the lighting command to perform a toggle restore operation
 * @param data [in] dimmer data
 * @param fade_time [in] BACnet lighting fade time
 */
void lighting_command_toggle_restore(
    struct bacnet_lighting_command_data *data, uint32_t fade_time)
{
    if (!data) {
        return;
    }
    /* cancel any warn in progress */
    lighting_command_blink_notify(data);
    /* configure the lighting operation */
    data->Fade_Time = fade_time;
    data->Lighting_Operation = BACNET_LIGHTS_TOGGLE_RESTORE;
    if (isless(data->Tracking_Value, 1.0f)) {
        /* OFF: write the Last_On_Value */
        data->Target_Level = data->Last_On_Value;
    } else {
        /* not OFF, write 0.0% */
        data->Target_Level = 0.0f;
    }
}

/**
 * @brief Set the lighting command to perform a toggle default operation
 * @param data [in] dimmer data
 * @param fade_time [in] BACnet lighting fade time
 */
void lighting_command_toggle_default(
    struct bacnet_lighting_command_data *data, uint32_t fade_time)
{
    if (!data) {
        return;
    }
    /* cancel any warn in progress */
    lighting_command_blink_notify(data);
    /* configure the lighting operation */
    data->Fade_Time = fade_time;
    data->Lighting_Operation = BACNET_LIGHTS_TOGGLE_DEFAULT;
    if (isless(data->Tracking_Value, 1.0f)) {
        /* OFF: write the Default_On_Value */
        data->Target_Level = data->Default_On_Value;
    } else {
        /* not OFF, write 0.0% */
        data->Target_Level = 0.0f;
    }
}

void lighting_command_init(struct bacnet_lighting_command_data *data)
{
    if (!data) {
        return;
    }
    data->Tracking_Value = 0.0f;
    data->Lighting_Operation = BACNET_LIGHTS_NONE;
    data->In_Progress = BACNET_LIGHTING_NOT_CONTROLLED;
    data->Out_Of_Service = false;
    data->Min_Actual_Value = 1.0f;
    data->Max_Actual_Value = 100.0f;
    data->Low_Trim_Value = 1.0f;
    data->High_Trim_Value = 100.0f;
    data->Last_On_Value = 100.0f;
    data->Default_On_Value = 100.0f;
    data->Overridden = false;
    data->Blink.On_Value = 100.0f;
    data->Blink.Off_Value = 0.0f;
    data->Blink.End_Value = 0.0f;
    data->Blink.Target_Interval = 0;
    data->Blink.Count = 0;
    data->Blink.Interval = 0;
    data->Blink.Duration = 0;
    data->Blink.Priority = 0;
    data->Blink.State = false;
    data->Notification_Head.next = NULL;
    data->Notification_Head.callback = NULL;
}
