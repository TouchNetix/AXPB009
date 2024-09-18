/*******************************************************************************
* @file           : Mode_Control.h
* @author         : James Cameron (TouchNetix)
* @date           : 29 Oct 2020
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

#ifndef MODE_CONTROL_H_
#define MODE_CONTROL_H_

/*============ Includes ============*/
#include "stm32f0xx.h"
#include <stdbool.h>

/*============ Exported Variables ============*/
extern  bool    boMouseEnabled;
extern  bool    boBlockReports;

/*============ Exported Defines ============*/
#define NRESET_OUTPUT               (0U)
#define NRESET_INPUT                (1U)

#define USBMODE_IDLE                (0U)
#define I2CMODE_IDLE                (1U)
#define NRESETACTIVITYDETECTED_USB  (2U)
#define NRESETACTIVITYDETECTED_I2C  (3U)

#define ACTIVITY_DETECTED           (0U)
#define RESET_PULSE_DETECTED        (1U)
#define RESET_WINDOW_ELAPSED        (2U)

/*============ Exported Functions ============*/
void        Device_DeInit(bool DoDelay);
bool        InMouseOrDigitizerMode(void);
void        RestartBridge(void);
bool        WakeupHost(uint8_t ByNumTouches, uint8_t byReportZ);
void        Reset_aXiom(void);
void        Configure_nRESET(uint8_t mode);
uint8_t     Get_Bridge_Comms_State(void);
void        Bridge_Comms_Switch_State_Machine(uint8_t Action);

#endif /* MODE_CONTROL_H_ */
