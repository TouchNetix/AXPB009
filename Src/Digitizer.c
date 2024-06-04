/*******************************************************************************
* @file           : Digitizer.c
* @author         : James Cameron (TouchNetix)
* @date           : 27 Oct 2020
*******************************************************************************/

/*
******************************************************************************
* Copyright (c) 2023 TouchNetix
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
******************************************************************************
*/

/*============ Includes ============*/
#include "Init.h"
#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"
#include <stdio.h>
#include <stdbool.h>
#include "Digitizer.h"
#include "usb_device.h"
#include "usbd_conf.h"
#include "usbd_composite.h"
#include "usbd_mouse_if.h"
#include "usbd_mouse.h"
#include "Comms.h"
#include "Proxy_driver.h"
#include "Press_driver.h"
#include "Usage_Builder.h"
#include "Mode_Control.h"
#include "Flash_Control.h"

/*============ Defines ============*/
#define U41_REPORT                  (0x41)
#define TOUCH_NUMBER                (1)

#define DIGI_CONFIDENCE             (0x04)
#define DIGI_TIP_SWITCH             (0x01)
#define DIGI_IN_RANGE               (0x02)

#define TPAD_TIP_SWITCH             (0x02)
#define TPAD_CONFIDENCE             (0x01)

#define X_COORD_LSB                 (2)
#define X_COORD_MSB                 (3)
#define Y_COORD_LSB                 (4)
#define Y_COORD_MSB                 (5)
#define PRESSURE_LSB                (6)
#define PRESSURE_MSB                (7)

#define TOUCHPAD_SURFACESWITCH_POS  (0U)
#define TOUCHPAD_BUTTONSWITCH_POS   (1U)

#define REL_MOUSE_MAX_POS_CHANGE    (127)
#define REL_MOUSE_MAX_NEG_CHANGE    (-127)

#define DO_NOT_IGNORE_COORDS        (0)
#define IGNORE_COORDS               (1)
#define MOUSE_RIGHT_BUTTON          (0x02)
#define MOUSE_LEFT_BUTTON           (0x01)
#define MOUSE_NO_BUTTONS            (0x00)
#define DATABYTES_PER_TOUCH         (7)

/*============ Compilation Flags ============*/


/*============ Macros ============*/
#define ALIGN_WITH_CORRECT_TOUCH(x)  ( ( x - 1 ) * DATABYTES_PER_TOUCH )

/*============ Data Types ============*/
enum en_RelativeMouseEvents
{
    eNewContact = 0,
    ePreviousContact = 1,
    eNoContacts = 2,
};

enum en_InputReportingMode
{
    eMouseMode = 0,
    eTouchPadMode = 3,
};

struct RelMouse_AxisInfo
{
    int32_t MovementCache;
    bool    CursorHasMoved;
    bool    CursorHasMovedThisFrame;
};

struct RelativeMouse
{
    struct RelMouse_AxisInfo AxisX;
    struct RelMouse_AxisInfo AxisY;
};

struct Touchpad
{

};

struct TouchPadInfo
{
    // Controls (set by the host)
    enum en_InputReportingMode  InputMode;      /* Relative mouse mode or Windows Precision Touchpad mode */
    bool                        SurfaceSwitch;  /* Indicates if surface contacts should be reported */
    bool                        ButtonSwitch;   /* Indicates if button states should be reported */

    // Mouse and Touchpad info
    struct RelativeMouse        RelMouseInfo;
    struct Touchpad             TouchpadInfo;
};

/*============ Local Variables ============*/
bool        boCRCCheckOK = 0;
uint16_t    DigitizerXCoord, DigitizerYCoord;
uint8_t     byReportX_msb, byReportX_lsb;
uint8_t     byReportY_msb, byReportY_lsb;
uint8_t     byReportZ_msb, byReportZ_lsb;
uint8_t     button_state; // bit 0 = left-button, bit 1 = right-button

//This table was extracted from online tool at http://www.sunshine2k.de/coding/javascript/crc/crc_js.html and verified with one other online source
//Note that it may also be known as 0xA001 in reverse polynomial notation (i.e. 0x8005 backwards)
const uint16_t CRC16_IBM_8005_Table[256] = {
    0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
    0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
    0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
    0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
    0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
    0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
    0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
    0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
    0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
    0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
    0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
    0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
    0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
    0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
    0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
    0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
    0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
    0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
    0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
    0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
    0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
    0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
    0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
    0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
    0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
    0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
    0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
    0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
    0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
    0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
    0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
    0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
};

