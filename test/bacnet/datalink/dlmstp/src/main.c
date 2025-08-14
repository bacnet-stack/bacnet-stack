/**
 * @file
 * @brief Unit test for a basic BACnet MS/TP Datalink
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/datalink/mstp.h>
#include <bacnet/datalink/dlmstp.h>
#include <bacnet/basic/sys/bytes.h>
#include <mstp-rs485.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */
static uint8_t RS485_Rx_Buffer[512];
static uint8_t RS485_Tx_Buffer[512];
static struct mstp_port_struct_t MSTP_Port;
static struct dlmstp_user_data_t MSTP_User;
static struct dlmstp_rs485_driver MSTP_RS485_Driver = {
    .init = MSTP_RS485_Init,
    .send = MSTP_RS485_Send,
    .read = MSTP_RS485_Read,
    .transmitting = MSTP_RS485_Transmitting,
    .baud_rate = MSTP_RS485_Baud_Rate,
    .baud_rate_set = MSTP_RS485_Baud_Rate_Set,
    .silence_milliseconds = MSTP_RS485_Silence_Milliseconds,
    .silence_reset = MSTP_RS485_Silence_Reset
};

/**
 * @brief Test the basic operation of the MS/TP Datalink
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(dlsmtp_tests, test_MSTP_Datalink)
#else
static void test_MSTP_Datalink(void)
#endif
{
    bool status = false;
    struct dlmstp_statistics test_stats;
    uint32_t test_baudrate;
    uint8_t test_mac_address;
    BACNET_ADDRESS test_address;
    uint8_t test_frames;
    uint16_t test_length;
    uint8_t test_data[10] = { PDU_TYPE_ABORT, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    BACNET_NPDU_DATA test_npdu_data = { 0 };

    /* error handling before port is initialized */
    test_length = dlmstp_send_pdu(NULL, NULL, 0, 0);
    test_length = MSTP_Get_Send(NULL, 0);
    zassert_equal(test_length, 0, NULL);
    test_length = MSTP_Get_Reply(NULL, 0);
    zassert_equal(test_length, 0, NULL);
    MSTP_Send_Frame(NULL, NULL, 0);
    test_length = dlmstp_receive(&test_address, NULL, 0, 0);
    zassert_equal(test_length, 0, NULL);
    test_mac_address = dlmstp_mac_address();
    zassert_equal(test_mac_address, 0, NULL);
    test_frames = dlmstp_max_info_frames();
    zassert_equal(test_frames, 0, NULL);
    test_mac_address = dlmstp_max_master();
    zassert_equal(test_mac_address, 0, NULL);
    status = dlmstp_sole_master();
    zassert_equal(status, false, NULL);
    status = dlmstp_slave_mode_enabled_set(true);
    zassert_equal(status, false, NULL);
    status = dlmstp_slave_mode_enabled();
    zassert_equal(status, false, NULL);
    status = dlmstp_zero_config_enabled();
    zassert_equal(status, false, NULL);
    test_mac_address = dlmstp_zero_config_preferred_station();
    zassert_equal(test_mac_address, 0, NULL);
    dlmstp_zero_config_preferred_station_set(0);
    dlmstp_set_baud_rate(38400);
    test_baudrate = dlmstp_baud_rate();
    zassert_equal(test_baudrate, 0, NULL);
    dlmstp_set_frame_rx_complete_callback(NULL);
    dlmstp_set_invalid_frame_rx_complete_callback(NULL);
    dlmstp_set_frame_rx_start_callback(NULL);
    dlmstp_fill_statistics(NULL);
    dlmstp_reset_statistics();
    /* initialize */
    MSTP_User.RS485_Driver = &MSTP_RS485_Driver;
    MSTP_Port.UserData = &MSTP_User;
    MSTP_Port.InputBuffer = RS485_Rx_Buffer;
    MSTP_Port.InputBufferSize = sizeof(RS485_Rx_Buffer);
    MSTP_Port.OutputBuffer = RS485_Tx_Buffer;
    MSTP_Port.OutputBufferSize = sizeof(RS485_Tx_Buffer);
    ztest_expect_value(MSTP_Init, mstp_port, &MSTP_Port);
    status = dlmstp_init((char *)&MSTP_Port);
    zassert_true(status, NULL);
    zassert_true(MSTP_User.Initialized, NULL);
    zassert_equal(
        Ringbuf_Size(&MSTP_User.PDU_Queue), DLMSTP_MAX_INFO_FRAMES, NULL);
    zassert_equal(
        Ringbuf_Data_Size(&MSTP_User.PDU_Queue), sizeof(struct dlmstp_packet),
        NULL);
    zassert_equal(dlmstp_send_pdu_queue_empty(), true, NULL);
    zassert_equal(dlmstp_send_pdu_queue_full(), false, NULL);
    ztest_returns_value(MSTP_RS485_Silence_Milliseconds, 0);
    zassert_equal(dlmstp_silence_milliseconds(&MSTP_Port), 0, NULL);
    dlmstp_silence_reset(&MSTP_Port);
    ztest_returns_value(MSTP_RS485_Silence_Milliseconds, 0);
    zassert_equal(dlmstp_silence_milliseconds(&MSTP_Port), 0, NULL);
    zassert_equal(dlmstp_max_master_limit(), DLMSTP_MAX_MASTER, NULL);
    zassert_equal(dlmstp_max_info_frames_limit(), DLMSTP_MAX_INFO_FRAMES, NULL);
    dlmstp_fill_statistics(NULL);
    dlmstp_fill_statistics(&test_stats);
    zassert_equal(
        test_stats.transmit_frame_counter,
        MSTP_User.Statistics.transmit_frame_counter, NULL);
    zassert_equal(
        test_stats.receive_valid_frame_counter,
        MSTP_User.Statistics.receive_valid_frame_counter, NULL);
    zassert_equal(
        test_stats.receive_valid_frame_not_for_us_counter,
        MSTP_User.Statistics.receive_valid_frame_not_for_us_counter, NULL);
    zassert_equal(
        test_stats.receive_invalid_frame_counter,
        MSTP_User.Statistics.receive_invalid_frame_counter, NULL);
    zassert_equal(
        test_stats.transmit_pdu_counter,
        MSTP_User.Statistics.transmit_pdu_counter, NULL);
    zassert_equal(
        test_stats.receive_pdu_counter,
        MSTP_User.Statistics.receive_pdu_counter, NULL);
    dlmstp_reset_statistics();
    dlmstp_set_frame_rx_complete_callback(NULL);
    dlmstp_set_invalid_frame_rx_complete_callback(NULL);
    dlmstp_set_frame_rx_start_callback(NULL);
    dlmstp_get_broadcast_address(&test_address);
    zassert_equal(test_address.mac_len, 1, NULL);
    zassert_equal(test_address.mac[0], MSTP_BROADCAST_ADDRESS, NULL);
    zassert_equal(test_address.net, BACNET_BROADCAST_NETWORK, NULL);
    zassert_equal(test_address.len, 0, NULL);
    dlmstp_get_my_address(&test_address);
    zassert_equal(test_address.mac_len, 1, NULL);
    zassert_equal(test_address.mac[0], MSTP_Port.This_Station, NULL);
    zassert_equal(test_address.net, 0, NULL);
    zassert_equal(test_address.len, 0, NULL);
    test_mac_address = dlmstp_max_master();
    zassert_equal(test_mac_address, MSTP_Port.Nmax_master, NULL);
    dlmstp_set_max_master(10);
    test_mac_address = dlmstp_max_master();
    zassert_equal(test_mac_address, 10, NULL);
    test_frames = dlmstp_max_info_frames();
    zassert_equal(test_frames, MSTP_Port.Nmax_info_frames, NULL);
    dlmstp_set_max_info_frames(10);
    test_frames = dlmstp_max_info_frames();
    zassert_equal(test_frames, 10, NULL);
    test_mac_address = dlmstp_mac_address();
    zassert_equal(test_mac_address, MSTP_Port.This_Station, NULL);
    dlmstp_set_mac_address(10);
    test_mac_address = dlmstp_mac_address();
    zassert_equal(test_mac_address, 10, NULL);
    dlmstp_fill_bacnet_address(&test_address, 10);
    zassert_equal(test_address.mac_len, 1, NULL);
    zassert_equal(test_address.mac[0], 10, NULL);
    zassert_equal(test_address.net, 0, NULL);
    zassert_equal(test_address.len, 0, NULL);
    dlmstp_fill_bacnet_address(&test_address, MSTP_BROADCAST_ADDRESS);
    zassert_equal(test_address.mac_len, 0, NULL);
    zassert_equal(test_address.mac[0], 0, NULL);
    zassert_equal(test_address.net, 0, NULL);
    zassert_equal(test_address.len, 0, NULL);
    ztest_returns_value(MSTP_RS485_Baud_Rate, 38400);
    test_baudrate = dlmstp_baud_rate();
    zassert_equal(test_baudrate, 38400, NULL);
    ztest_expect_value(MSTP_RS485_Baud_Rate_Set, baud, 19200);
    ztest_returns_value(MSTP_RS485_Baud_Rate_Set, true);
    dlmstp_set_baud_rate(19200);
    dlmstp_zero_config_preferred_station_set(65);
    test_mac_address = dlmstp_zero_config_preferred_station();
    zassert_equal(test_mac_address, 65, NULL);
    dlmstp_zero_config_enabled_set(true);
    status = dlmstp_zero_config_enabled();
    zassert_equal(status, true, NULL);
    status = dlmstp_slave_mode_enabled_set(true);
    zassert_equal(status, true, NULL);
    status = dlmstp_slave_mode_enabled();
    zassert_equal(status, true, NULL);
    status = dlmstp_sole_master();
    zassert_equal(status, MSTP_Port.SoleMaster, NULL);
    /* various test cases */
    ztest_returns_value(MSTP_RS485_Transmitting, true);
    test_length = dlmstp_receive(&test_address, NULL, 0, 0);
    zassert_equal(test_length, 0, NULL);
    /* fixme: dlmstp_receive() calls driver with local var pointer
       and zmock does not seem to have a way to test that. */
    test_length = MSTP_Put_Receive(&MSTP_Port);
    zassert_equal(test_length, MSTP_Port.DataLength, NULL);
    zassert_equal(MSTP_User.ReceivePacketPending, true, NULL);
    ztest_expect_data(MSTP_RS485_Send, payload, test_data);
    MSTP_Send_Frame(&MSTP_Port, test_data, sizeof(test_data));
    zassert_equal(MSTP_User.Statistics.transmit_frame_counter, 1, NULL);
    test_length = MSTP_Get_Reply(&MSTP_Port, 0);
    zassert_equal(test_length, 0, NULL);
    test_npdu_data.data_expecting_reply = false;
    test_length = dlmstp_send_pdu(
        &test_address, &test_npdu_data, test_data, sizeof(test_data));
    zassert_equal(
        test_length, sizeof(test_data), "dlmstp_send_pdu() length=%d",
        test_length);
    ztest_expect_value(MSTP_Create_Frame, buffer, &MSTP_Port.OutputBuffer[0]);
    ztest_expect_value(
        MSTP_Create_Frame, buffer_len, MSTP_Port.OutputBufferSize);
    ztest_expect_value(
        MSTP_Create_Frame, frame_type,
        FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY);
    ztest_expect_value(MSTP_Create_Frame, destination, MSTP_BROADCAST_ADDRESS);
    ztest_expect_value(MSTP_Create_Frame, source, MSTP_Port.This_Station);
    ztest_expect_data(MSTP_Create_Frame, data, test_data);
    ztest_returns_value(
        MSTP_Create_Frame, sizeof(test_data) + DLMSTP_HEADER_MAX);
    test_length = MSTP_Get_Send(&MSTP_Port, 0);
    zassert_equal(
        test_length, sizeof(test_data) + DLMSTP_HEADER_MAX,
        "MSTP_Get_Send() length=%d", test_length);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(dlmstp_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(dlmstp_tests, ztest_unit_test(test_MSTP_Datalink));

    ztest_run_test_suite(dlmstp_tests);
}
#endif
