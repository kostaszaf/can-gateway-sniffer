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

#define WHEEL_BUTTON_CAN_ID           0x5BF
#define DRIVE_SELECT_STATUS_CAN_ID    0x386
#define TEMP_PID_CAN_ID               0x640
#define CONSOLE_BUTTON_CAN_ID         0x65A
#define MIRROR_KNOB_CAN_ID            0x6AE
#define WINDOW_CAN_ID                 0x185
#define VC_FIELD_CAN_ID               0x17333211

#define WHEEL_BUTTON_ASTERISK         0x21
#define WHEEL_BUTTON_OK               0x08
#define WHEEL_BUTTON_VIEW             0x23

bool trigger_start_stop = 0;
bool trigger_drive_select_efficiency = 0;
bool trigger_drive_select_dynamic = 0;
bool trigger_drive_select_auto = 0;

//bool alarm_activated = 0;
bool knob_left_long_press = 0;
//bool knob_right_long_press = 0;
bool igla_valet_mode = 0;

uint8_t drive_select_efficiency_clicks = 0;
uint8_t drive_select_dynamic_clicks = 0;
uint8_t drive_select_auto_clicks = 0;

uint32_t last_console_button_stamp = 0;

extern float engine_load;
extern uint8_t  coolant_temp;
extern uint8_t  air_temp;
extern uint8_t  oil_temp;
extern uint32_t rpm;
extern uint8_t  speed;
extern float throttle;
extern uint32_t time;
extern float fuel;
extern uint32_t fuel_rate;
extern float battery_flt;
extern uint32_t odometer;

//#define SCREEN_0        0x0
//#define SCREEN_1        0x1
//#define SCREEN_EMPTY    0xFF
//uint8_t vc_screen_num = SCREEN_EMPTY;

#define DRV_CANFDSPI_INDEX_0    0
#define APP_TX_QUEUE            CAN_FIFO_CH0
#define APP_TX_PRINT_FIFO       CAN_FIFO_CH1
#define APP_IGLA_TX_FIFO        CAN_FIFO_CH2
#define APP_RX_FIFO             CAN_FIFO_CH3

#define VC_PRINT_MAX_LEN        21

#define IGLA_IDLE_CMD  0
#define IGLA_LEFT_CMD  1
#define IGLA_RIGHT_CMD 2
#define IGLA_MAX_CMD   3

char vcprint[VC_PRINT_MAX_LEN];
uint32_t vc_overwrite_once = 0;

#undef USE_IGLA_WHEEL
#define USE_IGLA_MIRROR_KNOB

#ifdef USE_IGLA_WHEEL
#ifdef USE_IGLA_MIRROR_KNOB
#error "Cannot have both USE_IGLA_WHEEL and USE_IGLA_MIRROR_KNOB defined"
#endif
#endif

uint8_t txd[MAX_DATA_BYTES];
CAN_TX_MSGOBJ txConsoleButtonHeader, txVCprintHeader, txIglaCmdHeader, txIglaValetCmdHeader;
uint8_t txConsoleButtonData[CAN_DLC_8];
uint8_t txVCprintData[4][CAN_DLC_8];
uint8_t IglaCmdData[IGLA_MAX_CMD][CAN_DLC_8]; // 0 - idle 1-left 2-right
uint8_t IglaValetCmdData[2][CAN_DLC_8];
CAN_RX_MSGOBJ rxheader;
uint8_t rxdata[MAX_DATA_BYTES];

bool dk_hw_int_detected(void);
bool console_initialized = 0;
bool last_ignition_status = 0;
bool need_send_once_at_boot_up = 1;
static OS_TIMER app_timer, print_timer, igla_timer;
extern OS_TIMER pid_timer;

int ee_printf(const char *fmt, ...);
#define printf ee_printf

void app_timer_cb(OS_TIMER pxTime);
void print_timer_cb(OS_TIMER pxTime);
void igla_deactivate_cb(OS_TIMER pxTime);

