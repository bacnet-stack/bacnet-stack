/**
 * @file
 * @brief BBMD Host Name handling for BACnet/IPv4
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdlib.h>
#include <string.h>
#include "bacnet/bacdef.h"
#include "bacnet/basic/bbmd/h_bbmdhost.h"
#include "bacnet/basic/bbmd/h_bbmd.h"
#include "bacnet/hostnport.h"
#include "bacnet/basic/sys/keylist.h"

/* Keylist of hostnames using minimial host-n-port structure */
static OS_Keylist Host_Keylist;

/**
 * @brief Initialize the host list
 */
void bbmdhost_init(void)
{
    if (!Host_Keylist) {
        Host_Keylist = Keylist_Create();
    }
}

/**
 * @brief Cleanup the host list and free memory
 */
void bbmdhost_cleanup(void)
{
    BACNET_HOST_ADDRESS_PAIR *entry;

    if (Host_Keylist) {
        entry = Keylist_Data_Pop(Host_Keylist);
        while (entry) {
            free(entry);
            entry = Keylist_Data_Pop(Host_Keylist);
        }
        Keylist_Delete(Host_Keylist);
        Host_Keylist = NULL;
    }
}

/**
 * @brief Compare a hostname to a character string
 * @param entry [in] The host-n-port structure
 * @param name [in] The character string
 * @return true if the structures are the same, false otherwise
 */
