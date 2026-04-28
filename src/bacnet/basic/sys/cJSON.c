/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors
  SPDX-License-Identifier: MIT

  Trimmed cJSON implementation for embedded / gateway use.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>

#include "cJSON.h"

/* ------------------------------------------------------------------ */
/* Internal state                                                       */
/* ------------------------------------------------------------------ */
static const char *global_ep = NULL;

const char *cJSON_GetErrorPtr(void)
{
    return global_ep;
}

/* ------------------------------------------------------------------ */
/* Memory helpers                                                       */
/* ------------------------------------------------------------------ */
static cJSON *cJSON_New_Item(void)
{
    cJSON *node = (cJSON *)calloc(1, sizeof(cJSON));
    return node;
}

void cJSON_Delete(cJSON *c)
{
    cJSON *next;
    while (c) {
        next = c->next;
        if (!(c->type & cJSON_IsReference) && c->child) {
            cJSON_Delete(c->child);
        }
        if (!(c->type & cJSON_IsReference) && c->valuestring) {
            free(c->valuestring);
        }
        if (!(c->type & cJSON_StringIsConst) && c->string) {
            free(c->string);
        }
        free(c);
        c = next;
    }
}

/* ------------------------------------------------------------------ */
/* Parse number                                                         */
/* ------------------------------------------------------------------ */
static const char *parse_number(cJSON *item, const char *num)
{
    double n = 0;
    double sign = 1.0;
    double scale = 0;
    int subscale = 0, signsubscale = 1;

    if (*num == '-') {
        sign = -1.0;
        num++;
    }
    if (*num == '0') {
        num++;
    } else if (*num >= '1' && *num <= '9') {
        do {
            n = n * 10.0 + (*num - '0');
            num++;
        } while (*num >= '0' && *num <= '9');
    }
    if (*num == '.') {
        num++;
        while (*num >= '0' && *num <= '9') {
            n = n * 10.0 + (*num - '0');
            scale--;
            num++;
        }
    }
    if (*num == 'e' || *num == 'E') {
        num++;
        if (*num == '+') {
            num++;
        } else if (*num == '-') {
            signsubscale = -1;
            num++;
        }
        while (*num >= '0' && *num <= '9') {
            subscale = subscale * 10 + (*num - '0');
            num++;
        }
    }
    n = sign * n * pow(10.0, scale + subscale * signsubscale);
    item->valuedouble = n;
    item->valueint = (int)n;
    item->type = cJSON_Number;
    return num;
}

/* ------------------------------------------------------------------ */
/* Parse string (stored without surrounding quotes)                     */
/* ------------------------------------------------------------------ */
static unsigned parse_hex4(const char *str)
{
    unsigned h = 0;
    int i;
    for (i = 0; i < 4; i++) {
        if (*str >= '0' && *str <= '9') {
            h = h * 16 + (*str - '0');
        } else if (*str >= 'A' && *str <= 'F') {
            h = h * 16 + (*str - 'A') + 10;
        } else if (*str >= 'a' && *str <= 'f') {
            h = h * 16 + (*str - 'a') + 10;
        } else {
            return 0;
        }
        str++;
    }
    return h;
}