void app_timer_cb_exec()
{
        trigger_start_stop = 0;
        // Even clicks == 0 , it will still send. The case after Q3 cold boot,  even at the same state trigger again
        if(drive_select_efficiency_clicks){
            drive_select_efficiency_clicks--;
            // If there are no more needed clicks left, then disable further sends
            if(drive_select_efficiency_clicks==0)
                trigger_drive_select_efficiency = 0;
        }
        // Even clicks == 0 , it will still send. The case after Q3 cold boot,  even at the same state trigger again
        if(drive_select_dynamic_clicks){
               drive_select_dynamic_clicks--;
               // If there are no more needed clicks left, then disable further sends
               if(drive_select_dynamic_clicks==0)
                   trigger_drive_select_dynamic = 0;
        }

        // Even clicks == 0 , it will still send. The case after Q3 cold boot,  even at the same state trigger again
        if(drive_select_auto_clicks){
               drive_select_auto_clicks--;
               // If there are no more needed clicks left, then disable further sends
               if(drive_select_auto_clicks==0)
                   trigger_drive_select_auto = 0;
        }

        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_TX_QUEUE, &txConsoleButtonHeader, txConsoleButtonData, CAN_DLC_8, true);
        printf("Button pressed!\r\n");
}

void print_timer_cb_exec() // Update virtual cockpit with last printf string
{
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_TX_PRINT_FIFO, &txVCprintHeader, &txVCprintData[0][0], CAN_DLC_8, true);
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_TX_PRINT_FIFO, &txVCprintHeader, &txVCprintData[1][0], CAN_DLC_8, true);
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_TX_PRINT_FIFO, &txVCprintHeader, &txVCprintData[2][0], CAN_DLC_8, true);
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_TX_PRINT_FIFO, &txVCprintHeader, &txVCprintData[3][0], CAN_DLC_8, true);
}

void igla_deactivate_cb_exec()
{
        if (last_ignition_status == 0) // Cancel if ignition is again OFF
                return;

        if(igla_valet_mode){
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);

                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);

                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);

                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);

                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[0][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);
                DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaValetCmdHeader, &IglaValetCmdData[1][0], CAN_DLC_8, true);

                igla_valet_mode = 0;
                printf("Igla Valet!\r\n");
                return;
        }

#ifdef USE_IGLA_WHEEL
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_RIGHT_CMD][0], CAN_DLC_8, true);
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_IDLE_CMD][0], CAN_DLC_8, true);

        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_LEFT_CMD][0], CAN_DLC_8, true);
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_IDLE_CMD][0], CAN_DLC_8, true);

        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_LEFT_CMD][0], CAN_DLC_8, true);
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_IDLE_CMD][0], CAN_DLC_8, true);

        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_RIGHT_CMD][0], CAN_DLC_8, true);
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_IDLE_CMD][0], CAN_DLC_8, true);
#endif

#ifdef USE_IGLA_MIRROR_KNOB
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_RIGHT_CMD][0], CAN_DLC_8, true);
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_IDLE_CMD][0], CAN_DLC_8, true);

        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_LEFT_CMD][0], CAN_DLC_8, true);
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_IDLE_CMD][0], CAN_DLC_8, true);

        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_RIGHT_CMD][0], CAN_DLC_8, true);
        DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txIglaCmdHeader, &IglaCmdData[IGLA_IDLE_CMD][0], CAN_DLC_8, true);
#endif

        printf("Igla OFF!\r\n");
}

void vc_write (char *ptr, int len){

        //Convert /r/n to NULL characters
        #define ASSIGN_VALID_CHAR(x) (ptr[x]!=0)?(ptr[x]!='\r')?(ptr[x]!='\n')?ptr[x]:0:0:0

        txVCprintData[0][5] = ASSIGN_VALID_CHAR(0);
        txVCprintData[0][6] = ASSIGN_VALID_CHAR(1);
        txVCprintData[0][7] = ASSIGN_VALID_CHAR(2);
        txVCprintData[1][1] = ASSIGN_VALID_CHAR(3);
        txVCprintData[1][2] = ASSIGN_VALID_CHAR(4);
        txVCprintData[1][3] = ASSIGN_VALID_CHAR(5);
        txVCprintData[1][4] = ASSIGN_VALID_CHAR(6);
        txVCprintData[1][5] = ASSIGN_VALID_CHAR(7);
        txVCprintData[1][6] = ASSIGN_VALID_CHAR(8);
        txVCprintData[1][7] = ASSIGN_VALID_CHAR(9);
        txVCprintData[2][1] = ASSIGN_VALID_CHAR(10);
        txVCprintData[2][2] = ASSIGN_VALID_CHAR(11);
        txVCprintData[2][3] = ASSIGN_VALID_CHAR(12);
        txVCprintData[2][4] = ASSIGN_VALID_CHAR(13);
        txVCprintData[2][5] = ASSIGN_VALID_CHAR(14);
        txVCprintData[2][6] = ASSIGN_VALID_CHAR(15);
        txVCprintData[2][7] = ASSIGN_VALID_CHAR(16);
        txVCprintData[3][1] = ASSIGN_VALID_CHAR(17);
        txVCprintData[3][2] = ASSIGN_VALID_CHAR(18);
        txVCprintData[3][3] = ASSIGN_VALID_CHAR(19);
        txVCprintData[3][4] = ASSIGN_VALID_CHAR(20);

        bool set_space = 0;
        for (uint8_t j = 0; j < 4; j++) {
                for (uint8_t i = 0; i < 8; i++) {
                        if (txVCprintData[j][i] == 0) {
                                set_space = 1;
                        }
                        if (set_space) {
                                txVCprintData[j][i] = ' ';
                        }
                }
        }

        txVCprintData[1][0] = 0xC0; // Fixed data from candump..
        txVCprintData[2][0] = 0xC1; // Fixed data from candump..
        txVCprintData[3][0] = 0xC2; // Fixed data from candump..

        vc_overwrite_once = 1;

        if(console_initialized) // Update VC data
                print_timer_cb(0);
}

