/**
 * @file
 * @brief Client/Server thread-safe websocket interface API.
 * @author Kirill Neznamov
 * @date May 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BACNET__WEBSOCKET__INCLUDED__
#define __BACNET__WEBSOCKET__INCLUDED__

#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/datalink/bsc/bsc-conf.h"
/**
 * Websocket connection timeout
 * @{
 */

#ifndef BSC_CONF_WEBSOCKET_TIMEOUT_SECONDS
#define BACNET_WEBSOCKET_TIMEOUT_SECONDS 10
#else
#define BACNET_WEBSOCKET_TIMEOUT_SECONDS BSC_CONF_WEBSOCKET_TIMEOUT_SECONDS
#endif

/**
 * Maximum number of sockets that can be opened on client's side.
 * @{
 */
#ifndef BSC_CONF_CLIENT_CONNECTIONS_NUM
#define BACNET_CLIENT_WEBSOCKETS_MAX_NUM 4
#else
#define BACNET_CLIENT_WEBSOCKETS_MAX_NUM BSC_CONF_CLIENT_CONNECTIONS_NUM
#endif

/** @} */

/**
 * Client websocket buffer size for received data.
 * Value must be a power of 2.
 * @{
 */
#ifndef BSC_CONF_CLIENT_WEBSOCKET_RX_BUFFER_SIZE
#define BACNET_CLIENT_WEBSOCKET_RX_BUFFER_SIZE 4096
#else
#define BACNET_CLIENT_WEBSOCKET_RX_BUFFER_SIZE BSC_CONF_CLIENT_WEBSOCKET_RX_BUFFER_SIZE
#endif

/** @} */

/**
 * Maximum number of sockets supported for hub websocket server
 * @{
 */
#ifndef BSC_CONF_SERVER_HUB_CONNECTIONS_MAX_NUM
#define BACNET_SERVER_HUB_WEBSOCKETS_MAX_NUM 4
#else
#define BACNET_SERVER_HUB_WEBSOCKETS_MAX_NUM BSC_CONF_SERVER_HUB_CONNECTIONS_MAX_NUM
#endif

/** @} */


/**
 * Maximum number of sockets supported for direct websocket server
 * @{
 */
#ifndef BSC_CONF_SERVER_DIRECT_CONNECTIONS_MAX_NUM
#define BACNET_SERVER_DIRECT_WEBSOCKETS_MAX_NUM 4
#else
#define BACNET_SERVER_DIRECT_WEBSOCKETS_MAX_NUM BSC_CONF_SERVER_DIRECT_CONNECTIONS_MAX_NUM
#endif

/** @} */

/**
 * Server websocket buffer size for received data.
 * Value must be a power of 2.
 * @{
 */

#ifndef BSC_CONF_SERVER_WEBSOCKET_RX_BUFFER_SIZE
#define BACNET_SERVER_WEBSOCKET_RX_BUFFER_SIZE 4096
#else
#define BACNET_SERVER_WEBSOCKET_RX_BUFFER_SIZE BSC_CONF_SERVER_WEBSOCKET_RX_BUFFER_SIZE
#endif

/** @} */


#define BACNET_WSURL_MAX_LEN 256

typedef int BACNET_WEBSOCKET_HANDLE;
#define BACNET_WEBSOCKET_INVALID_HANDLE (-1)

// Websockets protocol defined in BACnet/SC \S AB.7.1.
#define BACNET_WEBSOCKET_HUB_PROTOCOL_STR "hub.bsc.bacnet.org"
#define BACNET_WEBSOCKET_DIRECT_PROTOCOL_STR "dc.bsc.bacnet.org"

typedef enum {
    BACNET_WEBSOCKET_HUB_PROTOCOL = 0,
    BACNET_WEBSOCKET_DIRECT_PROTOCOL = 1,
    BACNET_WEBSOCKET_PROTOCOLS_AMOUNT = 2   // must be always last
} BACNET_WEBSOCKET_PROTOCOL;

typedef enum {
    BACNET_WEBSOCKET_SUCCESS = 0,
    BACNET_WEBSOCKET_CLOSED = 1,
    BACNET_WEBSOCKET_NO_RESOURCES = 2,
    BACNET_WEBSOCKET_OPERATION_IN_PROGRESS = 3,
    BACNET_WEBSOCKET_BAD_PARAM = 4,
    BACNET_WEBSOCKET_TIMEDOUT = 5,
    BACNET_WEBSOCKET_INVALID_OPERATION = 6,
    BACNET_WEBSOCKET_BUFFER_TOO_SMALL = 7,
    BACNET_WEBSOCKET_OPERATION_CANCELED = 8
} BACNET_WEBSOCKET_RET;

