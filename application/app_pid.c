/******************************************************************************************
Copyright 2020 Konstantinos Zafeiropoulos. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors 
may be used to endorse or promote products derived from this software without 
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************************
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "osal.h"
#include "resmgmt.h"
#include "hw_cpm.h"
#include "hw_gpio.h"
#include "hw_watchdog.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"

#include "hw_gpio.h"
#include "hw_wkup.h"

#include "ad_spi.h"

#include "platform_devices.h"
#include "drv_canfdspi_api.h"
#include "drv_canfdspi_register.h"

#define DRV_CANFDSPI_INDEX_1    1
#define APP_TX_QUEUE            CAN_FIFO_CH0
#define APP_TX_OBD_FUEL_FIFO    CAN_FIFO_CH1
#define APP_TX_OBD_PID_FIFO     CAN_FIFO_CH2
#define APP_RX_FIFO             CAN_FIFO_CH3


#define OBD_PID_REQ    0x7E0
#define OBD_PID_RSP    0x7E8

#define PID_ENGINE_LOAD         0x04
#define PID_COOLANT_TEMP        0x05
#define PID_RPM                 0x0C
#define PID_SPEED               0x0D
#define PID_THROTTLE            0x11
#define PID_TIME                0x1F
#define PID_FUEL                0x2F
#define PID_BATTERY             0x42
#define PID_AIR_TEMP            0x46
#define PID_OIL_TEMP            0x5C // Not at the supported pid list
#define PID_FUEL_RATE           0x9D
#define PID_ODOMETER            0x31

#define PID_DRV_TRQ             0x61
#define PID_ACT_TRQ             0x62
#define PID_REF_TRQ             0x63


float engine_load = 0;
uint8_t  coolant_temp = 0;
uint8_t  air_temp = 0;
uint8_t  oil_temp = 0;
uint32_t rpm = 0;
uint8_t  speed = 0;
float throttle = 0;
uint32_t time = 0;
float fuel = 0;
uint32_t fuel_rate = 0;
float battery_flt = 0;
uint32_t odometer = 1;

CAN_RX_MSGOBJ rxheader2;
uint8_t rxdata2[MAX_DATA_BYTES];

CAN_TX_MSGOBJ txPIDHeader;
uint8_t pid_req_data[CAN_DLC_8];

OS_TIMER pid_timer;

bool dk_hw_int2_detected(void);
int ee_printf(const char *fmt, ...);
#define printf ee_printf

void pid_timer_cb(OS_TIMER pxTime);

void pid_timer_cb_exec()
{
        pid_req_data[2] = PID_COOLANT_TEMP;
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txPIDHeader, pid_req_data, CAN_DLC_8, true);

        pid_req_data[2] = PID_AIR_TEMP;
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txPIDHeader, pid_req_data, CAN_DLC_8, true);

        pid_req_data[2] = PID_FUEL;
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txPIDHeader, pid_req_data, CAN_DLC_8, true);

        pid_req_data[2] = PID_BATTERY;
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txPIDHeader, pid_req_data, CAN_DLC_8, true);

       // pid_req_data[2] = PID_OIL_TEMP;
       // DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txPIDHeader, pid_req_data, CAN_DLC_8, true);

       // pid_req_data[2] = PID_RPM;
       // DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txPIDHeader, pid_req_data, CAN_DLC_8, true);

       // pid_req_data[2] = PID_THROTTLE;
       // DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txPIDHeader, pid_req_data, CAN_DLC_8, true);

       // pid_req_data[2] = PID_TIME;
       // DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txPIDHeader, pid_req_data, CAN_DLC_8, true);

       // pid_req_data[2] = PID_ODOMETER;
       // DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txPIDHeader, pid_req_data, CAN_DLC_8, true);

       // pid_req_data[2] = PID_FUEL_RATE;
       // DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txPIDHeader, pid_req_data, CAN_DLC_8, true);

       // pid_req_data[2] = PID_ENGINE_LOAD;
       // DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txPIDHeader, pid_req_data, CAN_DLC_8, true);

       // pid_req_data[2] = PID_SPEED;
       // DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txPIDHeader, pid_req_data, CAN_DLC_8, true);

}

void APP_CANFDSPI2_Init()
{
        pid_timer = OS_TIMER_CREATE("pid",   OS_MS_2_TICKS(10000),  pdTRUE, NULL, pid_timer_cb);

        txPIDHeader.word[0] = 0;
        txPIDHeader.word[1] = 0;
        txPIDHeader.bF.id.SID = OBD_PID_REQ;
        txPIDHeader.bF.ctrl.DLC = 8;
        memset(pid_req_data, 0, CAN_DLC_8);
        pid_req_data[0] = 0x02;
        pid_req_data[1] = 0x01;
        OS_TIMER_RESET(pid_timer, OS_TIMER_FOREVER);

        // Reset device
        DRV_CANFDSPI_Reset(DRV_CANFDSPI_INDEX_1);

        // Enable ECC and initialize RAM
        DRV_CANFDSPI_EccEnable(DRV_CANFDSPI_INDEX_1);
        DRV_CANFDSPI_RamInit(DRV_CANFDSPI_INDEX_1, 0xff);

        // Configure device
        CAN_CONFIG config;
        memset(&config, 0, sizeof(config));
        config.TXQEnable = 1;
        config.TxBandWidthSharing = 0xF; // Delay between two consecutive transmissions (in arbitration bit times)
                                         // 4096 bit = 8msec
        DRV_CANFDSPI_Configure(DRV_CANFDSPI_INDEX_1, &config);

        // Regarding RAM calculation: FIFO_size * (DLC + ~10bytes overhead)
        // Currently TBD
        CAN_TX_QUEUE_CONFIG qconfig;
        DRV_CANFDSPI_TransmitQueueConfigureObjectReset(&qconfig);
        qconfig.FifoSize = 8;
        qconfig.PayLoadSize = CAN_PLSIZE_8;
        qconfig.TxPriority = 1;
        DRV_CANFDSPI_TransmitQueueConfigure(DRV_CANFDSPI_INDEX_1, &qconfig);

        // Setup TX FIFO
        CAN_TX_FIFO_CONFIG txConfig;
        DRV_CANFDSPI_TransmitChannelConfigureObjectReset(&txConfig);
        txConfig.FifoSize = 31;
        txConfig.PayLoadSize = CAN_PLSIZE_8;
        txConfig.TxPriority = 2;
        DRV_CANFDSPI_TransmitChannelConfigure(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_FUEL_FIFO, &txConfig);

        // Setup TX FIFO
        DRV_CANFDSPI_TransmitChannelConfigureObjectReset(&txConfig);
        txConfig.FifoSize = 31;
        txConfig.PayLoadSize = CAN_PLSIZE_8;
        txConfig.TxPriority = 3;
        DRV_CANFDSPI_TransmitChannelConfigure(DRV_CANFDSPI_INDEX_1, APP_TX_OBD_PID_FIFO, &txConfig);

        // Setup RX FIFO
        CAN_RX_FIFO_CONFIG rxConfig;
        DRV_CANFDSPI_ReceiveChannelConfigureObjectReset(&rxConfig);
        rxConfig.FifoSize = 31;
        rxConfig.PayLoadSize = CAN_PLSIZE_8;

        DRV_CANFDSPI_ReceiveChannelConfigure(DRV_CANFDSPI_INDEX_1, APP_RX_FIFO, &rxConfig);

        // Setup RX Filter
        REG_CiFLTOBJ fObj;
        REG_CiMASK mObj;

        fObj.word = 0;
        fObj.bF.SID = OBD_PID_RSP;
        fObj.bF.EID = 0x00;
        fObj.bF.EXIDE = 0;

        DRV_CANFDSPI_FilterObjectConfigure(DRV_CANFDSPI_INDEX_1, CAN_FILTER0, &fObj.bF);


        // Setup RX Mask
        mObj.word = 0;
        mObj.bF.MSID = 0x7FF;
        mObj.bF.MIDE = 1; // Only allow standard IDs
        mObj.bF.MEID = 0x0;
        DRV_CANFDSPI_FilterMaskConfigure(DRV_CANFDSPI_INDEX_1, CAN_FILTER0, &mObj.bF);

        // Link FIFO and Filter
        DRV_CANFDSPI_FilterToFifoLink(DRV_CANFDSPI_INDEX_1, CAN_FILTER0, APP_RX_FIFO, true);

        // Setup Bit Time
        DRV_CANFDSPI_BitTimeConfigure(DRV_CANFDSPI_INDEX_1, CAN_500K_2M, CAN_SSP_MODE_AUTO, CAN_SYSCLK_40M);

        // Setup Transmit and Receive Interrupts
        DRV_CANFDSPI_GpioModeConfigure(DRV_CANFDSPI_INDEX_1, GPIO_MODE_INT, GPIO_MODE_INT);

        // DRV_CANFDSPI_TransmitChannelEventEnable(DRV_CANFDSPI_INDEX_1, APP_TX_FIFO, CAN_TX_FIFO_NOT_FULL_EVENT);
        DRV_CANFDSPI_ReceiveChannelEventEnable(DRV_CANFDSPI_INDEX_1, APP_RX_FIFO, CAN_RX_FIFO_NOT_EMPTY_EVENT);
        //DRV_CANFDSPI_ModuleEventEnable(DRV_CANFDSPI_INDEX_1, CAN_TX_EVENT | CAN_RX_EVENT);
        DRV_CANFDSPI_ModuleEventEnable(DRV_CANFDSPI_INDEX_1, CAN_RX_EVENT);

        // Select Normal Mode
        DRV_CANFDSPI_OperationModeSelect(DRV_CANFDSPI_INDEX_1, CAN_CLASSIC_MODE);
        // DRV_CANFDSPI_OperationModeSelect(DRV_CANFDSPI_INDEX_1, CAN_NORMAL_MODE);

        DRV_CANFDSPI_ModuleEventClear(DRV_CANFDSPI_INDEX_1, CAN_ALL_EVENTS);

        if (0) { // SPI sanity test
                uint8_t txd[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
                uint8_t rxd[32];
                DRV_CANFDSPI_ReadByteArray(DRV_CANFDSPI_INDEX_1, cREGADDR_DEVID, rxd, 4);
                OS_DELAY(OS_MS_2_TICKS(10));

                DRV_CANFDSPI_WriteByteArray(DRV_CANFDSPI_INDEX_1, cRAMADDR_START, txd, 16);
                DRV_CANFDSPI_ReadByteArray(DRV_CANFDSPI_INDEX_1, cRAMADDR_START, rxd, 16);
                printf("RAM2 RXD: %d,%d,%d,%d, %d,%d,%d,%d \r\n",rxd[0],rxd[1],rxd[2],rxd[3], rxd[4],rxd[5],rxd[6],rxd[7]);
                OS_DELAY(OS_MS_2_TICKS(10));

                DRV_CANFDSPI_WriteByteArray(DRV_CANFDSPI_INDEX_1, cREGADDR_CiFLTOBJ, txd, 16);
                DRV_CANFDSPI_ReadByteArray(DRV_CANFDSPI_INDEX_1, cREGADDR_CiFLTOBJ, rxd, 16);
                printf("REG2 RXD: %d,%d,%d,%d, %d,%d,%d,%d\r\n",rxd[0],rxd[1],rxd[2],rxd[3], rxd[4],rxd[5],rxd[6],rxd[7]);
                OS_DELAY(OS_MS_2_TICKS(10));
        }
}

// GROUP1: LOAD,COOLANT,FUEL,AIR,OIL,BATT

void proccess_rx2_data()
{
        if (rxheader2.bF.id.SID == OBD_PID_RSP) {
                if (rxdata2[2] == PID_COOLANT_TEMP) { // OK
                        coolant_temp = rxdata2[3] - 40;
                        return;
                }
                if (rxdata2[2] == PID_AIR_TEMP) { //OK
                        air_temp = rxdata2[3] - 40;
                        return;
                }
                if (rxdata2[2] == PID_FUEL) { //OK
                        fuel = ((float)rxdata2[3]) / 2.55;
                        return;
                }
                if (rxdata2[2] == PID_BATTERY) { //OK
                        battery_flt = (((float)(rxdata2[3] << 8)) + (float)rxdata2[4]) / 1000;
                        printf("%.1fV %d/%d/%dC %.0f%\r\n",
                                battery_flt, (int)air_temp, (int)coolant_temp, (int)oil_temp,  fuel);
                        return;
                }
//                if (rxdata2[2] == PID_ENGINE_LOAD) { //OK
//                        engine_load = ((float)rxdata2[3]) / 2.55;
//                        return;
//                }
//                if (rxdata2[2] == PID_RPM) { //OK
//                        rpm = (((uint32_t)(rxdata2[3]) << 8) + rxdata2[4]) >> 2;
//                        return;
//                }
//                if (rxdata2[2] == PID_SPEED) { //OK
//                        speed = rxdata2[3];
//                        return;
//                }
//                if (rxdata2[2] == PID_THROTTLE) { //OK
//                        throttle = ((float)rxdata2[3]) / 2.55;
//                        return;
//                }
//                if (rxdata2[2] == PID_TIME) { //OK
//                        time = (((uint32_t)rxdata2[3]) << 8) + rxdata2[4];
//                        return;
//                }
//
//                if (rxdata2[2] == PID_ODOMETER) { //  OK
//                        odometer = (((uint32_t)rxdata2[3]) << 8) + rxdata2[4];
//                        return;
//                }
//                if (rxdata2[2] == PID_FUEL_RATE) { // OK
//                        fuel_rate = (((uint32_t)rxdata2[3]) << 24) + (((uint32_t)rxdata2[4])<< 16) +
//                                (((uint32_t)rxdata2[5]) << 8) + (uint32_t)rxdata2[6];
//                        return;
//                }
        }
}

void handle_spi2_interrupt(){
        while (dk_hw_int2_detected()) { // Check if more interrupts are still pending, need to serve all in order for
                                       // the pin get high and trigger again
                DRV_CANFDSPI_ReadByteArray(DRV_CANFDSPI_INDEX_1, cREGADDR_CiVEC, rxdata2, 4 * 6);
                if (0) {
                        printf("CiVEC    0x%02X%02X%02X%02X\r\n", rxdata2[3], rxdata2[2], rxdata2[1], rxdata2[0]);
                        printf("CiINT    0x%02X%02X%02X%02X\r\n", rxdata2[7], rxdata2[6], rxdata2[5], rxdata2[4]);
                        printf("CiRXIF   0x%02X%02X%02X%02X\r\n", rxdata2[11], rxdata2[10], rxdata2[9], rxdata2[8]);
                        printf("CiRXOVIF 0x%02X%02X%02X%02X\r\n", rxdata2[15], rxdata2[14], rxdata2[13],rxdata2[12]);
                        printf("CiTXIF   0x%02X%02X%02X%02X\r\n", rxdata2[19], rxdata2[18], rxdata2[17],rxdata2[16]);
                }
                CAN_RX_FIFO_EVENT rxFlags;

                uint8_t rx_channel = rxdata2[3] & 0x1F; // This is CiVEC RXCODE byte
                if (rx_channel) {
                        DRV_CANFDSPI_ReceiveChannelEventGet(DRV_CANFDSPI_INDEX_1, rx_channel, &rxFlags);
                        if (rxFlags & CAN_RX_FIFO_NOT_EMPTY_EVENT) {
                                DRV_CANFDSPI_ReceiveMessageGet(DRV_CANFDSPI_INDEX_1, rx_channel, &rxheader2, rxdata2, CAN_DLC_8);
                                proccess_rx2_data();
                        }
                        //TODO: Need to handle other events
                }
                //TODO: Handle TXCODE and other INFO interrupts
        }
}