void APP_CANFDSPI_Init()
{

        app_timer = OS_TIMER_CREATE("tmr",   OS_MS_2_TICKS(200),  pdFALSE, NULL, app_timer_cb);
        print_timer = OS_TIMER_CREATE("prt", OS_MS_2_TICKS(100),  pdFALSE, NULL, print_timer_cb);
        igla_timer = OS_TIMER_CREATE("igl",  OS_MS_2_TICKS(10000), pdFALSE, NULL, igla_deactivate_cb);

        txConsoleButtonHeader.word[0] = 0;
        txConsoleButtonHeader.word[1] = 0;

        txVCprintHeader.word[0] = 0;
        txVCprintHeader.word[1] = 0;
        txVCprintHeader.bF.id.SID = (VC_FIELD_CAN_ID >> 18) & 0x7FF;
        txVCprintHeader.bF.id.EID = VC_FIELD_CAN_ID & 0x7FFFF;
        txVCprintHeader.bF.ctrl.DLC = 8;
        txVCprintHeader.bF.ctrl.IDE = 1;
        txVCprintData[0][0] = 0x80; // Fixed data from candump..
        txVCprintData[0][1] = VC_PRINT_MAX_LEN + 1;
        txVCprintData[0][2] = 0x3C; // Fixed data from candump..
        txVCprintData[0][3] = 0x93; // Fixed data from candump..
        txVCprintData[0][4] = VC_PRINT_MAX_LEN; // Length max is 21
        txVCprintData[1][0] = 0xC0; // Fixed data from candump..
        txVCprintData[2][0] = 0xC1; // Fixed data from candump..
        txVCprintData[3][0] = 0xC2; // Fixed data from candump..
        txVCprintData[3][5] = 0;    // Empty since max print len is 21 characters
        txVCprintData[3][6] = 0;    // Empty since max print len is 21 characters
        txVCprintData[3][7] = 0;    // Empty since max print len is 21 characters

        memset(&IglaCmdData[0][0], 0, sizeof(IglaCmdData[IGLA_MAX_CMD][8]));
#ifdef USE_IGLA_WHEEL
        txIglaCmdHeader.word[0] = 0;
        txIglaCmdHeader.word[1] = 0;
        txIglaCmdHeader.bF.id.SID = WHEEL_BUTTON_CAN_ID;
        txIglaCmdHeader.bF.ctrl.DLC = 8;
        IglaCmdData[IGLA_IDLE_CMD][3]  = 0x31;
        IglaCmdData[IGLA_LEFT_CMD][3]  = 0x31;
        IglaCmdData[IGLA_RIGHT_CMD][3] = 0x31;
        IglaCmdData[IGLA_LEFT_CMD][2]  = 0x01;
        IglaCmdData[IGLA_RIGHT_CMD][2] = 0x01;
        IglaCmdData[IGLA_RIGHT_CMD][0] = 0x02;  // Deksia press
        IglaCmdData[IGLA_LEFT_CMD][0]  = 0x03;  // Aristera press
#endif
#ifdef USE_IGLA_MIRROR_KNOB
        txIglaCmdHeader.word[0] = 0;
        txIglaCmdHeader.word[1] = 0;
        txIglaCmdHeader.bF.id.SID = MIRROR_KNOB_CAN_ID;
        txIglaCmdHeader.bF.ctrl.DLC = 8;
        IglaCmdData[IGLA_RIGHT_CMD][2] = 0x08;  // Deksia press
        IglaCmdData[IGLA_LEFT_CMD][2]  = 0x04;  // Aristera press
#endif

        txIglaValetCmdHeader.word[0] = 0;
        txIglaValetCmdHeader.word[1] = 0;
        txIglaValetCmdHeader.bF.id.SID = WINDOW_CAN_ID;
        txIglaValetCmdHeader.bF.ctrl.DLC = 8;
        memset(&IglaValetCmdData[0][0], 0, sizeof(IglaValetCmdData[2][8]));
        IglaValetCmdData[0][0] = 0x02; // Driver door up

        // Reset device
        DRV_CANFDSPI_Reset(DRV_CANFDSPI_INDEX_0);

        // Enable ECC and initialize RAM
        DRV_CANFDSPI_EccEnable(DRV_CANFDSPI_INDEX_0);
        DRV_CANFDSPI_RamInit(DRV_CANFDSPI_INDEX_0, 0xff);

        // Configure device
        CAN_CONFIG config;
        memset(&config, 0, sizeof(config));
        config.TXQEnable = 1;
        config.TxBandWidthSharing = 0xF; // Delay between two consecutive transmissions (in arbitration bit times)
                                         // 4096 bit = 8msec
        DRV_CANFDSPI_Configure(DRV_CANFDSPI_INDEX_0, &config);

        // Regarding RAM calculation: FIFO_size * (DLC + ~10bytes overhead)
        // Currently 18*(8+31+31+31) = 1818 byes out of max 2048
        // TxQueue holds all console button messages - sequence does not matter, priority is based on PID
        // APP_TX_PRINT_FIFO all vc print messages, sequence matters
        // APP_IGLA_TX_FIFO all igla deactivation commands, sequence matters
        // Setup TX QUEUE
        CAN_TX_QUEUE_CONFIG qconfig;
        DRV_CANFDSPI_TransmitQueueConfigureObjectReset(&qconfig);
        qconfig.FifoSize = 8;
        qconfig.PayLoadSize = CAN_PLSIZE_8;
        qconfig.TxPriority = 1;
        DRV_CANFDSPI_TransmitQueueConfigure(DRV_CANFDSPI_INDEX_0, &qconfig);

        // Setup TX FIFO
        CAN_TX_FIFO_CONFIG txConfig;
        DRV_CANFDSPI_TransmitChannelConfigureObjectReset(&txConfig);
        txConfig.FifoSize = 31;
        txConfig.PayLoadSize = CAN_PLSIZE_8;
        txConfig.TxPriority = 3;
        DRV_CANFDSPI_TransmitChannelConfigure(DRV_CANFDSPI_INDEX_0, APP_TX_PRINT_FIFO, &txConfig);

        DRV_CANFDSPI_TransmitChannelConfigureObjectReset(&txConfig);
        txConfig.FifoSize = 31;
        txConfig.PayLoadSize = CAN_PLSIZE_8;
        txConfig.TxPriority = 2;
        DRV_CANFDSPI_TransmitChannelConfigure(DRV_CANFDSPI_INDEX_0, APP_IGLA_TX_FIFO, &txConfig);

        // Setup RX FIFO
        CAN_RX_FIFO_CONFIG rxConfig;
        DRV_CANFDSPI_ReceiveChannelConfigureObjectReset(&rxConfig);
        rxConfig.FifoSize = 31;
        rxConfig.PayLoadSize = CAN_PLSIZE_8;

        DRV_CANFDSPI_ReceiveChannelConfigure(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO, &rxConfig);
        //DRV_CANFDSPI_ReceiveChannelConfigure(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO + 1, &rxConfig);
        //DRV_CANFDSPI_ReceiveChannelConfigure(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO + 2, &rxConfig);
        //DRV_CANFDSPI_ReceiveChannelConfigure(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO + 3, &rxConfig);

        // Setup RX Filter
        REG_CiFLTOBJ fObj;
        REG_CiMASK mObj;

        fObj.word = 0;
        fObj.bF.SID = CONSOLE_BUTTON_CAN_ID;
        fObj.bF.EID = 0x00;
        fObj.bF.EXIDE = 0;

        DRV_CANFDSPI_FilterObjectConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER0, &fObj.bF);

        fObj.word = 0;
        fObj.bF.SID = DRIVE_SELECT_STATUS_CAN_ID;
        fObj.bF.EID = 0x00;
        fObj.bF.EXIDE = 0;

        DRV_CANFDSPI_FilterObjectConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER1, &fObj.bF);

        fObj.word = 0;
        fObj.bF.SID = WHEEL_BUTTON_CAN_ID;
        fObj.bF.EID = 0x00;
        fObj.bF.EXIDE = 0;

        DRV_CANFDSPI_FilterObjectConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER2, &fObj.bF);

        fObj.word = 0;
        fObj.bF.SID = MIRROR_KNOB_CAN_ID;
        fObj.bF.EID = 0x00;
        fObj.bF.EXIDE = 0;

        DRV_CANFDSPI_FilterObjectConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER4, &fObj.bF);

        fObj.word = 0;
        fObj.bF.SID = TEMP_PID_CAN_ID;
        fObj.bF.EID = 0x00;
        fObj.bF.EXIDE = 0;

        DRV_CANFDSPI_FilterObjectConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER5, &fObj.bF);

        fObj.bF.SID = (VC_FIELD_CAN_ID >> 18) & 0x7FF;
        fObj.bF.EID = VC_FIELD_CAN_ID & 0x7FFFF;
        fObj.bF.EXIDE = 1;

        DRV_CANFDSPI_FilterObjectConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER3, &fObj.bF);

        // Setup RX Mask
        mObj.word = 0;
        mObj.bF.MSID = 0x7FF;
        mObj.bF.MIDE = 1; // Only allow standard IDs
        mObj.bF.MEID = 0x0;
        DRV_CANFDSPI_FilterMaskConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER0, &mObj.bF);
        DRV_CANFDSPI_FilterMaskConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER1, &mObj.bF);
        DRV_CANFDSPI_FilterMaskConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER2, &mObj.bF);
        DRV_CANFDSPI_FilterMaskConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER4, &mObj.bF);
        DRV_CANFDSPI_FilterMaskConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER5, &mObj.bF);

        mObj.word = 0x1FFFFFFF;
        mObj.bF.MIDE = 1;

        DRV_CANFDSPI_FilterMaskConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER3, &mObj.bF);

        // Link FIFO and Filter
        DRV_CANFDSPI_FilterToFifoLink(DRV_CANFDSPI_INDEX_0, CAN_FILTER0, APP_RX_FIFO, true);
        DRV_CANFDSPI_FilterToFifoLink(DRV_CANFDSPI_INDEX_0, CAN_FILTER1, APP_RX_FIFO, true);
        DRV_CANFDSPI_FilterToFifoLink(DRV_CANFDSPI_INDEX_0, CAN_FILTER2, APP_RX_FIFO, true);
        DRV_CANFDSPI_FilterToFifoLink(DRV_CANFDSPI_INDEX_0, CAN_FILTER3, APP_RX_FIFO, true);
        DRV_CANFDSPI_FilterToFifoLink(DRV_CANFDSPI_INDEX_0, CAN_FILTER4, APP_RX_FIFO, true);
        DRV_CANFDSPI_FilterToFifoLink(DRV_CANFDSPI_INDEX_0, CAN_FILTER5, APP_RX_FIFO, true);

        //DRV_CANFDSPI_FilterToFifoLink(DRV_CANFDSPI_INDEX_0, CAN_FILTER1, APP_RX_FIFO + 1, true);

        // Setup Bit Time
        DRV_CANFDSPI_BitTimeConfigure(DRV_CANFDSPI_INDEX_0, CAN_500K_2M, CAN_SSP_MODE_AUTO, CAN_SYSCLK_40M);

        // Setup Transmit and Receive Interrupts
        DRV_CANFDSPI_GpioModeConfigure(DRV_CANFDSPI_INDEX_0, GPIO_MODE_INT, GPIO_MODE_INT);

        // DRV_CANFDSPI_TransmitChannelEventEnable(DRV_CANFDSPI_INDEX_0, APP_TX_FIFO, CAN_TX_FIFO_NOT_FULL_EVENT);
        DRV_CANFDSPI_ReceiveChannelEventEnable(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO, CAN_RX_FIFO_NOT_EMPTY_EVENT);
        //DRV_CANFDSPI_ReceiveChannelEventEnable(DRV_CANFDSPI_INDEX_0, APP_RX_FIFO + 1, CAN_RX_FIFO_NOT_EMPTY_EVENT);
        //DRV_CANFDSPI_ModuleEventEnable(DRV_CANFDSPI_INDEX_0, CAN_TX_EVENT | CAN_RX_EVENT);
        DRV_CANFDSPI_ModuleEventEnable(DRV_CANFDSPI_INDEX_0, CAN_RX_EVENT);

        // Select Normal Mode
        DRV_CANFDSPI_OperationModeSelect(DRV_CANFDSPI_INDEX_0, CAN_CLASSIC_MODE);
        // DRV_CANFDSPI_OperationModeSelect(DRV_CANFDSPI_INDEX_0, CAN_NORMAL_MODE);

        DRV_CANFDSPI_ModuleEventClear(DRV_CANFDSPI_INDEX_0, CAN_ALL_EVENTS);

        if (0) { // SPI sanity test
                uint8_t txd[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
                uint8_t rxd[32];
                DRV_CANFDSPI_ReadByteArray(DRV_CANFDSPI_INDEX_0, cREGADDR_DEVID, rxd, 4);
                OS_DELAY(OS_MS_2_TICKS(10));

                DRV_CANFDSPI_WriteByteArray(DRV_CANFDSPI_INDEX_0, cRAMADDR_START, txd, 16);
                DRV_CANFDSPI_ReadByteArray(DRV_CANFDSPI_INDEX_0, cRAMADDR_START, rxd, 16);
                printf("RAM RXD: %d,%d,%d,%d, %d,%d,%d,%d \r\n",rxd[0],rxd[1],rxd[2],rxd[3], rxd[4],rxd[5],rxd[6],rxd[7]);
                OS_DELAY(OS_MS_2_TICKS(10));

                DRV_CANFDSPI_WriteByteArray(DRV_CANFDSPI_INDEX_0, cREGADDR_CiFLTOBJ, txd, 16);
                DRV_CANFDSPI_ReadByteArray(DRV_CANFDSPI_INDEX_0, cREGADDR_CiFLTOBJ, rxd, 16);
                printf("REG RXD: %d,%d,%d,%d, %d,%d,%d,%d\r\n",rxd[0],rxd[1],rxd[2],rxd[3], rxd[4],rxd[5],rxd[6],rxd[7]);
                OS_DELAY(OS_MS_2_TICKS(10));
        }
}