struct TouchPadInfo g_TouchPadInfo = {0};

/*============ Exported Variables ============*/
volatile uint16_t wd100usTick                               =  0;       // this is used to mark the digitizer packet with a timestamp --> incremented in SysTick_Handler every millisecond, Windows is expecting this value to overflow/wrap around, it is used as a reference from when the first touch is registered (after a period of not touching)
volatile uint16_t wdUSB1msTick                              =  0;       // this is used as a timer to re-activate proxy mode after TH2/host disconnects
uint8_t  u34_TCP_report[USBD_GENERIC_HID_REPORT_IN_SIZE]    = {0};      // note: u34 is the FIFO buffer on aXiom that all reports come out on --> digitizer report is an u41, but we see it coming in the u34 buffer!
uint8_t  byNumTouches                                       =  0;
uint8_t  touch_data[5][8]                                   = {0};
uint8_t  usb_remote_wake_state                              =  RESUMED;
bool     boUSBTimeoutEnabled                                =  0;
bool     boMouseEnabled                                     =  1;       // starts with digitizer enable (TH2 doesn't know the command to toggle it!)
uint8_t  wakeup_option                                      =  0;

/*============ Local Function Prototypes ============*/
static uint8_t  GetXYZFromReport(bool boIgnoreCoords, uint8_t byTouchNum);
static void     DecodeOneTouch(uint8_t byTouchToCheck, uint8_t *byStatus, uint8_t *wdXCoord, uint8_t *wdYCoord, uint8_t *byZAmplitude);
static void     ProcessRelativeMouse(void);
static void     ProcessTouchPad(void);
static void     PrepareRelMouseReport(enum en_RelativeMouseEvents ContactEvent);
static int8_t   ScaleRelMouseMovement(struct RelMouse_AxisInfo *pAxisInfo, int32_t PreScaleDistance);
static void     PrepareAbsMouseReport(void);
static void     SendMouseRightClick(void);
static inline   uint16_t CRC16_IBM_8005(uint16_t CRCIn, uint8_t DataIn);

/*============ Exported Functions ============*/

void DigitizerInit(void)
{
    g_TouchPadInfo.InputMode = eMouseMode;

    // After a reset, report both contacts and button states
    // https://tinyurl.com/4fkmpzu3
    g_TouchPadInfo.SurfaceSwitch = true;
    g_TouchPadInfo.ButtonSwitch = true;
}

/*-----------------------------------------------------------*/

// checks if the data obtained during proxy is a touch report, and whether it passed the CRC check
bool Check_u41Report(void)
{
    bool status = 0;

    if((u34_TCP_report[1] == U41_REPORT) && boCRCCheckOK)
    {
        status = true;
        boDoPressEvent = true;
    }
    else
    {
        status = false;
    }

    return status;
}

/*-----------------------------------------------------------*/

// counts the number of touches reported --> this is more for the benefit of press
uint8_t CheckTouches(void)
{
    uint8_t NumTouches = 0;
    uint8_t TouchNum;

    for(TouchNum = 1; TouchNum <= 5; TouchNum++)
    {
        DecodeOneTouch(TouchNum, &touch_data[TouchNum - 1][0], &touch_data[TouchNum - 1][1], &touch_data[TouchNum - 1][3], &touch_data[TouchNum - 1][5]);
    }

    NumTouches = ((u34_TCP_report[2] & 1)  ? 1 : 0)  +   // these bits indicate how many touches are present on the screen
                 ((u34_TCP_report[2] & 2)  ? 1 : 0)  +
                 ((u34_TCP_report[2] & 4)  ? 1 : 0)  +
                 ((u34_TCP_report[2] & 8)  ? 1 : 0)  +
                 ((u34_TCP_report[2] & 16) ? 1 : 0);

    return NumTouches;
}

/*-----------------------------------------------------------*/

void SetTouchPadMode(uint8_t Mode)
{
    if (Mode == INPUT_MOUSEMODE)
    {
        g_TouchPadInfo.InputMode = eMouseMode;
        byMouseReportLength = MOUSE_REL_REPORT_LENGTH;
    }
    else if (Mode == INPUT_TOUCHPADMODE)
    {
        g_TouchPadInfo.InputMode = eTouchPadMode;
        byMouseReportLength = MOUSE_TOUCHPAD_REPORT_LENGTH;
    }
    else
    {
        // If unknown input mode selected, default to mouse mode (as per recommendation from Microsoft https://tinyurl.com/9v3sa5eb)
        g_TouchPadInfo.InputMode = eMouseMode;
    }
}

