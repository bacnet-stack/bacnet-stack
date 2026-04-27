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
 * @brief compare two floating point values to 3 decimal places
 *
 * @param x1 - first comparison value
 * @param x2 - second comparison value
 * @return true if the value is the same to 3 decimal points
 */
static bool is_float_equal(float x1, float x2)
{
    return fabs(x1 - x2) < 0.001;
}

/**
 * @brief Clamp the value within the normalized range
 * @details The normalized output level is specified as the
 *  linearized percentage (0..100%) of the possible light output range
 *  with 0.0% being off, 1.0% being dimmest, and 100.0% being brightest.
 * @param value [in] value to clamp within the normalized range
 * @return value clamped within the normalized range of 0.0% to 100.0%
 */
float lighting_command_normalized_range_clamp(float value)
{
    float physical_value;

    /* clamp value within physical values, if non-zero */
    if (isless(value, 1.0f)) {
        /* jump target to OFF */
        physical_value = 0.0f;
    } else if (isgreater(value, 100.0f)) {
        physical_value = 100.0f;
    } else {
        physical_value = value;
    }

    return physical_value;
}

/**
 * @brief Clamp the value within the normal ON range
 * @details The normal ON output level is specified as the linearized
 *  percentage (1..100%) of the possible light output range with 1.0% being
 *  dimmest, and 100.0% being brightest.
 * @param value [in] value to clamp within the normalized ON range
 * @return value clamped within the normalized ON range of 1.0% to 100.0%
 */
float lighting_command_normalized_on_range_clamp(float value)
{
    float normalized_on_value;

    /* clamp value within 1.0 and 100.0 values */
    if (isless(value, 1.0f)) {
        normalized_on_value = 1.0f;
    } else if (isgreater(value, 100.0f)) {
        normalized_on_value = 100.0f;
    } else {
        normalized_on_value = value;
    }

    return normalized_on_value;
}

/**
 * @brief Adjust Min/Max Actual Value Range
 * @details Min_Actual_Value property, of type Real, shall specify
 * the physical output level that corresponds to a Present_Value of 1.0%.
 * Changing Min_Actual_Value to a value greater than Max_Actual_Value
 * shall force Max_Actual_Value to become equal to Min_Actual_Value.
 * Min_Actual_Value shall always be a positive number in the
 * range 1.0% to 100.0%.
 *
 * Max_Actual_Value property, of type Real, shall specify the physical
 * output level that corresponds to a Present_Value of 100.0%.
 * Changing Max_Actual_Value to a value less than Min_Actual_Value
 * shall force Min_Actual_Value to become equal to Max_Actual_Value.
 * Max_Actual_Value shall always be a positive number in the range
 * 1.0% to 100.0%.
 */
static void lighting_command_min_max_value_range_adjust(
    struct bacnet_lighting_command_data *data)
{
    float swap_value, min_value, max_value;

    min_value =
        lighting_command_normalized_on_range_clamp(data->Min_Actual_Value);
    max_value =
        lighting_command_normalized_on_range_clamp(data->Max_Actual_Value);
    if (isgreater(min_value, max_value)) {
        /* swap the configured high and low actual values if they are inverse */
        swap_value = min_value;
        min_value = max_value;
        max_value = swap_value;
    }
    data->Min_Actual_Value = min_value;
    data->Max_Actual_Value = max_value;
}

/**
 * @brief Calculate the Feedback_Value property value
 *
 * This property, of type Real, shall indicate the actual value
 * of the physical lighting output within the normalized range.
 * If the actual value of the physical lighting output in the
 * non-normalized range is not off but is less than the
 * Min_Actual_Value, then Feedback_Value shall be set to 1.0%.
 * If the actual value in the non-normalized range is greater than
 * Max_Actual_Value, then Feedback_Value shall be set to 100.0%.
 * @param data - dimmer data structure
 * @return calculated feedback-value
 */
float lighting_command_normalized_to_physical_value(
    float min_value, float max_value, float normalized_value)
{
    float physical_value, x1, x2, x3, y1, y3;

    if (isless(normalized_value, 1.0f)) {
        physical_value = 0.0f;
    } else if (isgreater(normalized_value, 100.0f)) {
        physical_value = max_value;
    } else {
        x1 = 1.0f;
        x2 = normalized_value;
        x3 = 100.0f;
        y1 = min_value;
        y3 = max_value;
        physical_value = linear_interpolate(x1, x2, x3, y1, y3);
    }

    return physical_value;
}

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

    if (!data || !notification) {
        return;
    }
    lighting_command_lock(data);

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
    lighting_command_unlock(data);
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

    if (!data || !notification) {
        return;
    }
    lighting_command_lock(data);

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
    lighting_command_unlock(data);
}