static const char *parse_string(cJSON *item, const char *str)
{
    const char *ptr = str + 1;
    char *ptr2;
    char *out;
    int len = 0;
    unsigned uc, uc2;

    if (*str != '\"') {
        global_ep = str;
        return NULL;
    }

    while (*ptr != '\"' && *ptr && ++len) {
        if (*ptr++ == '\\') {
            ptr++;
        }
    }

    out = (char *)malloc(len + 1);
    if (!out) {
        return NULL;
    }

    ptr = str + 1;
    ptr2 = out;
    while (*ptr != '\"' && *ptr) {
        if (*ptr != '\\') {
            *ptr2++ = *ptr++;
        } else {
            ptr++;
            switch (*ptr) {
                case 'b':
                    *ptr2++ = '\b';
                    break;
                case 'f':
                    *ptr2++ = '\f';
                    break;
                case 'n':
                    *ptr2++ = '\n';
                    break;
                case 'r':
                    *ptr2++ = '\r';
                    break;
                case 't':
                    *ptr2++ = '\t';
                    break;
                case 'u':
                    uc = parse_hex4(ptr + 1);
                    ptr += 4;
                    if (uc >= 0xD800 && uc <= 0xDBFF) {
                        if (ptr[1] == '\\' && ptr[2] == 'u') {
                            uc2 = parse_hex4(ptr + 3);
                            ptr += 6;
                            uc = 0x10000 +
                                (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
                        }
                    }
                    if (uc < 0x80) {
                        *ptr2++ = (char)uc;
                    } else if (uc < 0x800) {
                        *ptr2++ = (char)(0xC0 | (uc >> 6));
                        *ptr2++ = (char)(0x80 | (uc & 0x3F));
                    } else if (uc < 0x10000) {
                        *ptr2++ = (char)(0xE0 | (uc >> 12));
                        *ptr2++ = (char)(0x80 | ((uc >> 6) & 0x3F));
                        *ptr2++ = (char)(0x80 | (uc & 0x3F));
                    } else {
                        *ptr2++ = (char)(0xF0 | (uc >> 18));
                        *ptr2++ = (char)(0x80 | ((uc >> 12) & 0x3F));
                        *ptr2++ = (char)(0x80 | ((uc >> 6) & 0x3F));
                        *ptr2++ = (char)(0x80 | (uc & 0x3F));
                    }
                    break;
                default:
                    *ptr2++ = *ptr;
                    break;
            }
            ptr++;
        }
    }
    *ptr2 = '\0';
    if (*ptr == '\"') {
        ptr++;
    }
    item->valuestring = out;
    item->type = cJSON_String;
    return ptr;
}

/* ------------------------------------------------------------------ */
/* Skip whitespace / comments                                           */
/* ------------------------------------------------------------------ */
static const char *skip(const char *in)
{
    while (in && *in && (unsigned char)*in <= ' ') {
        in++;
    }
    return in;
}

/* ------------------------------------------------------------------ */
/* Forward declare                                                      */
/* ------------------------------------------------------------------ */
static const char *parse_value(cJSON *item, const char *value);
static const char *parse_array(cJSON *item, const char *value);
static const char *parse_object(cJSON *item, const char *value);

/* ------------------------------------------------------------------ */
/* parse_value                                                          */
/* ------------------------------------------------------------------ */
static const char *parse_value(cJSON *item, const char *value)
{
    if (!value) {
        return NULL;
    }
    if (!strncmp(value, "null", 4)) {
        item->type = cJSON_NULL;
        return value + 4;
    }
    if (!strncmp(value, "false", 5)) {
        item->type = cJSON_False;
        return value + 5;
    }
    if (!strncmp(value, "true", 4)) {
        item->type = cJSON_True;
        item->valueint = 1;
        return value + 4;
    }
    if (*value == '\"') {
        return parse_string(item, value);
    }
    if (*value == '-' || (*value >= '0' && *value <= '9')) {
        return parse_number(item, value);
    }
    if (*value == '[') {
        return parse_array(item, value);
    }
    if (*value == '{') {
        return parse_object(item, value);
    }
    global_ep = value;
    return NULL;
}

/* ------------------------------------------------------------------ */
/* parse_array                                                          */
/* ------------------------------------------------------------------ */
static const char *parse_array(cJSON *item, const char *value)
{
    cJSON *child;
    if (*value != '[') {
        global_ep = value;
        return NULL;
    }
    item->type = cJSON_Array;
    value = skip(value + 1);
    if (*value == ']') {
        return value + 1;
    }

    item->child = child = cJSON_New_Item();
    if (!item->child) {
        return NULL;
    }
    value = skip(parse_value(child, skip(value)));
    if (!value) {
        return NULL;
    }

    while (*value == ',') {
        cJSON *new_item = cJSON_New_Item();
        if (!new_item) {
            return NULL;
        }
        child->next = new_item;
        new_item->prev = child;
        child = new_item;
        value = skip(parse_value(child, skip(value + 1)));
        if (!value) {
            return NULL;
        }
    }
    if (*value == ']') {
        return value + 1;
    }
    global_ep = value;
    return NULL;
}

/* ------------------------------------------------------------------ */
/* parse_object                                                         */
/* ------------------------------------------------------------------ */
static const char *parse_object(cJSON *item, const char *value)
{
    cJSON *child;
    if (*value != '{') {
        global_ep = value;
        return NULL;
    }
    item->type = cJSON_Object;
    value = skip(value + 1);
    if (*value == '}') {
        return value + 1;
    }

    item->child = child = cJSON_New_Item();
    if (!item->child) {
        return NULL;
    }

    value = skip(parse_string(child, skip(value)));
    if (!value) {
        return NULL;
    }
    child->string = child->valuestring;
    child->valuestring = NULL;

    if (*value != ':') {
        global_ep = value;
        return NULL;
    }
    value = skip(parse_value(child, skip(value + 1)));
    if (!value) {
        return NULL;
    }

    while (*value == ',') {
        cJSON *new_item = cJSON_New_Item();
        if (!new_item) {
            return NULL;
        }
        child->next = new_item;
        new_item->prev = child;
        child = new_item;

        value = skip(parse_string(child, skip(value + 1)));
        if (!value) {
            return NULL;
        }
        child->string = child->valuestring;
        child->valuestring = NULL;

        if (*value != ':') {
            global_ep = value;
            return NULL;
        }
        value = skip(parse_value(child, skip(value + 1)));
        if (!value) {
            return NULL;
        }
    }
    if (*value == '}') {
        return value + 1;
    }
    global_ep = value;
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */
cJSON *cJSON_Parse(const char *value)
{
    cJSON *c = cJSON_New_Item();
    global_ep = NULL;
    if (!c) {
        return NULL;
    }
    if (!parse_value(c, skip(value))) {
        cJSON_Delete(c);
        return NULL;
    }
    return c;
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int item)
{
    cJSON *c = array ? array->child : NULL;
    while (c && item > 0) {
        item--;
        c = c->next;
    }
    return c;
}

int cJSON_GetArraySize(const cJSON *array)
{
    cJSON *c = array ? array->child : NULL;
    int i = 0;
    while (c) {
        i++;
        c = c->next;
    }
    return i;
}

cJSON *cJSON_GetObjectItem(const cJSON *const object, const char *const string)
{
    cJSON *c = object ? object->child : NULL;
    while (c && c->string && strcmp(c->string, string)) {
        c = c->next;
    }
    return c;
}

/* Type check helpers */
int cJSON_IsInvalid(const cJSON *const item)
{
    return item ? (item->type & 0xFF) == cJSON_Invalid : 0;
}
int cJSON_IsFalse(const cJSON *const item)
{
    return item ? (item->type & 0xFF) == cJSON_False : 0;
}
int cJSON_IsTrue(const cJSON *const item)
{
    return item ? (item->type & 0xFF) == cJSON_True : 0;
}
int cJSON_IsBool(const cJSON *const item)
{
    return item ? ((item->type & 0xFF) == cJSON_True ||
                   (item->type & 0xFF) == cJSON_False)
                : 0;
}
int cJSON_IsNull(const cJSON *const item)
{
    return item ? (item->type & 0xFF) == cJSON_NULL : 0;
}
int cJSON_IsNumber(const cJSON *const item)
{
    return item ? (item->type & 0xFF) == cJSON_Number : 0;
}
int cJSON_IsString(const cJSON *const item)
{
    return item ? (item->type & 0xFF) == cJSON_String : 0;
}
int cJSON_IsArray(const cJSON *const item)
{
    return item ? (item->type & 0xFF) == cJSON_Array : 0;
}
int cJSON_IsObject(const cJSON *const item)
{
    return item ? (item->type & 0xFF) == cJSON_Object : 0;
}
int cJSON_IsRaw(const cJSON *const item)
{
    return item ? (item->type & 0xFF) == cJSON_Raw : 0;
}