/*-----------------------------------------------------------*/

void SetTouchPadInputSelection(uint8_t InputSelection)
{
    g_TouchPadInfo.SurfaceSwitch = ((InputSelection & (1U << TOUCHPAD_SURFACESWITCH_POS)) != 0) ? true : false;
    g_TouchPadInfo.ButtonSwitch  = ((InputSelection & (1U << TOUCHPAD_BUTTONSWITCH_POS)) != 0) ? true : false;
}

/*-----------------------------------------------------------*/

//Fast look-up table CRC16 for use in real-time in processing etc
uint16_t ComputeCRC16(uint8_t *Buffer, uint32_t Len, uint16_t SeedCRC)
{
    while (Len--)
    {
        SeedCRC = CRC16_IBM_8005(SeedCRC, *Buffer);
        Buffer++;
    }

    return(SeedCRC);
}


void CRC_Checksum(void) // Checks if CRC received data is correct
{
    uint16_t CRC_Report = 0;
    uint16_t CRC_Calc = 0;

    // Extract CRC from report
    if(u34_TCP_report[0] >= 3)
    {
        uint32_t CRC_Offset = ((u34_TCP_report[0] & 0x7F) * 2) - 2;
        if(CRC_Offset > 62)
        {
            CRC_Offset = 62;
        }
        CRC_Report = u34_TCP_report[CRC_Offset] | (u34_TCP_report[CRC_Offset + 1] << 8);

        CRC_Calc = ComputeCRC16(u34_TCP_report, CRC_Offset, 0);

        if(CRC_Calc != CRC_Report)
        {
            // Failed - blocks report being sent to host
            boCRCCheckOK = 0;
        }
        else
        {
            // Passed
            boCRCCheckOK = 1;
        }
    }
    else
    {
        boCRCCheckOK = 0;
    }
}

/*-----------------------------------------------------------*/

