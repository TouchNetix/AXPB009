/*******************************************************************************
* @file           : Timers.h
* @author         : James Cameron (TouchNetix)
* @date           : 15 Jan 2021
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

#ifndef TIMERS_AND_LEDS_H_
#define TIMERS_AND_LEDS_H_

/*============ Includes ============*/
#include "stm32f0xx.h"
#include <stdbool.h>

/*============ Exported Variables ============*/
extern uint32_t aXiom_activity_counter;
extern uint32_t USB_activity_counter;
extern bool     boAxiomActivity;
extern bool     boUSBActivity;
extern bool     boFlashAxiomLED;
extern bool     boFlashUSBLED;

/*============ Exported Functions ============*/
extern void LED_control(void);
extern void comms_detect_inactivity(void);

#endif /* TIMERS_AND_LEDS_H_ */
