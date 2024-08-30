/*
 *   Copyright (C) 2008 John Crispin <blogic@openwrt.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _UCI_H__
#define _UCI_H__

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

BACNET_STACK_EXPORT
struct uci_context *ucix_init(const char *config_file);
BACNET_STACK_EXPORT
struct uci_context *ucix_init_path(const char *path, const char *config_file);
BACNET_STACK_EXPORT
void ucix_cleanup(struct uci_context *ctx);
BACNET_STACK_EXPORT
void ucix_save(struct uci_context *ctx);
BACNET_STACK_EXPORT
void ucix_save_state(struct uci_context *ctx);
BACNET_STACK_EXPORT
const char *ucix_get_option(
    struct uci_context *ctx, const char *p, const char *s, const char *o);
BACNET_STACK_EXPORT
int ucix_get_option_int(
    struct uci_context *ctx,
    const char *p,
    const char *s,
    const char *o,
    int def);
BACNET_STACK_EXPORT
void ucix_add_section(
    struct uci_context *ctx, const char *p, const char *s, const char *t);
BACNET_STACK_EXPORT
void ucix_add_option(
    struct uci_context *ctx,
    const char *p,
    const char *s,
    const char *o,
    const char *t);
BACNET_STACK_EXPORT
void ucix_add_option_int(
    struct uci_context *ctx,
    const char *p,
    const char *s,
    const char *o,
    int t);
BACNET_STACK_EXPORT
int ucix_commit(struct uci_context *ctx, const char *p);
BACNET_STACK_EXPORT
void ucix_revert(
    struct uci_context *ctx, const char *p, const char *s, const char *o);
BACNET_STACK_EXPORT
void ucix_del(
    struct uci_context *ctx, const char *p, const char *s, const char *o);
BACNET_STACK_EXPORT
void ucix_for_each_section_type(
    struct uci_context *ctx,
    const char *p,
    const char *t,
    void (*cb)(const char *, void *),
    void *priv);
#endif