void MultiPointDigitizer(void)
{
    bool     boGotTouchReport;
    uint8_t  byTouchNum;
    uint8_t  touched;
    uint16_t digitizer_timer;
    uint16_t digitizer_pressure;

    boGotTouchReport = Check_u41Report();   // checks if we are actually dealing with a u41 report or not

    if(boGotTouchReport)    // touch (digitizer) report waiting to be processed
    {
        byNumTouches = CheckTouches();  // discovers how many touches are present - necessary for press

        if((usb_remote_wake_state == SUSPENDED) || (usb_remote_wake_state == PENDING_WAKE))
        {
            if(usb_remote_wake_state == PENDING_WAKE)
            {
                HAL_PCD_ActivateRemoteWakeup(&hpcd_USB_FS);
                HAL_Delay(10);
                HAL_PCD_DeActivateRemoteWakeup(&hpcd_USB_FS);
            }
            else if((usb_remote_wake_state == SUSPENDED))
            {
                if(WakeupHost(byNumTouches, byReportZ_lsb) == true)
                {
                    usb_remote_wake_state = PENDING_WAKE;
                }
                else
                {
                    // do nothing!
                }
            }
        }
        else // if(usb_remote_wake_state == RESUMED)
        {
            usb_hid_mouse_report_in[0] = 1; //report number --> relates to the report number found in the report descriptor (allows windows to differentiate between different connected devices)

            for(byTouchNum = 1u; byTouchNum <= DIGITIZER_MAX_CONTACT_CT; byTouchNum++) // perform same processing for each touch
            {
                if(GetXYZFromReport(DO_NOT_IGNORE_COORDS, byTouchNum))
                {
                    if(byReportZ_lsb >= 0x80)   // if z coordinate is a negative value it indicates there is a hover or prox
                    {
                        touched = DIGI_CONFIDENCE | DIGI_IN_RANGE; // hover present --> set in range bit
                    }
                    else
                    {
                        touched = DIGI_CONFIDENCE | DIGI_IN_RANGE | DIGI_TIP_SWITCH; // touch present --> set tip switch bit
                    }
                }
                else
                {
                    touched = DIGI_CONFIDENCE;
                }

                // translates the pressure value into the range 0-1024 (prevents Windows from messing with our values!)
                digitizer_pressure = byReportZ_lsb;
                digitizer_pressure = digitizer_pressure + 1;
                digitizer_pressure = digitizer_pressure * 4;

                usb_hid_mouse_report_in[ALIGN_WITH_CORRECT_TOUCH(byTouchNum) + TOUCH_NUMBER]      = (uint8_t)(byTouchNum << 3u) | touched;
                usb_hid_mouse_report_in[ALIGN_WITH_CORRECT_TOUCH(byTouchNum) + X_COORD_LSB]       = (DigitizerXCoord >> 4) & 0xFF;
                usb_hid_mouse_report_in[ALIGN_WITH_CORRECT_TOUCH(byTouchNum) + X_COORD_MSB]       = (DigitizerXCoord >> 4) >> 8;
                usb_hid_mouse_report_in[ALIGN_WITH_CORRECT_TOUCH(byTouchNum) + Y_COORD_LSB]       = (DigitizerYCoord >> 4) & 0xFF;
                usb_hid_mouse_report_in[ALIGN_WITH_CORRECT_TOUCH(byTouchNum) + Y_COORD_MSB]       = (DigitizerYCoord >> 4) >> 8;
                usb_hid_mouse_report_in[ALIGN_WITH_CORRECT_TOUCH(byTouchNum) + PRESSURE_LSB]      = (uint8_t)(digitizer_pressure & 0xFF);
                usb_hid_mouse_report_in[ALIGN_WITH_CORRECT_TOUCH(byTouchNum) + PRESSURE_MSB]      = (uint8_t)((digitizer_pressure >> 8) & 0xFF);
            }

            digitizer_timer = wd100usTick; // Windows expects this to increment of 100 microseconds - so we do that!

            usb_hid_mouse_report_in[(DIGITIZER_MAX_CONTACT_CT * DATABYTES_PER_TOUCH) + 1] = digitizer_timer & 0xFF;
            usb_hid_mouse_report_in[(DIGITIZER_MAX_CONTACT_CT * DATABYTES_PER_TOUCH) + 2] = digitizer_timer >> 8;
            usb_hid_mouse_report_in[(DIGITIZER_MAX_CONTACT_CT * DATABYTES_PER_TOUCH) + 3] = byNumTouches;   // contact count, can be set between a value of 0 and 5 --> windows digitizer requires at least 5 touches to work correctly

            u34_TCP_report[1] = 0x00;   // "consumes" the report so it isn't used again

            boMouseReportToSend = 1;

        }
    }
}

/*-----------------------------------------------------------*/

void AbsoluteMouseDigitizer(void)
{
    bool boGotTouchReport;
    bool boTouch1Is = false;
    static bool     RightClickActive = false;
    static uint8_t  byNumTouchesWas  = 0;
    static uint8_t  byNumTouchesIs   = 0;


    boGotTouchReport = Check_u41Report();   // checks if we're actually dealing with a u41 (touch report)

    if(boGotTouchReport)
    {
        byNumTouchesWas = byNumTouchesIs;   // remembers how many touches were present last time a touch report came in
        byNumTouchesIs = CheckTouches();    // check how many touches are present

        if((usb_remote_wake_state == SUSPENDED) || (usb_remote_wake_state == PENDING_WAKE))
        {
            if(usb_remote_wake_state == PENDING_WAKE)
            {
                HAL_PCD_ActivateRemoteWakeup(&hpcd_USB_FS);
                HAL_Delay(10);
                HAL_PCD_DeActivateRemoteWakeup(&hpcd_USB_FS);
            }
            else if((usb_remote_wake_state == SUSPENDED) && (byNumTouchesIs > 0))
            {
                usb_remote_wake_state = PENDING_WAKE;
            }
        }
        else if(usb_remote_wake_state == RESUMED)
        {
            boTouch1Is = GetXYZFromReport(DO_NOT_IGNORE_COORDS, 1); // only copy the coordinates for the first touch --> second touch is a 'right click' so don't want location of it, just if it's present or not

            if(((byReportZ_lsb & 0x80) != 0x80)) // if z value is negative, reject the touch!
            {
                if((byNumTouchesIs < 2) && (byNumTouchesWas == 2))
                {
                    SendMouseRightClick();
                    RightClickActive = true;
                }

                if((byNumTouchesIs == 0) && (byNumTouchesWas > 0))
                {
                    button_state = 0;
                    PrepareAbsMouseReport();
                    RightClickActive = false;
                }
                else
                {
                    if(boTouch1Is)
                    {
                        button_state = RightClickActive ? 0 : 1;
                        PrepareAbsMouseReport();
                    }
                }
            }
        }

        u34_TCP_report[1] = 0x00;   // "consumes" the report so it isn't used again
    }
}