/**
 * @brief call the lighting command blink stop callback
 *        for WARN_OFF/WARN_RELINQUISH operations
 * @param data - dimmer data structure
 */
static void lighting_command_blink_stop_notify_nolock(
    struct bacnet_lighting_command_data *data)
{
    if (data->Blink.Callback) {
        /* do some checking to avoid extra callbacks */
        switch (data->Lighting_Operation) {
            case BACNET_LIGHTS_WARN_OFF:
            case BACNET_LIGHTS_WARN_RELINQUISH:
                if (data->Blink.Priority != 0) {
                    data->Blink.Callback(
                        data->Key, data->Lighting_Operation,
                        data->Blink.Priority);
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
 * @brief Calculate the target value for a low or high trim fade operation
 * @param data - dimmer data structure
 * @param value - target value for the trim fade operation
 * @param trim_value - target trim value for the fade operation
 * @param milliseconds - number of milliseconds elapsed since the last call
 * @return calculated target value for the trim fade operation
 */
static float lighting_command_trim_fade(
    struct bacnet_lighting_command_data *data,
    float value,
    float trim_value,
    uint32_t milliseconds)
{
    float new_value;
    float x1, x2, x3, y1, y3;

    if (data) {
        if (milliseconds > 0) {
            /* fading */
            if (milliseconds >= data->Trim_Fade_Time) {
                /* end of fading */
                data->Trim_Fade_Time = 0;
                new_value = trim_value;
            } else {
                x1 = 0.0f;
                x2 = (float)milliseconds;
                x3 = (float)data->Trim_Fade_Time;
                y1 = value;
                y3 = trim_value;
                new_value = linear_interpolate(x1, x2, x3, y1, y3);
                data->Trim_Fade_Time -= milliseconds;
            }
        } else {
            /* no fading */
            data->Trim_Fade_Time = 0;
            new_value = trim_value;
        }
    } else {
        /* no fading */
        new_value = value;
    }

    return new_value;
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
 * @param milliseconds - number of milliseconds elapsed since the last call
 * @return value clamped within the operating range defined by the High_End_Trim
 *  and Low_End_Trim property values
 */
static float lighting_command_operating_range_clamp_fade_nolock(
    struct bacnet_lighting_command_data *data,
    float value,
    uint16_t milliseconds)
{
    if (data) {
        /* clamp value within trim values, if non-zero */
        if (isless(value, 1.0f)) {
            /* jump target to OFF if below normalized min */
            value = 0.0f;
        } else if (isgreater(value, data->High_Trim_Value)) {
            value = lighting_command_trim_fade(
                data, value, data->High_Trim_Value, milliseconds);
            data->In_Progress = BACNET_LIGHTING_TRIM_ACTIVE;
        } else if (isless(value, data->Low_Trim_Value)) {
            value = lighting_command_trim_fade(
                data, value, data->Low_Trim_Value, milliseconds);
            data->In_Progress = BACNET_LIGHTING_TRIM_ACTIVE;
        }
    } else {
        /* no data, so just clamp value within physical limits */
        value = lighting_command_normalized_range_clamp(value);
    }

    return value;
}

float lighting_command_operating_range_clamp_fade(
    struct bacnet_lighting_command_data *data,
    float value,
    uint16_t milliseconds)
{
    float clamped_value;

    if (!data) {
        return value;
    }
    lighting_command_lock(data);
    clamped_value = lighting_command_operating_range_clamp_fade_nolock(
        data, value, milliseconds);
    lighting_command_unlock(data);

    return clamped_value;
}

/**
 * @brief Callback for tracking value updates
 * @param data - dimmer data structure
 * @param  old_value - value prior to write
 * @param  value - value of the write
 * @param milliseconds - number of milliseconds elapsed
 */
static void lighting_command_tracking_value_event(
    struct bacnet_lighting_command_data *data,
    float old_value,
    float tracking_value)
{
    float physical_value, old_physical_value;

    lighting_command_min_max_value_range_adjust(data);
    physical_value = lighting_command_normalized_to_physical_value(
        data->Min_Actual_Value, data->Max_Actual_Value, tracking_value);
    old_physical_value = lighting_command_normalized_to_physical_value(
        data->Min_Actual_Value, data->Max_Actual_Value, old_value);
    if (data->Overridden) {
        lighting_command_tracking_value_notify(
            data, old_physical_value, physical_value);
        if (data->Overridden_Momentary) {
            data->Overridden = false;
        }
    } else if (!data->Out_Of_Service) {
        data->Overridden_Momentary = false;
        lighting_command_tracking_value_notify(
            data, old_physical_value, physical_value);
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
    if (isless(old_value, 1.0f) && isless(data->Target_Level, 1.0f)) {
        /* check for OFF to OFF transition */
        data->Tracking_Value = 0.0f;
        data->In_Progress = BACNET_LIGHTING_IDLE;
        data->Lighting_Operation = BACNET_LIGHTS_STOP;
    } else {
        /* clamp Target value within the Normalized ON Range */
        target_value =
            lighting_command_normalized_on_range_clamp(data->Target_Level);
        if ((milliseconds >= data->Fade_Time) ||
            (is_float_equal(data->Tracking_Value, target_value))) {
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
            /* fading in the normalized ON range */
            x1 = 0.0f;
            x2 = (float)milliseconds;
            x3 = (float)data->Fade_Time;
            if (isless(old_value, 1.0f)) {
                y1 = 1.0f;
            } else {
                y1 = old_value;
            }
            y3 = target_value;
            data->Tracking_Value = linear_interpolate(x1, x2, x3, y1, y3);
            data->Fade_Time -= milliseconds;
            data->In_Progress = BACNET_LIGHTING_FADE_ACTIVE;
        }
        /* clamp Tracking Value inclusively within the Operating Range */
        data->Tracking_Value =
            lighting_command_operating_range_clamp_fade_nolock(
                data, data->Tracking_Value, milliseconds);
    }
    /* notify */
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
 * progress of the ramp.
 *
 * @param data - dimmer data structure
 * @param milliseconds - number of milliseconds elapsed
 */
static void lighting_command_ramp_handler(
    struct bacnet_lighting_command_data *data, uint16_t milliseconds)
{
    float old_value, target_value, step_value, steps, ramp_rate,
        operating_value;

    old_value = data->Tracking_Value;
    if (isless(old_value, 1.0f) && isless(data->Target_Level, 1.0f)) {
        /* check for OFF to OFF transition */
        data->Tracking_Value = 0.0f;
        data->In_Progress = BACNET_LIGHTING_IDLE;
        data->Lighting_Operation = BACNET_LIGHTS_STOP;
    } else {
        /* clamp Target value within the Normalized ON Range */
        target_value =
            lighting_command_normalized_on_range_clamp(data->Target_Level);
        if (is_float_equal(data->Tracking_Value, target_value)) {
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
            step_value = lighting_command_normalized_on_range_clamp(step_value);
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
        /* clamp Tracking_Value inclusively within the Operating Range */
        operating_value = lighting_command_operating_range_clamp_fade_nolock(
            data, data->Tracking_Value, milliseconds);
        data->Tracking_Value = operating_value;
    }
    /* notify */
    lighting_command_tracking_value_event(
        data, old_value, data->Tracking_Value);
}

/**
 * Updates the object tracking value while stepping
 *
 * Commands the dimmer to a value equal to the Tracking_Value
 * plus the step-increment. If the result of the addition is
 * greater than 100.0%, the value shall be set to 100.0%.
 *
 * @param data [in] dimmer data
 */
static void
lighting_command_step_up_handler(struct bacnet_lighting_command_data *data)
{
    float old_value, target_value;

    old_value = data->Tracking_Value;
    if (isgreaterequal(old_value, 1.0f)) {
        /* inhibit ON if the value is currently OFF */
        target_value = lighting_command_step_up_target_value(
            data->Tracking_Value, data->Step_Increment);
        /* clamp Tracking value inclusively within the Operating Range */
        data->Tracking_Value =
            lighting_command_normalized_on_range_clamp(target_value);
        data->In_Progress = BACNET_LIGHTING_IDLE;
        data->Lighting_Operation = BACNET_LIGHTS_STOP;
        /* notify */
        lighting_command_tracking_value_event(
            data, old_value, data->Tracking_Value);
    }
}

/**
 * Updates the object tracking value while stepping
 *
 * Commands the dimmer to a value equal to the Tracking_Value
 * plus the step-increment.
 *
 * @param data [in] dimmer data
 */
static void
lighting_command_step_down_handler(struct bacnet_lighting_command_data *data)
{
    float old_value, target_value, operating_value;

    old_value = data->Tracking_Value;
    target_value = lighting_command_step_down_target_value(
        data->Tracking_Value, data->Step_Increment);
    data->Tracking_Value =
        lighting_command_normalized_on_range_clamp(target_value);
    data->In_Progress = BACNET_LIGHTING_IDLE;
    data->Lighting_Operation = BACNET_LIGHTS_STOP;
    /* clamp Tracking value inclusively within the Operating Range */
    operating_value =
        lighting_command_normalized_range_clamp(data->Tracking_Value);
    data->Tracking_Value = operating_value;
    /* notify */
    lighting_command_tracking_value_event(
        data, old_value, data->Tracking_Value);
}

/**
 * Updates the object tracking value while stepping
 *
 * Commands the dimmer to a value equal to the Tracking_Value
 * plus the step-increment.
 * If the result of the addition is greater than 100.0%,
 * the value shall be set to 100.0%.
 * When the Tracking_Value is 0.0%, 1.0% is written
 * to the specified slot in the priority array.
 *
 * @param data [in] dimmer data
 */
static void
lighting_command_step_on_handler(struct bacnet_lighting_command_data *data)
{
    float old_value, target_value;

    old_value = data->Tracking_Value;
    if (isless(data->Tracking_Value, 1.0f)) {
        /* step is ignored when starting at OFF */
        data->Tracking_Value = 1.0f;
    } else {
        target_value = lighting_command_step_up_target_value(
            data->Tracking_Value, data->Step_Increment);
        /* clamp Tracking value inclusively within the Normalized Range */
        data->Tracking_Value =
            lighting_command_normalized_range_clamp(target_value);
    }
    data->In_Progress = BACNET_LIGHTING_IDLE;
    data->Lighting_Operation = BACNET_LIGHTS_STOP;
    /* notify */
    lighting_command_tracking_value_event(
        data, old_value, data->Tracking_Value);
}

/**
 * Updates the object tracking value while stepping
 *
 * Commands Present_Value to a value equal to the Tracking_Value
 * minus the step-increment at the specified priority.
 * The step-down operation is implemented by writing
 * the Tracking_Value minus step-increment to the specified
 * slot in the priority array.
 * If the result of the subtraction is less than 1.0%,
 * 0.0% is written to the specified slot in the priority array.
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
        lighting_command_normalized_range_clamp(target_value);
    data->In_Progress = BACNET_LIGHTING_IDLE;
    data->Lighting_Operation = BACNET_LIGHTS_STOP;
    /* notify */
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
    float old_value, target_value, operating_value;

    old_value = data->Tracking_Value;
    /* detect 'end' operation */
    if (data->Blink.Duration > milliseconds) {
        data->Blink.Duration -= milliseconds;
    } else {
        data->Blink.Duration = 0;
    }
    if (data->Blink.Duration == 0) {
        /* 'end' operation */
        lighting_command_blink_stop_notify_nolock(data);
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
                    lighting_command_blink_stop_notify_nolock(data);
                    data->In_Progress = BACNET_LIGHTING_IDLE;
                    data->Lighting_Operation = BACNET_LIGHTS_STOP;
                    target_value = data->Blink.End_Value;
                }
            }
        }
    }
    /* clamp Tracking value inclusively within the Operating Range */
    operating_value = lighting_command_normalized_range_clamp(target_value);
    /* note: The blink-warn notifications shall not be reflected
       in the tracking value. */
    if (data->In_Progress == BACNET_LIGHTING_IDLE) {
        data->Tracking_Value = operating_value;
    }
    lighting_command_tracking_value_event(data, old_value, operating_value);
}

/**
 * @brief Overrides the current lighting command with the provided value
 * @param data [in] dimmer data
 */
static void lighting_command_override_nolock(
    struct bacnet_lighting_command_data *data, float value)
{
    float old_value;

    old_value = data->Tracking_Value;
    data->Tracking_Value = lighting_command_normalized_range_clamp(value);
    lighting_command_tracking_value_event(
        data, old_value, data->Tracking_Value);
}

/**
 * @brief Overrides the current lighting command with the provided value
 * @param data [in] dimmer data
 */
void lighting_command_override(
    struct bacnet_lighting_command_data *data, float value)
{
    if (!data) {
        return;
    }
    lighting_command_lock(data);
    lighting_command_override_nolock(data, value);
    lighting_command_unlock(data);
}

/**
 * @brief Overrides the current lighting command with the provided value
 * @param data [in] dimmer data
 */
void lighting_command_override_set(
    struct bacnet_lighting_command_data *data, float value)
{
    if (!data) {
        return;
    }
    lighting_command_lock(data);
    data->Overridden = true;
    data->Overridden_Momentary = false;
    lighting_command_override_nolock(data, value);
    lighting_command_unlock(data);
}

/**
 * @brief Clears the override of the current lighting command
 * @param data [in] dimmer data
 */
void lighting_command_override_clear(
    struct bacnet_lighting_command_data *data, float value)
{
    float old_value;

    if (!data) {
        return;
    }
    lighting_command_lock(data);
    data->Overridden = false;
    data->Overridden_Momentary = false;
    old_value = data->Tracking_Value;
    /* clamp Tracking value within the Normalized Range */
    data->Tracking_Value = lighting_command_normalized_range_clamp(value);
    lighting_command_tracking_value_event(
        data, old_value, data->Tracking_Value);
    lighting_command_unlock(data);
}

/**
 * @brief Overrides the current lighting command with the provided value
 * @param data [in] dimmer data
 */
void lighting_command_override_momentary(
    struct bacnet_lighting_command_data *data, float value)
{
    if (!data) {
        return;
    }
    lighting_command_lock(data);
    data->Overridden = true;
    data->Overridden_Momentary = true;
    lighting_command_override_nolock(data, value);
    lighting_command_unlock(data);
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
    lighting_command_lock(data);
    value = data->Tracking_Value;
    lighting_command_tracking_value_event(data, value, value);
    lighting_command_unlock(data);
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
    lighting_command_lock(data);
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
    lighting_command_unlock(data);
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
    lighting_command_lock(data);
    /* possibly interrupting a blink warn, so notify */
    lighting_command_blink_stop_notify_nolock(data);
    /* configure the lighting operation */
    data->Fade_Time = fade_time;
    data->Lighting_Operation = BACNET_LIGHTS_FADE_TO;
    data->Target_Level = value;
    if (isgreaterequal(value, 1.0)) {
        /* the last value that was greater than or equal to 1.0%.*/
        data->Last_On_Value = value;
    }
    lighting_command_unlock(data);
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
    lighting_command_lock(data);
    /* possibly interrupting a blink warn, so notify */
    lighting_command_blink_stop_notify_nolock(data);
    /* configure the lighting operation */
    data->Ramp_Rate = lighting_command_ramp_rate_clamp(ramp_rate);
    data->Lighting_Operation = BACNET_LIGHTS_RAMP_TO;
    data->Target_Level = value;
    if (isgreaterequal(value, 1.0)) {
        /* the last value that was greater than or equal to 1.0%.*/
        data->Last_On_Value = value;
    }
    lighting_command_unlock(data);
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
    lighting_command_lock(data);
    /* possibly interrupting a blink warn, so notify */
    lighting_command_blink_stop_notify_nolock(data);
    /* configure the lighting operation */
    if (((operation == BACNET_LIGHTS_STEP_UP) ||
         (operation == BACNET_LIGHTS_STEP_DOWN)) &&
        (is_float_equal(data->Tracking_Value, 0.0))) {
        /* If the starting level of Tracking_Value is 0.0%,
        then this operation is ignored. */
        goto done;
    }
    data->Lighting_Operation = operation;
    data->Fade_Time = 0;
    data->Step_Increment = step_increment;
    /* determine the last-on-value for the given step operation */
    if (operation == BACNET_LIGHTS_STEP_UP) {
        target_value = lighting_command_step_up_target_value(
            data->Tracking_Value, data->Step_Increment);
        target_value = lighting_command_normalized_on_range_clamp(target_value);
    } else if (operation == BACNET_LIGHTS_STEP_DOWN) {
        target_value = lighting_command_step_down_target_value(
            data->Tracking_Value, data->Step_Increment);
        target_value = lighting_command_normalized_on_range_clamp(target_value);
    } else if (operation == BACNET_LIGHTS_STEP_ON) {
        target_value = lighting_command_step_up_target_value(
            data->Tracking_Value, data->Step_Increment);
        target_value = lighting_command_normalized_range_clamp(target_value);
    } else if (operation == BACNET_LIGHTS_STEP_OFF) {
        target_value = lighting_command_step_down_target_value(
            data->Tracking_Value, data->Step_Increment);
        target_value = lighting_command_normalized_range_clamp(target_value);
    } else {
        goto done;
    }
    if (isgreaterequal(target_value, 1.0)) {
        /* the last value that was greater than or equal to 1.0%.*/
        data->Last_On_Value = target_value;
    }

done:
    lighting_command_unlock(data);
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
    if (!data || !blink) {
        return;
    }
    lighting_command_lock(data);
    /* possibly interrupting a blink warn, so notify */
    lighting_command_blink_stop_notify_nolock(data);
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
    lighting_command_unlock(data);
}

/**
 * @brief Copy the current blink data from the lighting command
 * @param data [in] dimmer object instance
 * @param blink [out] BACnet blink data to copy into
 */
void lighting_command_blink_copy(
    struct bacnet_lighting_command_data *data,
    struct bacnet_lighting_command_warn_data *blink)
{
    if (!data || !blink) {
        return;
    }
    lighting_command_lock(data);
    blink->On_Value = data->Blink.On_Value;
    blink->Off_Value = data->Blink.Off_Value;
    blink->End_Value = data->Blink.End_Value;
    blink->Priority = data->Blink.Priority;
    blink->Callback = data->Blink.Callback;
    blink->Target_Interval = data->Blink.Target_Interval;
    blink->Interval = data->Blink.Interval;
    blink->Duration = data->Blink.Duration;
    blink->Count = data->Blink.Count;
    blink->State = data->Blink.State;
    lighting_command_unlock(data);
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
    lighting_command_lock(data);
    /* possibly interrupting a blink warn, so notify */
    lighting_command_blink_stop_notify_nolock(data);
    /* configure the lighting operation */
    data->Lighting_Operation = BACNET_LIGHTS_STOP;
    if (isgreaterequal(data->Tracking_Value, 1.0)) {
        /* the last value that was greater than or equal to 1.0%.*/
        data->Last_On_Value = data->Tracking_Value;
    }
    lighting_command_unlock(data);
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
    lighting_command_lock(data);
    /* possibly interrupting a blink warn, so notify */
    lighting_command_blink_stop_notify_nolock(data);
    /* configure the lighting operation */
    data->Lighting_Operation = BACNET_LIGHTS_NONE;
    lighting_command_unlock(data);
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
    lighting_command_lock(data);
    /* possibly interrupting a blink warn, so notify */
    lighting_command_blink_stop_notify_nolock(data);
    /* configure the lighting operation */
    data->Fade_Time = fade_time;
    data->Lighting_Operation = BACNET_LIGHTS_RESTORE_ON;
    data->Target_Level = data->Last_On_Value;
    lighting_command_unlock(data);
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
    lighting_command_lock(data);
    /* possibly interrupting a blink warn, so notify */
    lighting_command_blink_stop_notify_nolock(data);
    /* configure the lighting operation */
    data->Fade_Time = fade_time;
    data->Lighting_Operation = BACNET_LIGHTS_DEFAULT_ON;
    data->Target_Level = data->Default_On_Value;
    lighting_command_unlock(data);
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
    lighting_command_lock(data);
    /* possibly interrupting a blink warn, so notify */
    lighting_command_blink_stop_notify_nolock(data);
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
    lighting_command_unlock(data);
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
    lighting_command_lock(data);
    /* possibly interrupting a blink warn, so notify */
    lighting_command_blink_stop_notify_nolock(data);
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
    lighting_command_unlock(data);
}

/**
 * @brief Configure the lighting command to apply low or high trim
 * @param data [in] dimmer data
 * @param high_end_trim [in] BACnet lighting high end trim
 * @param low_end_trim [in] BACnet lighting low end trim
 * @param trim_fade_time [in] BACnet lighting trim fade time
 */
void lighting_command_trim_set(
    struct bacnet_lighting_command_data *data,
    float high_end_trim,
    float low_end_trim,
    uint32_t trim_fade_time)
{
    float swap_value, high_trim, low_trim;

    if (!data) {
        return;
    }
    lighting_command_lock(data);
    /* clamp range within normalized limits */
    high_trim = lighting_command_normalized_on_range_clamp(high_end_trim);
    low_trim = lighting_command_normalized_on_range_clamp(low_end_trim);
    /* valid range check for high and low trim values */
    if (isgreater(low_trim, high_trim)) {
        /* swap the trims if they are inverse */
        swap_value = low_trim;
        low_trim = high_trim;
        high_trim = swap_value;
    }
    data->High_Trim_Value = high_trim;
    data->Low_Trim_Value = low_trim;
    data->Trim_Fade_Time = trim_fade_time;
    lighting_command_unlock(data);
}

/**
 * @brief Set the lighting command key
 * @param data [in] dimmer data
 * @param key [in] BACnet lighting key
 */
void lighting_command_key_set(
    struct bacnet_lighting_command_data *data, uint32_t key)
{
    if (!data) {
        return;
    }
    lighting_command_lock(data);
    data->Key = key;
    lighting_command_unlock(data);
}

/**
 * @brief Set the lighting command tracking value callback
 * @param data [in] dimmer data
 * @param cb [in] BACnet lighting tracking value callback
 */
void lighting_command_tracking_value_callback_set(
    struct bacnet_lighting_command_data *data,
    lighting_command_tracking_value_callback cb)
{
    if (!data) {
        return;
    }
    lighting_command_lock(data);
    data->Notification_Head.callback = cb;
    lighting_command_unlock(data);
}

/**
 * @brief Get the lighting command in progress
 * @param data [in] dimmer data
 * @return BACNET_LIGHTING_IN_PROGRESS - lighting command in progress
 */
BACNET_LIGHTING_IN_PROGRESS
lighting_command_in_progress_get(struct bacnet_lighting_command_data *data)
{
    BACNET_LIGHTING_IN_PROGRESS in_progress = BACNET_LIGHTING_IDLE;

    if (!data) {
        return in_progress;
    }
    lighting_command_lock(data);
    in_progress = data->In_Progress;
    lighting_command_unlock(data);

    return in_progress;
}

/**
 * @brief Set the lighting command in progress
 * @param data [in] dimmer data
 * @param in_progress [in] BACnet lighting in progress
 */
void lighting_command_in_progress_set(
    struct bacnet_lighting_command_data *data,
    BACNET_LIGHTING_IN_PROGRESS in_progress)
{
    if (!data) {
        return;
    }
    lighting_command_lock(data);
    data->In_Progress = in_progress;
    lighting_command_unlock(data);
}

/**
 * @brief Get the lighting command tracking value
 * @param data [in] dimmer data
 * @return float - lighting command tracking value
 */
float lighting_command_tracking_value_get(
    struct bacnet_lighting_command_data *data)
{
    float value = 0.0f;

    if (!data) {
        return value;
    }
    lighting_command_lock(data);
    value = data->Tracking_Value;
    lighting_command_unlock(data);

    return value;
}

/**
 * @brief Set the lighting command tracking value
 * @param data [in] dimmer data
 * @param value [in] BACnet lighting tracking value
 */
void lighting_command_tracking_value_set(
    struct bacnet_lighting_command_data *data, float value)
{
    if (!data) {
        return;
    }
    lighting_command_lock(data);
    data->Tracking_Value = value;
    lighting_command_unlock(data);
}

void lighting_command_blink_warn_feature_set(
    struct bacnet_lighting_command_data *data,
    float off_value,
    uint16_t interval,
    uint16_t count)
{
    if (!data) {
        return;
    }
    lighting_command_lock(data);
    data->Blink.Off_Value = off_value;
    data->Blink.Interval = interval;
    data->Blink.Count = count;
    lighting_command_unlock(data);
}

/**
 * @brief Check if the lighting command blink egress is active
 * @param data [in] dimmer data
 * @return bool - true if the lighting command blink egress is active
 */
bool lighting_command_blink_egress_active(
    struct bacnet_lighting_command_data *data)
{
    bool active = false;

    if (!data) {
        return active;
    }
    lighting_command_lock(data);
    active = data->Blink.Duration > 0;
    lighting_command_unlock(data);

    return active;
}

/**
 * @brief Get the lighting command out of service
 * @param data [in] dimmer data
 * @return bool - true if the lighting command is out of service
 */
bool lighting_command_out_of_service_get(
    struct bacnet_lighting_command_data *data)
{
    bool value = false;

    if (!data) {
        return value;
    }
    lighting_command_lock(data);
    value = data->Out_Of_Service;
    lighting_command_unlock(data);

    return value;
}

/**
 * @brief Set the lighting command out of service
 * @param data [in] dimmer data
 * @param value [in] BACnet lighting out of service
 */
void lighting_command_out_of_service_set(
    struct bacnet_lighting_command_data *data, bool value)
{
    if (!data) {
        return;
    }
    lighting_command_lock(data);
    data->Out_Of_Service = value;
    lighting_command_unlock(data);
}

/**
 * @brief Get the lighting command last on value
 * @param data [in] dimmer data
 * @return float - lighting command last on value
 */
float lighting_command_last_on_value_get(
    struct bacnet_lighting_command_data *data)
{
    float value = 0.0f;

    if (!data) {
        return value;
    }
    lighting_command_lock(data);
    value = data->Last_On_Value;
    lighting_command_unlock(data);

    return value;
}

/**
 * @brief Set the lighting command last on value
 * @param data [in] dimmer data
 * @param value [in] BACnet lighting last on value
 */
void lighting_command_last_on_value_set(
    struct bacnet_lighting_command_data *data, float value)
{
    if (!data) {
        return;
    }
    lighting_command_lock(data);
    data->Last_On_Value = value;
    lighting_command_unlock(data);
}

/**
 * @brief Get the lighting command default on value
 * @param data [in] dimmer data
 * @return float - lighting command default on value
 */
float lighting_command_default_on_value_get(
    struct bacnet_lighting_command_data *data)
{
    float value = 0.0f;

    if (!data) {
        return value;
    }
    lighting_command_lock(data);
    value = data->Default_On_Value;
    lighting_command_unlock(data);

    return value;
}

/**
 * @brief Set the lighting command default on value
 * @param data [in] dimmer data
 * @param value [in] BACnet lighting default on value
 */
void lighting_command_default_on_value_set(
    struct bacnet_lighting_command_data *data, float value)
{
    if (!data) {
        return;
    }
    lighting_command_lock(data);
    data->Default_On_Value = value;
    lighting_command_unlock(data);
}

/**
 * @brief Get the lighting command overridden status
 * @param data [in] dimmer data
 * @return bool - true if the lighting command is overridden
 */
bool lighting_command_overridden_status(
    struct bacnet_lighting_command_data *data)
{
    bool status = false;

    if (!data) {
        return status;
    }
    lighting_command_lock(data);
    status = data->Overridden || data->Overridden_Momentary;
    lighting_command_unlock(data);

    return status;
}

/**
 * @brief Get the lighting command feedback value
 * @param data [in] dimmer data
 * @return float - lighting command feedback value
 */
float lighting_command_feedback_value(struct bacnet_lighting_command_data *data)
{
    float feedback_value;

    if (!data) {
        return 0.0f;
    }
    lighting_command_lock(data);
    lighting_command_min_max_value_range_adjust(data);
    feedback_value = lighting_command_normalized_to_physical_value(
        data->Min_Actual_Value, data->Max_Actual_Value, data->Tracking_Value);
    lighting_command_unlock(data);

    return feedback_value;
}

/**
 * @brief Determine if fade, ramp, or warn command is currently executing
 * @param pObject [in] object to apply the trim values to
 * @param priority [in] priority of the command
 */
bool lighting_command_active(struct bacnet_lighting_command_data *data)
{
    bool in_progress = false;

    if (!data) {
        return in_progress;
    }
    lighting_command_lock(data);
    if ((data->In_Progress == BACNET_LIGHTING_FADE_ACTIVE) ||
        (data->In_Progress == BACNET_LIGHTING_RAMP_ACTIVE) ||
        (data->Blink.Duration > 0)) {
        in_progress = true;
    }
    lighting_command_unlock(data);

    return in_progress;
}

/**
 * @brief Locks the lighting command for exclusive access
 * @param data [in] dimmer data
 */
void lighting_command_lock(struct bacnet_lighting_command_data *data)
{
    if (!data) {
        return;
    }
    if (data->Lock) {
        data->Lock(data);
    }
}

/**
 * @brief Unlocks the lighting command for exclusive access
 * @param data [in] dimmer data
 */
void lighting_command_unlock(struct bacnet_lighting_command_data *data)
{
    if (!data) {
        return;
    }
    if (data->Unlock) {
        data->Unlock(data);
    }
}

/**
 * @brief Initializes the lighting command data structure to default values
 */
void lighting_command_init(struct bacnet_lighting_command_data *data)
{
    if (!data) {
        return;
    }
    lighting_command_lock(data);
    data->Tracking_Value = 0.0f;
    data->Lighting_Operation = BACNET_LIGHTS_NONE;
    data->In_Progress = BACNET_LIGHTING_NOT_CONTROLLED;
    data->Out_Of_Service = false;
    data->Min_Actual_Value = 1.0f;
    data->Max_Actual_Value = 100.0f;
    data->Low_Trim_Value = 1.0f;
    data->High_Trim_Value = 100.0f;
    data->Trim_Fade_Time = 0;
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
    data->Blink.State = false;
    data->Blink.Callback = NULL;
    data->Blink.State = false;
    data->Notification_Head.next = NULL;
    data->Notification_Head.callback = NULL;
    data->Timer_Notification_Head.next = NULL;
    data->Timer_Notification_Head.callback = NULL;
    lighting_command_unlock(data);
}
