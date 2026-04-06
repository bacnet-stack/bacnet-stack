/**
 * @file
 * @brief BACnet MS/TP datalink environment setup for the PlatformIO ESP32 port
 * @author Kato Gangstad
 */

#include "dlenv.h"

#include "bacnet/datalink/dlmstp.h"
#include "rs485.h"

volatile struct mstp_port_struct_t MSTP_Port;

static struct dlmstp_user_data_t MSTP_User_Data;
static uint8_t Input_Buffer[DLMSTP_MPDU_MAX];
static uint8_t Output_Buffer[DLMSTP_MPDU_MAX];

static struct dlmstp_rs485_driver RS485_Driver = {
    .init = rs485_init,
    .send = rs485_bytes_send,
    .read = rs485_byte_available,
    .transmitting = rs485_rts_enabled,
    .baud_rate = rs485_baud_rate,
    .baud_rate_set = rs485_baud_rate_set,
    .silence_milliseconds = rs485_silence_milliseconds,
    .silence_reset = rs485_silence_reset
};

/**
 * @brief Initialize the BACnet MS/TP datalink environment
 * @param mac_address local MS/TP MAC address
 * @return true if the datalink initialized successfully
 */
bool m5_dlenv_init(uint8_t mac_address)
{
    RS485_Driver.init();

    MSTP_Port.Nmax_info_frames = BACNET_MSTP_MAX_INFO_FRAMES;
    MSTP_Port.Nmax_master = BACNET_MSTP_MAX_MASTER;

    MSTP_Port.InputBuffer = Input_Buffer;
    MSTP_Port.InputBufferSize = sizeof(Input_Buffer);
    MSTP_Port.OutputBuffer = Output_Buffer;
    MSTP_Port.OutputBufferSize = sizeof(Output_Buffer);

    MSTP_Port.ZeroConfigEnabled = false;
    MSTP_Port.SlaveNodeEnabled = false;
    MSTP_Port.CheckAutoBaud = false;

    MSTP_Zero_Config_UUID_Init((struct mstp_port_struct_t *)&MSTP_Port);

    MSTP_User_Data.RS485_Driver = &RS485_Driver;
    MSTP_Port.UserData = &MSTP_User_Data;

    if (!dlmstp_init((char *)&MSTP_Port)) {
        return false;
    }

    dlmstp_set_mac_address(mac_address);
    dlmstp_set_baud_rate(RS485_BAUD_RATE);
    dlmstp_set_max_master(BACNET_MSTP_MAX_MASTER);
    dlmstp_set_max_info_frames(BACNET_MSTP_MAX_INFO_FRAMES);

    return true;
}

/**
 * @brief Run any periodic MS/TP maintenance hooks
 * @param seconds elapsed seconds since the last call
 */
void dlenv_maintenance_timer(uint16_t seconds)
{
    (void)seconds;
}

/**
 * @brief Shut down the MS/TP datalink environment
 */
void dlenv_cleanup(void)
{
}
