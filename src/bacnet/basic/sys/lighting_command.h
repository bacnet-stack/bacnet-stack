/**
 * @file
 * @brief API for lighting command brightness control engine
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SYS_LIGHTING_COMMAND_H
#define BACNET_BASIC_SYS_LIGHTING_COMMAND_H
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/**
 * @brief Callback for lighting command notifications
 * @param  key - key used to link to specific light
 * @param  old_value - physical value prior to write
 * @param  value - physical value of the write
 */
typedef void (*lighting_command_tracking_value_callback)(
    uint32_t key, float old_value, float value);
struct lighting_command_notification;
struct lighting_command_notification {
    struct lighting_command_notification *next;
    lighting_command_tracking_value_callback callback;
};

/* forward prototype of the structure defined later */
struct bacnet_lighting_command_data;

/**
 * @brief Callback for non-standard Lighting_Operation timer values
 * @param  data - Lighting Command data structure
 * @param  milliseconds - elapsed time in milliseconds
 */
typedef void (*lighting_command_timer_callback)(
    struct bacnet_lighting_command_data *data, uint16_t milliseconds);
struct lighting_command_timer_notification {
    struct lighting_command_timer_notification *next;
    lighting_command_timer_callback callback;
};

/**
 * @brief Callback for locking shared state during lighting command processing
 * @param data - dimmer data structure
 */
typedef void (*lighting_command_lock_callback)(
    struct bacnet_lighting_command_data *);

/**
 * @brief Callback that manipulates the value at the specified priority slot
    after a delay of Egress_Time seconds.
 * @param object_instance object-instance number of the object
 * @param operation BACnet lighting operation
 * @param priority BACnet priority array value 1..16
 */
typedef void (*lighting_command_blink_callback)(
    uint32_t key, BACNET_LIGHTING_OPERATION operation, uint8_t priority);

typedef struct bacnet_lighting_command_warn_data {
    /* warn */
    float On_Value;
    float Off_Value;
    float End_Value;
    uint8_t Priority;
    lighting_command_blink_callback Callback;
    uint16_t Target_Interval;
    /* internal tracking */
    uint16_t Interval;
    uint32_t Duration;
    uint16_t Count;
    /* bits */
    bool State : 1;
} BACNET_LIGHTING_COMMAND_WARN_DATA;

typedef struct bacnet_lighting_command_data {
    float Tracking_Value;
    BACNET_LIGHTING_OPERATION Lighting_Operation;
    float Target_Level;
    float Ramp_Rate;
    float Step_Increment;
    uint32_t Fade_Time;
    BACNET_LIGHTING_IN_PROGRESS In_Progress;
    float Min_Actual_Value;
    float Max_Actual_Value;
    float High_Trim_Value;
    float Low_Trim_Value;
    uint32_t Trim_Fade_Time;
    float Default_On_Value;
    float Last_On_Value;
    BACNET_LIGHTING_COMMAND_WARN_DATA Blink;
    /* bits - in common area of structure */
    bool Out_Of_Service : 1;
    bool Overridden : 1;
    bool Overridden_Momentary : 1;
    /* key used with callback */
    uint32_t Key;
    struct lighting_command_notification Notification_Head;
    struct lighting_command_timer_notification Timer_Notification_Head;
    /* lock for accessing shared state */
    lighting_command_lock_callback Lock;
    lighting_command_lock_callback Unlock;
    void *Context;
} BACNET_LIGHTING_COMMAND_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void lighting_command_timer(
    struct bacnet_lighting_command_data *data, uint16_t milliseconds);
BACNET_STACK_EXPORT
void lighting_command_fade_to(
    struct bacnet_lighting_command_data *data, float value, uint32_t fade_time);
BACNET_STACK_EXPORT
void lighting_command_ramp_to(
    struct bacnet_lighting_command_data *data, float value, float ramp_rate);
BACNET_STACK_EXPORT
void lighting_command_step(
    struct bacnet_lighting_command_data *data,
    BACNET_LIGHTING_OPERATION operation,
    float step_increment);
BACNET_STACK_EXPORT
void lighting_command_blink_warn(
    struct bacnet_lighting_command_data *data,
    BACNET_LIGHTING_OPERATION operation,
    struct bacnet_lighting_command_warn_data *blink);
BACNET_STACK_EXPORT
void lighting_command_blink_copy(
    struct bacnet_lighting_command_data *data,
    struct bacnet_lighting_command_warn_data *blink);
BACNET_STACK_EXPORT
void lighting_command_stop(struct bacnet_lighting_command_data *data);
BACNET_STACK_EXPORT
void lighting_command_none(struct bacnet_lighting_command_data *data);
BACNET_STACK_EXPORT
void lighting_command_restore_on(
    struct bacnet_lighting_command_data *data, uint32_t fade_time);
BACNET_STACK_EXPORT
void lighting_command_default_on(
    struct bacnet_lighting_command_data *data, uint32_t fade_time);