/*-----------------------------------------------------------*/

void PrecisionTouchpad(void)
{
    bool boGotTouchReport;
    uint8_t byNumTouchesIs = CheckTouches();

    boGotTouchReport = Check_u41Report();   // checks if we're actually dealing with a u41 (touch report)

    if(boGotTouchReport)
    {
        if((usb_remote_wake_state == SUSPENDED) || (usb_remote_wake_state == PENDING_WAKE))
        {
            if(usb_remote_wake_state == PENDING_WAKE)
            {
                HAL_PCD_ActivateRemoteWakeup(&hpcd_USB_FS);
                HAL_Delay(10);
                HAL_PCD_DeActivateRemoteWakeup(&hpcd_USB_FS);
            }
            else if((usb_remote_wake_state == SUSPENDED) && (byNumTouchesIs > 0))
            {
                usb_remote_wake_state = PENDING_WAKE;
            }
        }
        else if(usb_remote_wake_state == RESUMED)
        {
            if(((byReportZ_lsb & 0x80) != 0x80)) // if z value is negative, reject the touch!
            {
                if (g_TouchPadInfo.InputMode == eMouseMode)
                {
                    ProcessRelativeMouse();
                }
                else if (g_TouchPadInfo.InputMode == eTouchPadMode)
                {
                    ProcessTouchPad();
                }
                else
                {
                    // Unknown input mode
                }
            }
        }

        u34_TCP_report[1] = 0x00;   // "consumes" the report so it isn't used again
    }
}

/*-----------------------------------------------------------*/

// sets parameters and flags to put the bridge in proxy mode at startup
void setup_proxy_for_digitizer(void)
{
    aXiom_NumBytesTx = 4; // num bytes to send
    aXiom_NumBytesRx = 64; // num bytes to read (1 report)
    memset(aXiom_Tx_Buffer, 0x00, sizeof(aXiom_Tx_Buffer));
    aXiom_Tx_Buffer[0] = u34_addr & 0x00FF; // offset of u34
    aXiom_Tx_Buffer[1] = (u34_addr & 0xFF00) >> 8; // page address of u34
    memset(usb_hid_mouse_report_in, 0x00, USBD_MOUSE_HID_REPORT_IN_SIZE);   // pre-zero this
}

/*-----------------------------------------------------------*/

/*============ Local Functions ============*/

static void DecodeOneTouch(uint8_t byTouchToCheck, uint8_t *byStatus, uint8_t *wdXCoord, uint8_t *wdYCoord, uint8_t *byZAmplitude)
{
    // DON'T copy coordinates into global variables
    *byStatus = GetXYZFromReport(IGNORE_COORDS, byTouchToCheck);

    *((wdXCoord) + 0) = byReportX_lsb;
    *((wdXCoord) + 1) = byReportX_msb;
    *((wdYCoord) + 0) = byReportY_lsb;
    *((wdYCoord) + 1) = byReportY_msb;

    *byZAmplitude = byReportZ_lsb;
}

/*-----------------------------------------------------------*/

