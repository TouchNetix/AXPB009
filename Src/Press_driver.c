/*******************************************************************************
* @file           : Press_driver.c
* @author         : James Cameron (TouchNetix)
* @date           : 16 Nov 2020
*******************************************************************************/

/*
******************************************************************************
* Copyright (c) 2022 TouchNetix
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
#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"
#include "stdio.h"
#include <stdbool.h>
#include "Mode_Control.h"
#include "Digitizer.h"
#include "usbd_press_if.h"

/*============ Defines ============*/
#define NOT_USED                        (0x00)
#define IDFIELD_TOUCHSCREENDATA         (0x03)
#define IDFIELD_TOUCHXYDATA             (0x02)
#define IDFIELD_ENDOFLIST               (0x00)
#define PAYLOADLENGTH_TOUCHSCREENDATA   (2)
#define PAYLOADLENGTH_TOUCHXYDATA       (6)
/*============ Macros ============*/

/*============ Local Variables ============*/

/*============ Exported Variables ============*/
bool    boBlockPressReports = false;
bool    boDoPressEvent      = false;

/*============ Local Function Prototypes ============*/
static void BuildPressReport(void);

/*============ Local Functions ============*/
static void BuildPressReport(void)
{
    uint8_t ByteCount = 0;
    uint8_t TouchIdx;

    memset(usb_hid_press_report_in, 0x00, USBD_PRESS_HID_REPORT_IN_SIZE);

    /* in PB-005 we used to send the data from the 4 press channels first, then send each touch with associated XYZ coordinates --> downside of this was had to cut out region information from touches!
     * in this bridge we're only sending the touches and the associated Z amplitude (as it has already been calculated and processed in aXiom anyway)
     * order of data (each one preceeded by the corresponding ID):
     *  - No. total touches
     *  - Touch data (x5)
     *      --> TouchID (touch num. and if its present), X low, X high, Y low, Y high, Z amplitude
     *  - End of List marker
     */

    // send overall touch screen status and no. total touches - Byte count starts at 0 and increments AFTER the line has been executed
    usb_hid_press_report_in[ByteCount++] = ((PAYLOADLENGTH_TOUCHSCREENDATA << 3) | IDFIELD_TOUCHSCREENDATA);  // ID Field
    usb_hid_press_report_in[ByteCount++] = byNumTouches;  // payload
    usb_hid_press_report_in[ByteCount++] = NOT_USED;  // empty

    // Touch XYZ data
    for(TouchIdx = 1; TouchIdx <= 5; TouchIdx++)
    {
        usb_hid_press_report_in[ByteCount++] = (PAYLOADLENGTH_TOUCHXYDATA << 3 ) | IDFIELD_TOUCHXYDATA; // ID Field
        usb_hid_press_report_in[ByteCount++] = touch_data[TouchIdx - 1][0] | TouchIdx;  // status field --> whether touch is present and touch number

        for(uint8_t i = 1; i <= 5; i++) // X and Y are 2 bytes long, Z is 1 byte long = 5 bytes in total
        {
            usb_hid_press_report_in[ByteCount++] = touch_data[TouchIdx - 1][i];
        }
    }

    // set to the EndOfList ID field to indicate the end of the packet
    usb_hid_press_report_in[ByteCount] = IDFIELD_ENDOFLIST;
}

/*-----------------------------------------------------------*/

/*============ Exported Functions ============*/

void BuildPressReportFromPressAndTouch(void)
{
    bool boGotTouchReport = 0;

    if(!boBlockPressReports) // only executes if we're NOT blocking reports
    {
        if((!InMouseOrDigitizerMode()) || (boMouseEnabled == 0))    // check if any form of mouse mode is activated or not --> if not then we need to do all the touch data processing here (otherwise it's done in digitizer)
        {
            boGotTouchReport = Check_u41Report();   // checks if we are actually dealing with a u41 report or not

            if(boGotTouchReport)    // only process if report is a u41
            {
                // checks the previously fetched report and copies data for press-touch reports (up to 5 touches). If in mouse or digitizer mode, this has alreay happened
                byNumTouches = CheckTouches();

                // "consumes" the report so it isn't used again
                u34_TCP_report[1] = 0x00;
            }
        }
        else
        {
            // touch data has already been processed by digitizer so no need to do anything else
        }

        if(boDoPressEvent)  // only make a press report if the read report was a u41
        {
            BuildPressReport();
            boDoPressEvent = 0;
            boPressReportToSend = 1; // send a press report
        }
    }
}