typedef struct BACNetWebsocketClient {
    /**
     * @brief Blocking bws_сonnect() function opens a new connection to
     * a websocket server specified by url parameter.
     *
     * @param type - type of BACNet/SC connection, check
     * BACNET_WEBSOCKET_CONNECTION_TYPE enum. According BACNet standard
     * different type of connections require different websocket protocols.
     * @param url - BACNet/SC server URL. For example: wss://legrand.com:8080.
     * @param ca_cert - pointer to certificate authority (CA) cert in PEM or DER
     * format.
     * @param ca_cert_size - size in bytes of CA cert.
     * @param cert - pointer to client certificate in PEM or DER format.
     * @param cert_size - size in bytes of client certificate.
     * @param key - pointer to client private key in PEM or DER format.
     * @param key_size - size of private key in bytes of of client certificate.
     * @param out_handle - pointer to a valid websocket handle in a case if
     * connection succeeded.
     * @return error code from BACNET_WEBSOCKET_RET enum.
     *    The following error codes can be returned:
     *     BACNET_WEBSOCKET_BAD_PARAM - In a case if some input parameter is
     *                                  incorrect.
     *     BACNET_WEBSOCKET_NO_RESOURCES - if a user has already opened
     *         more sockets than the limit defined by BACNET_CLIENT_WEBSOCKETS_MAX_NUM,
     *         or if some mem allocation has failed or some allocation of system
     *         resources like mutex, thread, etc .., failed.
     *     BACNET_WEBSOCKET_CLOSED -if the bws_disconnect() was called on same
     *                              websocket handle from some other thread.
     *                              BACNET_WEBSOCKET_SUCCESS - the connection
     *                              attempt has succeded.
     */

    BACNET_WEBSOCKET_RET(*bws_connect)
    (BACNET_WEBSOCKET_PROTOCOL proto,
        char *url,
        uint8_t *ca_cert,
        size_t ca_cert_size,
        uint8_t *cert,
        size_t cert_size,
        uint8_t *key,
        size_t key_size,
        BACNET_WEBSOCKET_HANDLE *out_handle);

    /**
     * @brief Blocking bws_disconnnect() function closes previously opened
     * connection to some websocket server.
     *
     * @param h - websocket handle.
     *
     * @return error code from BACNET_WEBSOCKET_RET enum.
     *    The following error codes can be returned:
     *      BACNET_WEBSOCKET_BAD_PARAM - In a case if some input parameter is
     *                                   incorrect.
     *      BACNET_WEBSOCKET_CLOSED - websocket was already closed by
     *                                remote peer or by bws_disconnect()
     *                                call from other thread.
     *      BACNET_WEBSOCKET_OPERATION_IN_PROGRESS - some other thread has
     *                                already started disconnect operation
     *                                on socket h.
     *      BACNET_WEBSOCKET_SUCCESS - operation has succeded.
     */

    BACNET_WEBSOCKET_RET (*bws_disconnect)(BACNET_WEBSOCKET_HANDLE h);

    /**
     * @brief Blocking bws_send() function sends data to a websocket server.
     *
     * @param h - websocket handle.
     * @param payload - pointer to a data to send.
     * @param payload_size - size in bytes of data to send.
     *
     * @return error code from BACNET_WEBSOCKET_RET enum.
     *    The following error codes can be returned:
     *      BACNET_WEBSOCKET_BAD_PARAM - In a case if some input parameter is
     *                                   incorrect.
     *      BACNET_WEBSOCKET_CLOSED - websocket was already closed by
     *                                remote peer or by bws_disconnect() call
     *                                from other thread.
     *      BACNET_WEBSOCKET_OPERATION_IN_PROGRESS - some other thread has
     *                              started disconnect operation on socket h.
     *      BACNET_WEBSOCKET_SUCCESS - operation has succeded.
     */

    BACNET_WEBSOCKET_RET(*bws_send)
    (BACNET_WEBSOCKET_HANDLE h, uint8_t *payload, size_t payload_size);

    /**
     * @brief Blocking bws_recv() function receives data from a websocket
     * server.
     *
     * @param h - websocket handle.
     * @param buf - pointer to a buffer to store received data.
     * @param bufsize - size in bytes of a buffer to store received data.
     * @param bytes_received - pointer to actual number of bytes received.
     * @param timeout - timeout in milliseconds for receive operation.
     *
     * @return error code from BACNET_WEBSOCKET_RET enum.
     *    The following error codes can be returned:
     *     BACNET_WEBSOCKET_BAD_PARAM - In a case if some input parameter is
     *                                  incorrect.
     *     BACNET_WEBSOCKET_NO_RESOURCES - if some mem allocation has
     *       failed or some allocations of system resources like mutex, thread,
     *       etc.., has failed.
     *     BACNET_WEBSOCKET_CLOSED - websocket was already closed by
     *                               remote peer or by bws_disconnect() call
     *                               from other thread.
     *     BACNET_WEBSOCKET_OPERATION_IN_PROGRESS - some other thread has
     *                           started disconnect operation on socket h.
     *     BACNET_WEBSOCKET_TIMEDOUT - timeout was elapsed but no data was
     *                                 received.
     *     BACNET_WEBSOCKET_SUCCESS - operation has succeded.
     *     BACNET_WEBSOCKET_BUFFER_TOO_SMALL - received datagram sized
     *                         is larger than provided buffer size.
     *                         In that case only part of websocket datagram
     *                         equal to bufsize is copied into buf.
     */

    BACNET_WEBSOCKET_RET(*bws_recv)
    (BACNET_WEBSOCKET_HANDLE h,
        uint8_t *buf,
        size_t bufsize,
        size_t *bytes_received,
        int timeout);

} BACNET_WEBSOCKET_CLIENT;

