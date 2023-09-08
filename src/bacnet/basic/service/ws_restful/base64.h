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

#include <stddef.h>
#include <stdint.h>

size_t base64_encode_size(size_t size);

size_t base64_encode(const uint8_t *src, size_t len, uint8_t *out);

size_t base64_decode_size(size_t size);

size_t base64_inplace_decode(const uint8_t *src, size_t len);

size_t base64_decode(const uint8_t *src, size_t len, uint8_t *out);