static void ProcessRelativeMouse(void)
{
    bool boTouch1Is = GetXYZFromReport(DO_NOT_IGNORE_COORDS, 1); // only copy the coordinates for the first touch --> second touch is a 'right click' so don't want location of it, just if it's present or not
    static uint8_t byNumTouchesIs = 0;
    static uint8_t byNumTouchesWas = 0;

    byNumTouchesWas = byNumTouchesIs;
    byNumTouchesIs = CheckTouches();

    // =========== Button States START ===========
    if (byNumTouchesIs > 2)
    {
        // 3 or more contacts - Right button.
        button_state = MOUSE_RIGHT_BUTTON;
    }
    else if (byNumTouchesIs > 1)
    {
        // 2 contacts - Left button.
        button_state = MOUSE_LEFT_BUTTON;
    }
    else if (byNumTouchesIs > 0)
    {
        // 1 contact - No buttons.
        button_state = MOUSE_NO_BUTTONS;
    }
    else
    {
        // No contacts this frame
        if ((g_TouchPadInfo.RelMouseInfo.AxisX.CursorHasMoved == false) && (g_TouchPadInfo.RelMouseInfo.AxisY.CursorHasMoved == false))
        {
            if (byNumTouchesWas > 1)
            {
                button_state = MOUSE_RIGHT_BUTTON;
            }
            else if (byNumTouchesWas > 0)
            {
                button_state = MOUSE_LEFT_BUTTON;
            }
        }
    }
    // =========== Button States END ===========

    if (boTouch1Is)
    {
        if (byNumTouchesWas == 0)
        {
            // First contact has just appeared this frame
            PrepareRelMouseReport(eNewContact);
        }
        else
        {
            // Contact was present last frame
            PrepareRelMouseReport(ePreviousContact);
        }

        if ((g_TouchPadInfo.RelMouseInfo.AxisX.CursorHasMovedThisFrame == true) ||
                (g_TouchPadInfo.RelMouseInfo.AxisY.CursorHasMovedThisFrame == true) ||
                (button_state != MOUSE_NO_BUTTONS))
        {
            boMouseReportToSend = 1;
        }
    }
    else
    {
        if (byNumTouchesWas > 0)
        {
            PrepareRelMouseReport(ePreviousContact);
            boMouseReportToSend = 1;
        }
        else if ((byNumTouchesWas == 0) && (button_state != MOUSE_NO_BUTTONS))
        {
            // Button state to clear
            button_state = MOUSE_NO_BUTTONS;
            PrepareRelMouseReport(eNoContacts);
            boMouseReportToSend = 1;
        }
        else
        {
            // No report to prepare
            g_TouchPadInfo.RelMouseInfo.AxisX.CursorHasMoved = false;
            g_TouchPadInfo.RelMouseInfo.AxisY.CursorHasMoved = false;
            button_state = MOUSE_NO_BUTTONS;
        }
    }
}

/*-----------------------------------------------------------*/

static void ProcessTouchPad(void)
{
    static uint8_t byNumTouchesWas = 0;
    static uint8_t byNumTouchesIs = 0;

    byNumTouchesWas = byNumTouchesIs;
    byNumTouchesIs = CheckTouches();

    usb_hid_mouse_report_in[0] = REPORT_TOUCHPAD; /* Report ID */

    for(uint8_t byTouchNum = 0U; byTouchNum < PRECISION_TPAD_MAX_CONTACT_CT; byTouchNum++) /* perform same processing for each touch */
    {
        uint8_t byCorrectedTouchNum = byTouchNum + 1;
        uint8_t ArrayIndexOffset = (byTouchNum * 5U);
        uint8_t touch_state = 0;

        /* Negative Z value indicates the target is a hover or prox */
        if(GetXYZFromReport(DO_NOT_IGNORE_COORDS, byCorrectedTouchNum) && (byReportZ_lsb < 0x80))
        {
            touch_state = TPAD_TIP_SWITCH | TPAD_CONFIDENCE;

            usb_hid_mouse_report_in[ArrayIndexOffset + 2] = (DigitizerXCoord >> 4) & 0xFF;
            usb_hid_mouse_report_in[ArrayIndexOffset + 3] = (DigitizerXCoord >> 4) >> 8;
            usb_hid_mouse_report_in[ArrayIndexOffset + 4] = (DigitizerYCoord >> 4) & 0xFF;
            usb_hid_mouse_report_in[ArrayIndexOffset + 5] = (DigitizerYCoord >> 4) >> 8;
        }
        else
        {
            touch_state = TPAD_CONFIDENCE;
        }

        usb_hid_mouse_report_in[ArrayIndexOffset + 1] = (uint8_t)(byTouchNum << 2u) | touch_state;
    }

    // Scan time
    /* Windows expects this to increment of 100 microseconds */
    usb_hid_mouse_report_in[(PRECISION_TPAD_MAX_CONTACT_CT * 5) + 1] = wd100usTick & 0xFF;
    usb_hid_mouse_report_in[(PRECISION_TPAD_MAX_CONTACT_CT * 5) + 2] = wd100usTick >> 8;

    // Contact count
    if (byNumTouchesWas > byNumTouchesIs)
    {
        usb_hid_mouse_report_in[(PRECISION_TPAD_MAX_CONTACT_CT * 5) + 3] = byNumTouchesWas;
    }
    else
    {
        usb_hid_mouse_report_in[(PRECISION_TPAD_MAX_CONTACT_CT * 5) + 3] = byNumTouchesIs;
    }
//    usb_hid_mouse_report_in[(PRECISION_TPAD_MAX_CONTACT_CT * 5) + 3] = 5;
//    usb_hid_mouse_report_in[(PRECISION_TPAD_MAX_CONTACT_CT * 5) + 3] = byNumTouches;
//    usb_hid_mouse_report_in[(PRECISION_TPAD_MAX_CONTACT_CT * 5) + 3] = (byNumTouchesIs == 0) ? 1 : byNumTouchesIs;

    // Buttons
    /* Button clicks (press) are controlled by us, the host handles 'tap' (touch) clicks */
    usb_hid_mouse_report_in[(PRECISION_TPAD_MAX_CONTACT_CT * 5) + 4] = 0;

    u34_TCP_report[1] = 0x00;   // "consumes" the report so it isn't used again

    boMouseReportToSend = 1;
}