bool bbmdhost_name_same(
    BACNET_HOST_ADDRESS_PAIR *entry, BACNET_CHARACTER_STRING *name)
{
    if (!entry || !name) {
        return false;
    }
    if (entry->name.length != characterstring_length(name)) {
        return false;
    }
    if (entry->name.length > 0) {
        if (memcmp(
                entry->name.fqdn, characterstring_value(name),
                entry->name.length) != 0) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Compare two host-n-port structures
 * @param dst [in] The first host-n-port structure
 * @param src [in] The second host-n-port structure
 * @return true if the structures are the same, false otherwise
 */
bool bbmdhost_same(BACNET_HOST_ADDRESS_PAIR *dst, BACNET_HOST_ADDRESS_PAIR *src)
{
    if (!dst || !src) {
        return false;
    }
    if (dst->name.length != src->name.length) {
        return false;
    }
    if (dst->ip_address.length != src->ip_address.length) {
        return false;
    }
    if (dst->name.length > 0) {
        if (memcmp(dst->name.fqdn, src->name.fqdn, dst->name.length) != 0) {
            return false;
        }
    }
    if ((dst->ip_address.length > 0) &&
        (dst->ip_address.length < sizeof(dst->ip_address.address))) {
        if (memcmp(
                dst->ip_address.address, src->ip_address.address,
                dst->ip_address.length) != 0) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Find a host name in the list
 * @param name [in] The hostname to search for
 * @return the index where it was found, or -1 on failure
 */
int bbmdhost_name_find(BACNET_CHARACTER_STRING *name)
{
    int index;
    int count;
    BACNET_HOST_ADDRESS_PAIR *entry;

    if (!Host_Keylist || !name) {
        return -1;
    }
    count = Keylist_Count(Host_Keylist);
    for (index = 0; index < count; index++) {
        entry = Keylist_Data_Index(Host_Keylist, index);
        if (bbmdhost_name_same(entry, name)) {
            return index;
        }
    }

    return -1;
}

/**
 * @brief Add a host name to the list
 * @param name [in] The hostname
 * @return the index where it was added, or -1 on failure
 */
int bbmdhost_add(BACNET_CHARACTER_STRING *name)
{
    BACNET_HOST_ADDRESS_PAIR *entry;
    KEY key;
    size_t length;
    int index;
    const char *hostname;

    if (!Host_Keylist || !name) {
        return -1;
    }
    hostname = characterstring_value(name);
    length = characterstring_length(name);
    /* check for existing entry */
    index = bbmdhost_name_find(name);
    if (index >= 0) {
        return index;
    }
    /* add new entry */
    entry = calloc(1, sizeof(BACNET_HOST_ADDRESS_PAIR));
    if (!entry) {
        return -1;
    }
    if (length > sizeof(entry->name.fqdn) - 1) {
        length = sizeof(entry->name.fqdn) - 1;
    }
    memcpy(entry->name.fqdn, hostname, length);
    entry->name.fqdn[length] = '\0';
    entry->name.length = (uint8_t)length;
    key = Keylist_Count(Host_Keylist);

    return Keylist_Data_Add(Host_Keylist, key, entry);
}

/**
 * @brief Returns the number of items in the list
 * @return the number of items in the list
 */
int bbmdhost_count(void)
{
    return Keylist_Count(Host_Keylist);
}

/**
 * @brief Returns the data from the node specified by index
 * @param index [in] The index to the list
 * @param host [out] The host and port data
 * @return true if the data was returned. Note that the host->tag
 *  will be TAG_IP_ADDRESS if hostname has been resolved to an IP address,
 *  TAG_NAME if hostname is not resolved, TAG_NONE if no data is available
 */
bool bbmdhost_data_resolved(int index, BACNET_HOST_N_PORT_MINIMAL *host)
{
    BACNET_HOST_ADDRESS_PAIR *entry;

    entry = Keylist_Data_Index(Host_Keylist, index);
    if (entry) {
        if (host) {
            memset(host, 0, sizeof(BACNET_HOST_N_PORT_MINIMAL));
            if (entry->ip_address.length > 0) {
                host->tag = BACNET_HOST_ADDRESS_TAG_IP_ADDRESS;
                memcpy(
                    &host->host.ip_address, &entry->ip_address,
                    sizeof(host->host.ip_address));
            } else if (entry->name.length > 0) {
                host->tag = BACNET_HOST_ADDRESS_TAG_NAME;
                memcpy(&host->host.name, &entry->name, sizeof(host->host.name));
            } else {
                host->tag = BACNET_HOST_ADDRESS_TAG_NONE;
            }
        }
        return true;
    }

    return false;
}

/**
 * @brief Returns the data from the node specified by index
 * @param index [in] The index to the list
 * @param data [out] The host address pair data
 * @return true if the data was returned
 */
bool bbmdhost_data(int index, BACNET_HOST_ADDRESS_PAIR *data)
{
    BACNET_HOST_ADDRESS_PAIR *entry;

    entry = Keylist_Data_Index(Host_Keylist, index);
    if (entry && data) {
        memcpy(data, entry, sizeof(BACNET_HOST_ADDRESS_PAIR));
        return true;
    }

    return false;
}

/**
 * @brief Update the IP addresses for all hostnames in the list
 * @param callback [in] The callback to query the hostname to IP address
 */
void bbmdhost_resolver_iterate(bbmdhost_resolver_callback resolver)
{
    int index;
    int count;
    BACNET_HOST_ADDRESS_PAIR *entry;
    BACNET_HOST_N_PORT_MINIMAL host;

    count = Keylist_Count(Host_Keylist);
    for (index = 0; index < count; index++) {
        entry = Keylist_Data_Index(Host_Keylist, index);
        if (entry) {
            memset(&host, 0, sizeof(host));
            if (entry->name.length > 0) {
                host.tag = BACNET_HOST_ADDRESS_TAG_NAME;
                memcpy(&host.host.name, &entry->name, sizeof(host.host.name));
                if (resolver) {
                    resolver(&host);
                }
                if (host.tag == BACNET_HOST_ADDRESS_TAG_IP_ADDRESS) {
                    memcpy(
                        &entry->ip_address, &host.host.ip_address,
                        sizeof(entry->ip_address));
                }
            }
        }
    }
}

/**
 * @brief Returns the IP address for the node specified by index
 * @param index [in] The index to the list
 * @param ip_address [out] The IP address
 * @param ip_address_len [out] The length of the IP address
 * @return true if the IP address was returned
 */
bool bbmdhost_ip_address(
    int index, uint8_t *ip_address, uint8_t *ip_address_len)
{
    BACNET_HOST_ADDRESS_PAIR *entry;

    entry = Keylist_Data_Index(Host_Keylist, index);
    if (entry && (entry->ip_address.length > 0)) {
        if (ip_address) {
            memcpy(
                ip_address, entry->ip_address.address,
                entry->ip_address.length);
        }
        if (ip_address_len) {
            *ip_address_len = entry->ip_address.length;
        }
        return true;
    }

    return false;
}

/**
 * @brief Returns the IP address for the hostname in the list
 * @param hostname [in] The hostname to search for
 * @param ip_address [out] The IP address
 * @param ip_address_len [out] The length of the IP address
 * @return true if the IP address was found and returned
 */
bool bbmdhost_hostname_ip_address(
    const char *hostname, uint8_t *ip_address, uint8_t *ip_address_len)
{
    int index;
    int count;
    BACNET_HOST_ADDRESS_PAIR *entry;

    if (!hostname) {
        return false;
    }
    count = Keylist_Count(Host_Keylist);
    for (index = 0; index < count; index++) {
        entry = Keylist_Data_Index(Host_Keylist, index);
        if (entry && (entry->ip_address.length > 0)) {
            if (strcmp(entry->name.fqdn, hostname) == 0) {
                if (ip_address) {
                    memcpy(
                        ip_address, entry->ip_address.address,
                        entry->ip_address.length);
                }
                if (ip_address_len) {
                    *ip_address_len = entry->ip_address.length;
                }
                return true;
            }
        }
    }

    return false;
}