// If Console button has not appeared for a while, go ahead and trigger it
#define TRIGGER_ACTION(action, str, byte, value) \
        if (action == 0) { \
            action = 1; \
            printf(str); \
            if ((OS_TICKS_2_MS(OS_GET_TICK_COUNT()) - last_console_button_stamp) > 1500){ \
                 txConsoleButtonData[byte] |= value; \
                 app_timer_cb(0); \
            } \
        }

void proccess_rx_data()
{
        if(rxheader.bF.ctrl.IDE){ // Process extended IDs
                if (((rxheader.bF.id.SID <<18) | rxheader.bF.id.EID) == VC_FIELD_CAN_ID){
                        console_initialized = 1;
                        if(vc_overwrite_once){
                                OS_TIMER_RESET(print_timer, OS_TIMER_FOREVER); //Overwrite VC print with our own message
                                vc_overwrite_once--;
                        }
                }
                return; // Do not check for standard PIDs
        }

        if (rxheader.bF.id.SID == TEMP_PID_CAN_ID) {
                if (rxdata[0] == 0x0) {
                        oil_temp = rxdata[2] - 60;
                }
        }

        if (rxheader.bF.id.SID == MIRROR_KNOB_CAN_ID) {
                if (rxdata[2] == 0x10) { // Left mirror selected - igla valet s activated
                        // alarm_activated = 1;
                        if(knob_left_long_press == 0){ // Ignore first press in case it is a manual igla deactivate command
                                if(igla_valet_mode == 0){
                                        OS_TIMER_CHANGE_PERIOD(igla_timer, OS_MS_2_TICKS(2500),  OS_TIMER_FOREVER);
                                        OS_TIMER_RESET(igla_timer, OS_TIMER_FOREVER);
                                        printf("schedule valet!");
                                }
                                igla_valet_mode = 1;
                        }
                        knob_left_long_press = 1;
                }
                if (rxdata[2] == 0x00) { // Left mirror not selected
                        // alarm_activated = 0;
                        knob_left_long_press = 0;
                       // knob_right_long_press = 0;
                }
                if (rxdata[2] == 0x01) { // up button knob selected
                        TRIGGER_ACTION(trigger_drive_select_dynamic, "trigger_ds_dynamic! \r\n", 4, 0x02)
                }
                if (rxdata[2] == 0x02) { // down button knob selected
                        TRIGGER_ACTION(trigger_drive_select_efficiency,"trigger_ds_efficiency! \r\n", 4, 0x02)
                }
                if (rxdata[2] == 0x08) { // deksia button knob selected
                       // if(knob_right_long_press){ // Ignore first press in case it is a manual igla deactivate command
                       //         TRIGGER_ACTION(trigger_drive_select_auto,"trigger_ds_auto! \r\n", 4, 0x02)
                       // }
                       // knob_right_long_press = 1;
                }
                if (rxdata[2] == 0x04) { // aristera button knob selected

                }
        }

        if (rxheader.bF.id.SID == WHEEL_BUTTON_CAN_ID) {
#if 0
                if (rxdata[0] == WHEEL_BUTTON_OK) {
                        if (rxdata[1] == WHEEL_BUTTON_ASTERISK) {
                                TRIGGER_ACTION(trigger_drive_select_efficiency,"trigger_ds_efficiency! \r\n", 4, 0x02)
                        }
                }
                if (rxdata[0] == WHEEL_BUTTON_VIEW) {
                        if (rxdata[1] == WHEEL_BUTTON_ASTERISK) {
                                TRIGGER_ACTION(trigger_drive_select_dynamic, "trigger_ds_dynamic! \r\n", 4, 0x02)
                        }
                }
#endif
                if (rxdata[0] == WHEEL_BUTTON_OK) {
                        if( OS_TIMER_IS_ACTIVE(pid_timer)){
                                OS_TIMER_STOP(pid_timer, OS_TIMER_FOREVER);
                        }else{
                                OS_TIMER_RESET(pid_timer, OS_TIMER_FOREVER); //Overwrite VC print with our own message
                        }
                }
//                if (rxdata[0] == WHEEL_BUTTON_VIEW) {
//                }
                if (rxdata[0] == WHEEL_BUTTON_ASTERISK) {
                        TRIGGER_ACTION(trigger_start_stop, "trigger_start_stop! \r\n", 3, 0x01)
                }

                if ((trigger_drive_select_efficiency) || (trigger_drive_select_dynamic) || (trigger_drive_select_auto)) {
                        trigger_start_stop = 0; // drive_select_* has priority over start_stop_trigger
                }
        }

        if (rxheader.bF.id.SID == DRIVE_SELECT_STATUS_CAN_ID) {
                if ((rxdata[6] == 0x30)) { // Dynamic
                        drive_select_efficiency_clicks = 3;
                        drive_select_dynamic_clicks = 0;
                        drive_select_auto_clicks = 5;
                }
                if ((rxdata[6] == 0x20)) { // Auto
                        drive_select_efficiency_clicks = 4;
                        drive_select_dynamic_clicks = 1;
                        drive_select_auto_clicks = 0;
                }
                if ((rxdata[6] == 0x70)) { // Individual
                        drive_select_efficiency_clicks = 2;
                        drive_select_dynamic_clicks = 5;
                        drive_select_auto_clicks = 4;
                }
                if ((rxdata[6] == 0x90)) { // OffRoad
                        drive_select_efficiency_clicks = 1;
                        drive_select_dynamic_clicks = 4;
                        drive_select_auto_clicks = 3;
                }
                if ((rxdata[6] == 0x50)) { // Efficiency
                        drive_select_efficiency_clicks = 0;
                        drive_select_dynamic_clicks = 3;
                        drive_select_auto_clicks = 2;
                }
                if ((rxdata[6] == 0x10)) { // Comfort
                        drive_select_efficiency_clicks = 5;
                        drive_select_dynamic_clicks = 2;
                        drive_select_auto_clicks = 1;
                }
                //printf("eff: %d dyn: %d ! \r\n", (int) drive_select_efficiency_clicks, (int) drive_select_dynamic_clicks);
        }

        // If its a console status message.
        // Send after 500msec. Reset timer if needed. Prepare send payload.
        if (rxheader.bF.id.SID == CONSOLE_BUTTON_CAN_ID) {
                last_console_button_stamp = OS_TICKS_2_MS(OS_GET_TICK_COUNT());
                txConsoleButtonHeader.word[0] = 0;
                txConsoleButtonHeader.word[1] = 0;
                txConsoleButtonHeader.bF.id.SID = CONSOLE_BUTTON_CAN_ID;
                txConsoleButtonHeader.bF.ctrl.DLC = 8;
                memcpy(txConsoleButtonData, rxdata, 8); // Copy latest data


                if(0){ //if(alarm_activated == 0){ //No knob selection
                        if(rxdata[2] & 0x02){ //Ignition status
                                if(need_send_once_at_boot_up){ //Bootup:  igla should be de-activated
                                        OS_TIMER_CHANGE_PERIOD(igla_timer, OS_MS_2_TICKS(50),  OS_TIMER_FOREVER);
                                        OS_TIMER_RESET(igla_timer, OS_TIMER_FOREVER);
                                        need_send_once_at_boot_up = 0;
                                        printf("schedule igla off");
                                } else if(last_ignition_status == 0){ // Ignition ON after OFF..
                                        if(OS_TIMER_IS_ACTIVE(igla_timer)){
                                                // Timer is running, igla is activated after 10 sec ignition is OFF,
                                                // so wait to send the deactivate command after 10sec..
                                                // Do nothing here..
                                                printf("wait for igla off");
                                        }else{ // 10 sec have passed during ignition off, send deactivate commands immediately
                                                igla_deactivate_cb(0);
                                        }
                                }
                                last_ignition_status = 1;
                        }else{
                                if(last_ignition_status){ //Alarm command already send
                                        OS_TIMER_CHANGE_PERIOD(igla_timer, OS_MS_2_TICKS(12000),  OS_TIMER_FOREVER);
                                        OS_TIMER_RESET(igla_timer, OS_TIMER_FOREVER); // Send again at least after 10 sec (time the alarm takes to activate again)
                                }
                                last_ignition_status = 0;
                        }
                }


                if ((trigger_drive_select_dynamic) || (trigger_drive_select_efficiency) || (trigger_drive_select_auto) ) {
                        //  NEW: 00 00 02 10 02 80 3D 00  vs tmr: 0000065A#0000021000803D00
                        txConsoleButtonData[4] |= 0x02;
                        OS_TIMER_RESET(app_timer, OS_TIMER_FOREVER);
                        printf("ds button scheduled! \r\n");
                }

                if (trigger_start_stop) {
                        //  NEW: 0000065A#0000021100803D00 vs tmr: 0000065A#0000021000803D00
                        txConsoleButtonData[3] |= 0x01;
                        OS_TIMER_RESET(app_timer, OS_TIMER_FOREVER);
                        printf("ss button scheduled! \r\n");
                }
        }
}

