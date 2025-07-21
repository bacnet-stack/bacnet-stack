/**
 * @file
 * @author Andriy Sukhynyuk, Vasyl Tkhir, Andriy Ivasiv
 * @date 2012
 * @brief Datalink for MS/TP module
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include "mstpmodule.h"
#include "bacnet/bacint.h"
#include "dlmstp_port.h"

void *dl_mstp_thread(void *pArgs)
{
    ROUTER_PORT *port = (ROUTER_PORT *)pArgs;
    struct mstp_port_struct_t mstp_port = { 0 };
    volatile SHARED_MSTP_DATA shared_port_data = { 0 };
    uint16_t pdu_len;
    uint8_t shutdown = 0;

    shared_port_data.MSTP_Packets = 0;
    shared_port_data.RS485_Handle = -1;
    shared_port_data.RS485MOD = 0;

    switch (port->params.mstp_params.databits) {
        case 5:
            shared_port_data.RS485MOD = CS5;
            break;
        case 6:
            shared_port_data.RS485MOD = CS6;
            break;
        case 7:
            shared_port_data.RS485MOD = CS7;
            break;
        default:
            shared_port_data.RS485MOD = CS8;
            break;
    }

    switch (port->params.mstp_params.parity) {
        case PARITY_EVEN:
            shared_port_data.RS485MOD |= PARENB;
            break;
        case PARITY_ODD:
            shared_port_data.RS485MOD |= PARENB | PARODD;
            break;
        default:
            break;
    }

    if (port->params.mstp_params.stopbits == 2) {
        shared_port_data.RS485MOD |= CSTOPB;
    }

    mstp_port.UserData = (void *)&shared_port_data;
    dlmstp_set_baud_rate(&mstp_port, port->params.mstp_params.baudrate);
    dlmstp_set_mac_address(&mstp_port, port->route_info.mac[0]);
    dlmstp_set_max_info_frames(&mstp_port, port->params.mstp_params.max_frames);
    dlmstp_set_max_master(&mstp_port, port->params.mstp_params.max_master);
    if (!dlmstp_init(&mstp_port, port->iface)) {
        printf("MSTP %s init failed. Stop.\n", port->iface);
    }
    mstp_port.Treply_timeout = 260;
    mstp_port.Tusage_timeout = 30;

    port->port_id = create_msgbox();
    if (port->port_id == INVALID_MSGBOX_ID) {
        port->state = INIT_FAILED;
        return NULL;
    }

    port->state = RUNNING;

    while (!shutdown) {
        /* message loop */
        BACMSG msg_storage, *bacmsg;
        MSG_DATA *msg_data;

        bacmsg = recv_from_msgbox(port->port_id, &msg_storage, IPC_NOWAIT);

        if (bacmsg) {
            switch (bacmsg->type) {
                case DATA:
                    msg_data = (MSG_DATA *)bacmsg->data;

                    if (msg_data->dest.net == BACNET_BROADCAST_NETWORK) {
                        dlmstp_get_broadcast_address(&(msg_data->dest));
                    } else {
                        msg_data->dest.mac[0] = msg_data->dest.adr[0];
                        msg_data->dest.mac_len = 1;
                    }

                    dlmstp_send_pdu(
                        &mstp_port, &(msg_data->dest), msg_data->pdu,
                        msg_data->pdu_len);

                    check_data(msg_data);

                    break;
                case SERVICE:
                    switch (bacmsg->subtype) {
                        case SHUTDOWN:
                            shutdown = 1;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    continue;
                    break;
            }
        } else {
            pdu_len = dlmstp_receive(&mstp_port, NULL, NULL, 0, 5);

            if (pdu_len > 0) {
                msg_data = (MSG_DATA *)malloc(sizeof(MSG_DATA));
                memmove(
                    &(msg_data->src),
                    (const void *)&(shared_port_data.Receive_Packet.address),
                    sizeof(shared_port_data.Receive_Packet.address));
                msg_data->src.adr[0] = msg_data->src.mac[0];
                msg_data->src.len = 1;
                msg_data->pdu = (uint8_t *)malloc(pdu_len);
                memmove(
                    msg_data->pdu,
                    (const void *)&(shared_port_data.Receive_Packet.pdu),
                    pdu_len);
                msg_data->pdu_len = pdu_len;

                msg_storage.type = DATA;
                msg_storage.subtype = (MSGSUBTYPE)0;
                msg_storage.origin = port->port_id;
                msg_storage.data = msg_data;

                if (!send_to_msgbox(port->main_id, &msg_storage)) {
                    free_data(msg_data);
                }
            }
        }
    }

    dlmstp_cleanup(&mstp_port);
    port->state = FINISHED;

    return NULL;
}
