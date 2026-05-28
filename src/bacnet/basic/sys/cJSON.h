/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors
  SPDX-License-Identifier: MIT

  Trimmed single-header cJSON for embedded use.
*/
#ifndef CJSON_H
#define CJSON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* cJSON Types */
#define cJSON_Invalid (0)
#define cJSON_False (1 << 0)
#define cJSON_True (1 << 1)
#define cJSON_NULL (1 << 2)
#define cJSON_Number (1 << 3)
#define cJSON_String (1 << 4)
#define cJSON_Array (1 << 5)
#define cJSON_Object (1 << 6)
#define cJSON_Raw (1 << 7)

#define cJSON_IsReference 256
#define cJSON_StringIsConst 512

typedef struct cJSON {
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int type;
    char *valuestring;
    int valueint; /* DEPRECATED, use cJSON_SetNumberValue */
    double valuedouble;
    char *string;
} cJSON;

/* Parses a JSON string and returns a cJSON tree. */
cJSON *cJSON_Parse(const char *value);

/* Deletes a cJSON tree. */
void cJSON_Delete(cJSON *item);

/* Returns the error pointer after a failed parse. */
const char *cJSON_GetErrorPtr(void);

/* Array / object access */
cJSON *cJSON_GetArrayItem(const cJSON *array, int index);
int cJSON_GetArraySize(const cJSON *array);
cJSON *cJSON_GetObjectItem(const cJSON *const object, const char *const string);

/* Type checks */
int cJSON_IsInvalid(const cJSON *const item);
int cJSON_IsFalse(const cJSON *const item);
int cJSON_IsTrue(const cJSON *const item);
int cJSON_IsBool(const cJSON *const item);
int cJSON_IsNull(const cJSON *const item);
int cJSON_IsNumber(const cJSON *const item);
int cJSON_IsString(const cJSON *const item);
int cJSON_IsArray(const cJSON *const item);
int cJSON_IsObject(const cJSON *const item);
int cJSON_IsRaw(const cJSON *const item);

/* Iterate over array/object – uses next pointer */
#define cJSON_ArrayForEach(element, array)                    \
    for ((element) = (array != NULL) ? (array)->child : NULL; \
         (element) != NULL; (element) = (element)->next)

#ifdef __cplusplus
}
#endif
#endif /* CJSON_H */
