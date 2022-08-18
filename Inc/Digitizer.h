/*******************************************************************************
* @file           : Digitizer.h
* @author         : James Cameron (TouchNetix)
* @date           : 27 Oct 2020
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

#ifndef DIGITIZER_H_
#define DIGITIZER_H_

/*============ Includes ============*/
#include "stdio.h"
#include <stdbool.h>
#include "usbd_composite.h"
#include "Comms.h"

/*============ Defines ============*/
#define DIGITIZER_DEVICE_ADDRESS    (0x67)
#define RESUMED                     (0)
#define PENDING_WAKE                (1)
#define SUSPENDED                   (2)

/*============ Exported Variables ============*/
extern volatile uint16_t wd100usTick;
extern volatile uint16_t wdUSB1msTick;
extern          uint8_t  u34_TCP_report[USBD_GENERIC_HID_REPORT_IN_SIZE]; //SPI_CMD_BYTES + SPI_PADDING_BYTES +
extern          uint8_t  byNumTouches;
extern          uint8_t  touch_data[5][8];
extern          uint8_t  usb_remote_wake_state;
extern          bool     boUSBTimeoutEnabled;
extern          uint8_t  wakeup_option;

/*============ Exported Functions ============*/
void CRC_Checksum(void);
void MultiPointDigitizer(void);
void MouseDigitizer(void);
void setup_proxy_for_digitizer(void);
bool Check_u41Report(void);
uint8_t CheckTouches(void);

#endif /* DIGITIZER_H_ */
