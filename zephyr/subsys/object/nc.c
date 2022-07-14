/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include "object.h"
#include "bacnet/basic/object/nc.h"

#if defined(INTRINSIC_REPORTING)
    OBJECT_FUNCTIONS(Notification_Class, NOTIFICATION_CLASS_INFO);
#endif
