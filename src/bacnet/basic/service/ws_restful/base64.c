/**
 * @file
* @brief base64 encode/decode.
 * @author Mikhail Antropov
 * @date Sep 2023
  * @section LICENSE
 *
 * Copyright (C) 2023 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include <bacnet/basic/service/ws_restful/base64.h>

static const uint8_t base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t base64_encode_size(size_t size)
{
    size_t olen;
    olen = size * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
    olen += olen / 72; /* line feeds */
    return olen;
}

size_t base64_encode(const uint8_t *src, size_t len, uint8_t *out)
{
    uint8_t *pos;
    const uint8_t *end, *in;
    int line_len;

    end = src + len;
    in = src;
    pos = out;
    line_len = 0;
    while (end - in >= 3) {
        *pos++ = base64_table[(in[0] >> 2) & 0x3f];
        *pos++ = base64_table[(((in[0] & 0x03) << 4) | (in[1] >> 4)) & 0x3f];
        *pos++ = base64_table[(((in[1] & 0x0f) << 2) | (in[2] >> 6)) & 0x3f];
        *pos++ = base64_table[in[2] & 0x3f];
        in += 3;
        line_len += 4;
        if (line_len >= 72) {
            *pos++ = '\n';
            line_len = 0;
        }
    }

    if (end - in) {
        *pos++ = base64_table[(in[0] >> 2) & 0x3f];
        if (end - in == 1) {
            *pos++ = base64_table[((in[0] & 0x03) << 4) & 0x3f];
            *pos++ = '=';
        } else {
            *pos++ = base64_table[(((in[0] & 0x03) << 4) |
                    (in[1] >> 4)) & 0x3f];
            *pos++ = base64_table[((in[1] & 0x0f) << 2) & 0x3f];
        }
        *pos++ = '=';
        line_len += 4;
    }

    if (line_len)
        *pos++ = '\n';

    return pos - out;
}

// allign up to
size_t base64_decode_size(size_t size)
{
    return (size + 3) / 4 * 3;
}

size_t base64_inplace_decode(const uint8_t *src, size_t len)
{
    return base64_decode(src, len, (uint8_t *)src);
}

size_t base64_decode(const uint8_t *src, size_t len, uint8_t *out)
{
    uint8_t dtable[256], *pos, block[4];
    size_t i, count, olen;
    int pad = 0;
    size_t extra_pad;
    uint8_t val;

    memset(dtable, 0x80, 256);
    for (i = 0; i < sizeof(base64_table) - 1; i++)
        dtable[base64_table[i]] = (uint8_t)i;
    dtable['='] = 0;

    count = 0;
    for (i = 0; i < len; i++) {
        if (dtable[src[i]] != 0x80)
            count++;
    }

    if (count == 0)
        return 0;
    extra_pad = (4 - count % 4) % 4;

    olen = (count + extra_pad) / 4 * 3;
    if (olen > len)
        return 0;

    pos = out;
    count = 0;
    for (i = 0; i < len + extra_pad; i++) {

        val = (i < len) ? src[i] : '=';
        if (val == '=') {
            pad++;
        }

        block[count] = dtable[val];
        if (block[count] == 0x80) {
            continue;
        }
        count++;

        if (count == 4) {
            *pos++ = (block[0] << 2) | (block[1] >> 4);
            if (pad < 2)
                *pos++ = (block[1] << 4) | (block[2] >> 2);
            if (pad < 1)
                *pos++ = (block[2] << 6) | block[3];
            count = 0;

            if (pad) {
                if (pad > 2) {
                    /* Invalid padding */
                    return 0;
                }
                break;
            }
        }
    }

    return pos - out;
}