BACNET_STACK_EXPORT
void lighting_command_toggle_restore(
    struct bacnet_lighting_command_data *data, uint32_t fade_time);
BACNET_STACK_EXPORT
void lighting_command_toggle_default(
    struct bacnet_lighting_command_data *data, uint32_t fade_time);

BACNET_STACK_EXPORT
void lighting_command_override(
    struct bacnet_lighting_command_data *data, float value);
BACNET_STACK_EXPORT
void lighting_command_override_set(
    struct bacnet_lighting_command_data *data, float value);
BACNET_STACK_EXPORT
void lighting_command_override_clear(
    struct bacnet_lighting_command_data *data, float value);
BACNET_STACK_EXPORT
void lighting_command_override_momentary(
    struct bacnet_lighting_command_data *data, float value);

BACNET_STACK_EXPORT
float lighting_command_ramp_rate_clamp(float ramp_rate);
BACNET_STACK_EXPORT
float lighting_command_step_increment_clamp(float step_increment);

BACNET_STACK_EXPORT
float lighting_command_operating_range_clamp_fade(
    struct bacnet_lighting_command_data *data,
    float value,
    uint16_t milliseconds);
BACNET_STACK_EXPORT
float lighting_command_normalized_range_clamp(float value);

BACNET_STACK_EXPORT
float lighting_command_normalized_on_range_clamp(float value);
BACNET_STACK_EXPORT
float lighting_command_normalized_to_physical_value(
    float min_value, float max_value, float normalized_value);
BACNET_STACK_EXPORT
float lighting_command_feedback_value(
    struct bacnet_lighting_command_data *data);

BACNET_STACK_EXPORT
void lighting_command_trim_set(
    struct bacnet_lighting_command_data *data,
    float High_End_Trim,
    float Low_End_Trim,
    uint32_t Trim_Fade_Time);
BACNET_STACK_EXPORT
void lighting_command_key_set(
    struct bacnet_lighting_command_data *data, uint32_t key);
BACNET_STACK_EXPORT
void lighting_command_tracking_value_callback_set(
    struct bacnet_lighting_command_data *data,
    lighting_command_tracking_value_callback cb);
BACNET_STACK_EXPORT
BACNET_LIGHTING_IN_PROGRESS
lighting_command_in_progress_get(struct bacnet_lighting_command_data *data);
BACNET_STACK_EXPORT
void lighting_command_in_progress_set(
    struct bacnet_lighting_command_data *data,
    BACNET_LIGHTING_IN_PROGRESS in_progress);
BACNET_STACK_EXPORT
float lighting_command_tracking_value_get(
    struct bacnet_lighting_command_data *data);
BACNET_STACK_EXPORT
void lighting_command_tracking_value_set(
    struct bacnet_lighting_command_data *data, float value);
BACNET_STACK_EXPORT
void lighting_command_blink_warn_feature_set(
    struct bacnet_lighting_command_data *data,
    float off_value,
    uint16_t interval,
    uint16_t count);
BACNET_STACK_EXPORT
bool lighting_command_blink_egress_active(
    struct bacnet_lighting_command_data *data);
BACNET_STACK_EXPORT
bool lighting_command_out_of_service_get(
    struct bacnet_lighting_command_data *data);
BACNET_STACK_EXPORT
void lighting_command_out_of_service_set(
    struct bacnet_lighting_command_data *data, bool value);
BACNET_STACK_EXPORT
float lighting_command_last_on_value_get(
    struct bacnet_lighting_command_data *data);
BACNET_STACK_EXPORT
void lighting_command_last_on_value_set(
    struct bacnet_lighting_command_data *data, float value);
BACNET_STACK_EXPORT
float lighting_command_default_on_value_get(
    struct bacnet_lighting_command_data *data);
BACNET_STACK_EXPORT
void lighting_command_default_on_value_set(
    struct bacnet_lighting_command_data *data, float value);
BACNET_STACK_EXPORT
bool lighting_command_overridden_status(
    struct bacnet_lighting_command_data *data);

BACNET_STACK_EXPORT
bool lighting_command_active(struct bacnet_lighting_command_data *data);

BACNET_STACK_EXPORT
void lighting_command_lock(struct bacnet_lighting_command_data *data);
BACNET_STACK_EXPORT
void lighting_command_unlock(struct bacnet_lighting_command_data *data);

BACNET_STACK_EXPORT
void lighting_command_refresh(struct bacnet_lighting_command_data *data);
BACNET_STACK_EXPORT
void lighting_command_init(struct bacnet_lighting_command_data *data);
BACNET_STACK_EXPORT
void lighting_command_notification_add(
    struct bacnet_lighting_command_data *data,
    struct lighting_command_notification *notification);
BACNET_STACK_EXPORT
void lighting_command_timer_notfication_add(
    struct bacnet_lighting_command_data *data,
    struct lighting_command_timer_notification *notification);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