/*-----------------------------------------------------------*/

static void PrepareRelMouseReport(enum en_RelativeMouseEvents ContactEvent)
{
    // Always need the previous frames touch coordinates
    static uint16_t PreviousFrameX = 32767;
    static uint16_t PreviousFrameY = 32767;

    // Reset these flags
    g_TouchPadInfo.RelMouseInfo.AxisX.CursorHasMovedThisFrame = false;
    g_TouchPadInfo.RelMouseInfo.AxisY.CursorHasMovedThisFrame = false;

    // Get the coordinates from this frame
    uint16_t ThisFrameX = DigitizerXCoord;
    uint16_t ThisFrameY = DigitizerYCoord;

    // If it's a new contact (appeared this frame), update the previous coordinates to use
    // this position.
    if (ContactEvent == eNewContact)
    {
        PreviousFrameX = ThisFrameX;
        PreviousFrameY = ThisFrameY;
    }

    int32_t XDiff = (int32_t)ThisFrameX - (int32_t)PreviousFrameX;
    int8_t MovementX = ScaleRelMouseMovement(&g_TouchPadInfo.RelMouseInfo.AxisX, XDiff);
    g_TouchPadInfo.RelMouseInfo.AxisX.CursorHasMovedThisFrame = (MovementX != 0) ? true : false;

    int32_t YDiff = (int32_t)ThisFrameY - (int32_t)PreviousFrameY;
    int8_t MovementY = ScaleRelMouseMovement(&g_TouchPadInfo.RelMouseInfo.AxisY, YDiff);
    g_TouchPadInfo.RelMouseInfo.AxisY.CursorHasMovedThisFrame = (MovementY != 0) ? true : false;

    // Populate the report
    usb_hid_mouse_report_in[0] = REPORT_REL_MOUSE;
    usb_hid_mouse_report_in[1] = button_state;
    usb_hid_mouse_report_in[2] = MovementX;
    usb_hid_mouse_report_in[3] = MovementY;

    // Finally, cache the touch coordinates from the previous frame
    PreviousFrameX = ThisFrameX;
    PreviousFrameY = ThisFrameY;

    if (ContactEvent == eNoContacts)
    {
        // All contacts removed, clear the movement caches
        g_TouchPadInfo.RelMouseInfo.AxisX.MovementCache = 0;
        g_TouchPadInfo.RelMouseInfo.AxisY.MovementCache = 0;
    }
}

/*-----------------------------------------------------------*/

static int8_t ScaleRelMouseMovement(struct RelMouse_AxisInfo *pAxisInfo, int32_t PreScaleDistance)
{
    int32_t ScaledDistance = 0;

    // For small movements, a contact needs to move further to actually move the cursor.
    uint32_t AbsPreScaleDistance = abs(PreScaleDistance);
    if (AbsPreScaleDistance < 100)
    {
        pAxisInfo->MovementCache += PreScaleDistance;
        if (pAxisInfo->MovementCache > 100)
        {
            ScaledDistance = 1;
            pAxisInfo->MovementCache = 0;
            pAxisInfo->CursorHasMoved = true;
        }
        else if (pAxisInfo->MovementCache < -100)
        {
            ScaledDistance = -1;
            pAxisInfo->MovementCache = 0;
            pAxisInfo->CursorHasMoved = true;
        }
    }
    else
    {
        pAxisInfo->MovementCache = 0;
        pAxisInfo->CursorHasMoved = true;

        ScaledDistance = PreScaleDistance / 50;

        // Clamp between range of values that can be reported by a mouse
        ScaledDistance = (ScaledDistance > REL_MOUSE_MAX_POS_CHANGE) ?
                            REL_MOUSE_MAX_POS_CHANGE : ((ScaledDistance < REL_MOUSE_MAX_NEG_CHANGE) ?
                                    REL_MOUSE_MAX_NEG_CHANGE : (int8_t)ScaledDistance);
    }

    return ((int8_t)ScaledDistance);
}