typedef struct BACNetWebsocketServer {
    /**
     * @brief Blocking bws_start() function starts a websocket server on a
     * specified port for specified BACNet websocket protocol.
     *
     * @param proto - type of BACNet websocket protocol defined in
     *                BACNET_WEBSOCKET_PROTOCOL enum.
     * @param port- port number.
     * @param ca_cert - pointer to certificate authority (CA) cert in PEM or DER
     * format.
     * @param ca_cert_size - size in bytes of CA cert.
     * @param cert - pointer to server certificate in PEM or DER format.
     * @param cert_size - size in bytes of server certificate.
     * @param key - pointer to server private key in PEM or DER format.
     * @param key_size - size of private key in bytes of of client certificate.
     *
     * @return error code from BACNET_WEBSOCKET_RET enum.
     *  The following error codes can be returned:
     *    BACNET_WEBSOCKET_BAD_PARAM - In a case if some input parameter is
     *            incorrect.
     *    BACNET_WEBSOCKET_NO_RESOURCES - if a user has already opened
     *            more sockets than the limit defined to corresponded protocol
     *            (BACNET_SERVER_HUB_WEBSOCKETS_MAX_NUM or
     *             BACNET_CLIENT_WEBSOCKETS_MAX_NUM), or if some mem allocation
     *             has failed or some allocation of system resources like
     *             mutex, thread, condition variable etc .., failed.
     *    BACNET_WEBSOCKET_SUCCESS - the connection attempt has succeded.
     */

    BACNET_WEBSOCKET_RET (*bws_start) (
                            BACNET_WEBSOCKET_PROTOCOL proto,
                            int port,
                            uint8_t *ca_cert,
                            size_t ca_cert_size,
                            uint8_t *cert,
                            size_t cert_size,
                            uint8_t *key,
                            size_t key_size);

    /**
     * @brief Blocking bws_accept() function waits for incoming connection over
     * websocket from client. The function blocks the caller until a connection
     * is present or no pending connections are present on the internal server
     * accept queue or timeout elapses.
     *
     * @param out_handle- if function succeded conntains valid websocket handle.
     * @param timeout - timeout in milliseconds for accept operation.
     *
     * @return error code from BACNET_WEBSOCKET_RET enum.
     *  The following error codes can be returned:
     *    BACNET_WEBSOCKET_BAD_PARAM - In a case if some input parameter is
     *                                 ncorrect.
     *    BACNET_WEBSOCKET_INVALID_OPERATION - if server shutdown is in
     *                                         progress.
     *    BACNET_WEBSOCKET_NO_RESOURCES - if some mem allocation has
     *        failed or some allocations of system resources like mutex,
     *        thread, etc.., has failed.
     *    BACNET_WEBSOCKET_CLOSED - if the bws_disconnect() was
     *         called on same websocket handle from some other thread and
     *         socket was closed or if a remote peer has closed the connection
     *         or server was stopped.
     *    BACNET_WEBSOCKET_SUCCESS - incoming connection attempt has succeded.
     *    BACNET_WEBSOCKET_TIMEDOUT - timeout was elapsed but incoming connection
     *           was not established.
     *    BACNET_WEBSOCKET_OPERATION_CANCELED - the execution of call was
     *           canceled by bws_cancel_accept() call from other thread.
     */

    BACNET_WEBSOCKET_RET (*bws_accept)(
                            BACNET_WEBSOCKET_PROTOCOL protocol,
                            BACNET_WEBSOCKET_HANDLE* out_handle,
                            int timeout);

    /**
     * @brief bws_cancel_accept() function cancel waiting for result 
     *        of all blocked bws_accept() calls. As a result
     *        all pending bws_accept() calls will failed with
     *        BACNET_WEBSOCKET_OPERATION_CANCELED error.
     */

    void (*bws_cancel_accept)(BACNET_WEBSOCKET_PROTOCOL protocol);

    /**
     * @brief Blocking bws_disconnnect() function closes websocket handle.
     *
     * @param h - websocket handle.
     *
     * @return error code from BACNET_WEBSOCKET_RET enum.
     *  The following error codes can be returned:
     *    BACNET_WEBSOCKET_BAD_PARAM - In a case if some input parameter is
     *                                 incorrect.
     *    BACNET_WEBSOCKET_CLOSED - websocket was already closed by
     *           remote peer or by bws_disconnect() call from other thread.
     *    BACNET_WEBSOCKET_OPERATION_IN_PROGRESS - some other thread has
     *           already started disconnect operation on socket h.
     *    BACNET_WEBSOCKET_INVALID_OPERATION - if server was stopped or
     *           shutdown process is in progres or some incorrect handle
     *           was passed to function.
     *    BACNET_WEBSOCKET_SUCCESS - operation has succeded.
     */

    BACNET_WEBSOCKET_RET (*bws_disconnect) (
                            BACNET_WEBSOCKET_PROTOCOL protocol,
                            BACNET_WEBSOCKET_HANDLE h);

    /**
     * @brief Blocking bws_send() function sends data to a websocket client.
     *
     * @param h - websocket handle.
     * @param payload - pointer to a data to send.
     * @param payload_size - size in bytes of data to send.
     *
     * @return error code from BACNET_WEBSOCKET_RET enum.
     *  The following error codes can be returned:
     *    BACNET_WEBSOCKET_BAD_PARAM - In a case if some input parameter is
     *                                 incorrect.
     *    BACNET_WEBSOCKET_CLOSED - websocket was already closed by
     *           remote peer or by bws_disconnect() call from other thread.
     *    BACNET_WEBSOCKET_OPERATION_IN_PROGRESS - some other thread has
     *           started disconnect operation on socket h.
     *    BACNET_WEBSOCKET_INVALID_OPERATION - if server was stopped or
     *           server shutdown process is in progress.
     *    BACNET_WEBSOCKET_SUCCESS - operation has succeded.
     */

    BACNET_WEBSOCKET_RET (*bws_send) (
                            BACNET_WEBSOCKET_PROTOCOL protocol,
                            BACNET_WEBSOCKET_HANDLE h,
                            uint8_t *payload,
                            size_t payload_size);
    /**
     * @brief Blocking bws_recv() function receives data from a websocket
     * client specified by the websocket handle.
     *
     * @param h - websocket handle.
     * @param buf - pointer to a buffer to store received data.
     * @param bufsize - size in bytes of a buffer to store received data.
     * @param bytes_received - pointer to actual number of bytes received.
     * @param timeout - timeout in milliseconds for receive operation.
     *
     * @return error code from BACNET_WEBSOCKET_RET enum.
     *  The following error codes can be returned:
     *    BACNET_WEBSOCKET_BAD_PARAM - In a case if some input parameter is
     *                                 incorrect.
     *    BACNET_WEBSOCKET_CLOSED - websocket was already closed by
     *           remote peer or by bws_disconnect() call from other thread.
     *    BACNET_WEBSOCKET_OPERATION_IN_PROGRESS - some other thread has
     *           started disconnect operation on socket h.
     *    BACNET_WEBSOCKET_INVALID_OPERATION - if server was stopped or
     *           server shutdown process is in progress.
     *    BACNET_WEBSOCKET_TIMEDOUT - timeout was elapsed but no data
     *           was received.
     *    BACNET_WEBSOCKET_SUCCESS - operation has succeded.
     *    BACNET_WEBSOCKET_NO_RESOURCES - if some mem allocation has
     *           failed or some allocations of system resources like mutex,
     *           thread, etc.., has failed.
     *    BACNET_WEBSOCKET_BUFFER_TOO_SMALL - received datagram sized
     *                         is larger than provided buffer size.
     *                         In that case only part of websocket datagram
     *                         equal to bufsize is copied into buf.
     */

    BACNET_WEBSOCKET_RET(*bws_recv) (
                           BACNET_WEBSOCKET_PROTOCOL protocol,
                           BACNET_WEBSOCKET_HANDLE h,
                           uint8_t *buf,
                           size_t bufsize,
                           size_t *bytes_received,
                           int timeout);

   /**
     * @brief Blocking bws_recv_from() function receives data from any connected
     *        websocket client. Basically bws_recv() function assumes that you
     *        want to receive a data for a specified websocket handle. As a
     *        result if you want to receive data from several websockets
     *        simultaneously you have to create several threads which may be
     *        waste of resource for embedded systems. That's why
     *        bws_recv_from() can be usefull if you wants to reduce
     *        number of threads in a whole system. In a case of successful
     *        completition ph pointer contains handle of a connection
     *        corresponding to received data. You should also note that it
     *        is not allowed to use bws_recv() and bws_recv_from() calls
     *        simultaneously. In that case one call will failed with
     *        BACNET_WEBSOCKET_INVALID_OPERATION error.
     *
     * @param ph - pointer to a websocket handle for which data is received.
     *             The value is assumed valid only if function returned
     *             BACNET_WEBSOCKET_SUCCESS.
     * @param buf - pointer to a buffer to store received data.
     * @param bufsize - size in bytes of a buffer to store received data.
     * @param bytes_received - pointer to actual number of bytes received.
     * @param timeout - timeout in milliseconds for receive operation.
     *
     * @return error code from BACNET_WEBSOCKET_RET enum.
     *  The following error codes can be returned:
     *    BACNET_WEBSOCKET_BAD_PARAM - In a case if some input parameter is
     *                                 incorrect.
     *    BACNET_WEBSOCKET_CLOSED - there are no any active websockets
     *                              available.
     *    BACNET_WEBSOCKET_INVALID_OPERATION - if server was stopped or
     *           server shutdown process is in progress or there is
     *           at least one pending receive operationn on some socket.
     *    BACNET_WEBSOCKET_TIMEDOUT - timeout was elapsed but no data
     *           was received.
     *    BACNET_WEBSOCKET_SUCCESS - operation has succeded.
     *    BACNET_WEBSOCKET_NO_RESOURCES - if some mem allocation has
     *           failed or some allocations of system resources like mutex,
     *           thread, etc.., has failed.
     *    BACNET_WEBSOCKET_BUFFER_TOO_SMALL - received datagram sized
     *                         is larger than provided buffer size.
     *                         In that case only part of websocket datagram
     *                         equal to bufsize is copied into buf.
     */


    BACNET_WEBSOCKET_RET (*bws_recv_from) (
                            BACNET_WEBSOCKET_PROTOCOL protocol,
                            BACNET_WEBSOCKET_HANDLE* ph,
                            uint8_t *buf,
                            size_t bufsize,
                            size_t *bytes_received,
                            int timeout);

    /**
     * @brief Blocking bws_stop() function shutdowns a websocket server. All
     * opened websocket connections are closed.
     *
     * @return error code from BACNET_WEBSOCKET_RET enum.
     *    The following error codes can be returned:
     *         BACNET_WEBSOCKET_SUCCESS - the server successfully stopped.
     *         BACNET_WEBSOCKET_INVALID_OPERATION - if server was not started or
     *                server shutdown is already in progress.
     */

    BACNET_WEBSOCKET_RET (*bws_stop)(BACNET_WEBSOCKET_PROTOCOL protocol);
} BACNET_WEBSOCKET_SERVER;

BACNET_STACK_EXPORT
BACNET_WEBSOCKET_CLIENT *bws_cli_get(void);

BACNET_STACK_EXPORT
BACNET_WEBSOCKET_SERVER *bws_srv_get(void);

#endif