void handle_spi_interrupt(){
        while (dk_hw_int_detected()) { // Check if more interrupts are still pending, need to serve all in order for
                                       // the pin get high and trigger again
                DRV_CANFDSPI_ReadByteArray(DRV_CANFDSPI_INDEX_0, cREGADDR_CiVEC, rxdata, 4 * 6);
                if (0) {
                        printf("CiVEC    0x%02X%02X%02X%02X\r\n", rxdata[3], rxdata[2], rxdata[1], rxdata[0]);
                        printf("CiINT    0x%02X%02X%02X%02X\r\n", rxdata[7], rxdata[6], rxdata[5], rxdata[4]);
                        printf("CiRXIF   0x%02X%02X%02X%02X\r\n", rxdata[11], rxdata[10], rxdata[9], rxdata[8]);
                        printf("CiRXOVIF 0x%02X%02X%02X%02X\r\n", rxdata[15], rxdata[14], rxdata[13],rxdata[12]);
                        printf("CiTXIF   0x%02X%02X%02X%02X\r\n", rxdata[19], rxdata[18], rxdata[17],rxdata[16]);
                }
                CAN_RX_FIFO_EVENT rxFlags;
                //CAN_TX_FIFO_EVENT txFlags;


                uint8_t rx_channel = rxdata[3] & 0x1F; // This is CiVEC RXCODE byte
                if (rx_channel) {
                        DRV_CANFDSPI_ReceiveChannelEventGet(DRV_CANFDSPI_INDEX_0, rx_channel, &rxFlags);
                        if (rxFlags & CAN_RX_FIFO_NOT_EMPTY_EVENT) {
                                DRV_CANFDSPI_ReceiveMessageGet(DRV_CANFDSPI_INDEX_0, rx_channel, &rxheader, rxdata, CAN_DLC_8);
                                proccess_rx_data();
                        }
                        //TODO: Need to handle other events
                }
                //TODO: Handle TXCODE and other INFO interrupts
        }
}