/*-----------------------------------------------------------*/

static void PrepareAbsMouseReport(void)
{
    uint16_t TempX;
    uint16_t TempY;

    TempX = DigitizerXCoord >> 4;
    TempY = DigitizerYCoord >> 4;

    usb_hid_mouse_report_in[0] = (0xF8 | button_state);

    // send x coordinates
    usb_hid_mouse_report_in[1] = TempX & 0xFF;
    usb_hid_mouse_report_in[2] = TempX >> 8;

    // then send y coordinates
    usb_hid_mouse_report_in[3] = TempY & 0xFF;
    usb_hid_mouse_report_in[4] = TempY >> 8;

    boMouseReportToSend = 1;
}

/*-----------------------------------------------------------*/

static void SendMouseRightClick(void)
{
    // send button press -> wait 30ms -> send button release
    button_state = MOUSE_RIGHT_BUTTON;
    PrepareAbsMouseReport();
    Send_USB_Report(MOUSE, &hUsbDeviceFS, usb_hid_mouse_report_in, byMouseReportLength);

    HAL_Delay(30);

    button_state = MOUSE_NO_BUTTONS;
    PrepareAbsMouseReport();
    Send_USB_Report(MOUSE, &hUsbDeviceFS, usb_hid_mouse_report_in, byMouseReportLength);
}

/*-----------------------------------------------------------*/

static uint8_t GetXYZFromReport(bool boIgnoreCoords, uint8_t byTouchToProcess)
{
    uint32_t Temp = 0;
    uint8_t byTouchReportDetectState;

    byTouchToProcess--; // our touch indexes start at 1, but arrays start at 0!

    /* grab the touch coordinates */
    byReportX_msb = u34_TCP_report[5 + (byTouchToProcess*4)];
    byReportX_lsb = u34_TCP_report[4 + (byTouchToProcess*4)];
    byReportY_msb = u34_TCP_report[7 + (byTouchToProcess*4)];
    byReportY_lsb = u34_TCP_report[6 + (byTouchToProcess*4)];
    byReportZ_lsb = u34_TCP_report[44 + (byTouchToProcess*1)];
    byReportZ_msb = (byReportZ_lsb & 0x80) ? 0xFF : 0x00;   // sign extends the z value (which is only 8-bits long)

    byTouchReportDetectState = ((u34_TCP_report[2] | ((u34_TCP_report[3] << 8))) & (1 << byTouchToProcess)) ? 0x20 : 0x00;  // determines if we have a valid touch

    // these 2 bytes hold the Zlvl bits
    Temp |= (uint16_t)(u34_TCP_report[56]) | ((uint16_t)(u34_TCP_report[57]) << 8);
    Temp |= ((uint16_t)(u34_TCP_report[58]) | ((uint16_t)(u34_TCP_report[59]) << 8)) << 16;

    if(boIgnoreCoords == DO_NOT_IGNORE_COORDS) // sometimes we don't want to use the coordinates we're processing (e.g. second 'click' in mouse mode) so can choose to ignore them
    {
        DigitizerXCoord = ((uint16_t)(byReportX_msb) << 8) | byReportX_lsb;
        DigitizerYCoord = ((uint16_t)(byReportY_msb) << 8) | byReportY_lsb;
    }

    return byTouchReportDetectState;
}

/*-----------------------------------------------------------*/

static inline uint16_t CRC16_IBM_8005(uint16_t CRCIn, uint8_t DataIn)
{
    uint8_t Idx;

    Idx = (uint8_t) ((CRCIn ^ DataIn) & 0xFF);
    return((CRCIn >> 8) ^ CRC16_IBM_8005_Table[Idx]);
}

/*-----------------------------------------------------------*/
