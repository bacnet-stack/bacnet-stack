/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

/* The module handles sending data out the RS-485 port */
/* and handles receiving data from the RS-485 port. */
/* Customize this file for your specific hardware */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
/*#include "mstp.h" */

/* This file has been customized for use with ATMEGA168 */
//#include "hardware.h"   /***************KAI***********/ // Muss überprüft werden 
//#include "timer.h"      /***************KAI***********/

/* This file has been added for use with ESP32 */ /***************KAI***********/
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#define BUF_SIZE            (1024)

static const char *TAG = "RS485_initalize";

/* Define pins for Uart1 */ /***************KAI***********/
//Define pins vor UART0
#define UART1_TXD           (23)
#define UART1_RXD           (22)
#define UART1_RTS           (18)


/* Timers for turning off the TX,RX LED indications */
static uint8_t LED1_Off_Timer;
static uint8_t LED3_Off_Timer;

/* baud rate */
static uint32_t RS485_Baud = 38400;

/***************KAI***********/
/****************************************************************************
* DESCRIPTION: Initializes the RS485 hardware and variables, and starts in
*              receive mode.
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Initialize(
    void)
{
    uart_config_t uart1_config = {
        .baud_rate = RS485_Baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_2,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_APB,
    };
    ESP_LOGI(TAG, "Start Modem application test and configure UART.");
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart1_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, UART1_TXD, UART1_RXD, UART1_RTS, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(UART_NUM_1, UART_MODE_RS485_HALF_DUPLEX));

    return;
}

/****************************************************************************
* DESCRIPTION: Returns the baud rate that we are currently running at
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
uint32_t RS485_Get_Baud_Rate(
    void)
{
    return RS485_Baud;
}

/****************************************************************************
* DESCRIPTION: Sets the baud rate for the chip USART
* RETURN:      true if valid baud rate
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool RS485_Set_Baud_Rate(
    uint32_t baud)
{
    bool valid = true;

    switch (baud) {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 76800:
        case 115200:
            RS485_Baud = baud;
            break;
        default:
            valid = false;
            break;
    }

    return valid;
}

/****************************************************************************
* DESCRIPTION: Enable or disable the transmitter
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Transmitter_Enable( /***************KAI***********/ // Not Necessary because auto transmitting ist used by chip ... Set with: uart_set_rts(uart_port_t uart_num, int level)
    bool enable)
{
    if (enable) {
       ESP_LOGI(TAG,"enable transmitter");
    } else {
        ESP_LOGI(TAG,"disable transmitter");
    }
}

/****************************************************************************
* DESCRIPTION: Waits on the SilenceTimer for 40 bits.
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Turnaround_Delay(
    void)
{
    ESP_LOGI(TAG,"Goingt into Turnaround Delay");
    ESP_ERROR_CHECK(uart_wait_tx_done(UART_NUM_1,40));  //Wait 40 Ticks??

    RS485_Transmitter_Enable(false);
  
}

/****************************************************************************
* DESCRIPTION: Timers for delaying the LED indicators going off
* RETURN:      none
* ALGORITHM:   none
* NOTES:       expected to be called once a millisecond
*****************************************************************************/
void RS485_LED_Timers(
    void)
{
    if (LED1_Off_Timer) {
        LED1_Off_Timer--;
        if (LED1_Off_Timer == 0) {
            ESP_LOGI(TAG,"RS485_LED_TIMERS: LED1_OFF_Timer");
        }
    }
    if (LED3_Off_Timer) {
        LED3_Off_Timer--;
        if (LED3_Off_Timer == 0) {
            ESP_LOGI(TAG,"RS485_LED_TIMERS: LED3_OFF_Timer");
        }
    }
}

/****************************************************************************
* DESCRIPTION: Turn on the LED, and set the off timer to turn it off
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
static void RS485_LED1_On(
    void)
{
    ESP_LOGI(TAG,"RS485_LED1_On: LED1_ON");
    LED1_Off_Timer = 20;
}

/****************************************************************************
* DESCRIPTION: Turn on the LED, and set the off timer to turn it off
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
static void RS485_LED3_On(
    void)
{
    ESP_LOGI(TAG,"RS485_LED3_On: LED3_ON");
    LED3_Off_Timer = 20;
}

/****************************************************************************
* DESCRIPTION: Send some data and wait until it is sent
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Send_Data(
    uint8_t * buffer,   /* data to send */
    uint16_t nbytes)
{       /* number of bytes of data */
    RS485_LED3_On();
   const int len = nbytes;
    ESP_LOGI(TAG, "RS485_Send_Data: len=%d", len);
        for (int i = 0; i < len; i++) {
                printf("0x%.2X ", (uint8_t)buffer[i]);
                uart_write_bytes(UART_NUM_1, (const char*)&buffer[i], 1);
                 }
    printf("\r \n");
    /* per MSTP spec, sort of */
    // Timer_Silence_Reset();  //Why do I need it ? 
}

/****************************************************************************
* DESCRIPTION: Return true if a framing or overrun error is present
* RETURN:      true if error
* ALGORITHM:   autobaud - if there are a lot of errors, switch baud rate
* NOTES:       Clears any error flags.
*****************************************************************************/
bool RS485_ReceiveError(
    void)
{
    bool ReceiveError = false;
    volatile uint8_t dummy_data;

    /* check for framing error */
#if 0
    ESP_LOGI(TAG,"ReceiveError: if 0 = True?");
   
    // if (BIT_CHECK(UCSR0A, FE0)) {
    //     /* FIXME: how do I clear the error flags? */
    //     BITMASK_CLEAR(UCSR0A, (_BV(FE0) | _BV(DOR0) | _BV(UPE0)));
    //     ReceiveError = true; 

    }
#endif

    if (ReceiveError) {
        ESP_LOGI(TAG,"RS485_ReceiveError: Error= True");
        RS485_LED1_On();
    }

    return ReceiveError;
}

/****************************************************************************
* DESCRIPTION: Return true if data is available
* RETURN:      true if data is available, with the data in the parameter set
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool RS485_DataAvailable(
    uint8_t * data)
{
    bool DataAvailable = false;
    ESP_LOGI(TAG,"RS485_DataAvailable: Look if data is aviable:....");

    uint8_t data_r[128];
    int length_r = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1, (size_t*)&length_r));
    length_r = uart_read_bytes(UART_NUM_1, data_r, length_r, 100);
    /* check for data */
    if (length_r != 0) {
        *data = data_r;
        DataAvailable = true;
        RS485_LED1_On();
    }

    return DataAvailable;
}

// #ifdef TEST_RS485
// int main(
//     void)
// {
//     unsigned i = 0;
//     uint8_t DataRegister;

//     RS485_Set_Baud_Rate(38400);
//     RS485_Initialize();
//     /* receive task */
//     for (;;) {
//         if (RS485_ReceiveError()) {
//             fprintf(stderr, "ERROR ");
//         } else if (RS485_DataAvailable(&DataRegister)) {
//             fprintf(stderr, "%02X ", DataRegister);
//         }
//     }
// }
// #endif /* TEST_RS485 */

