/*******************************************************************************
* @file           : Usage_Builder.h
* @author         : James Cameron (TouchNetix)
* @date           : 20 Oct 2020
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

#ifndef USAGE_BUILDER_H_
#define USAGE_BUILDER_H_

/*============ Includes ============*/
#include "stm32f0xx.h"
#include <stdbool.h>

/*============ Defines ============*/
#define START                       (1)
#define STOP                        (0)
#define MAX_NUM_USAGES              (83)  // This is assuming the maximum no. usages fits on 2 pages (may change with future hardware revisions/releases)
#define NO_WAKE                     (0x00)
#define WAKE_ON_TOUCH               (0x01)
#define WAKE_ON_TOUCH_HOVER         (0x03)
#define WAKE_ON_TOUCH_HOVER_PROX    (0x07)

/*============ Exported Structures ============*/
struct usagetableentry_st
{
    uint8_t usagenum;
    uint8_t startpage;
    uint8_t numpages;
    uint8_t maxoffset;
    uint8_t uifrevision;
    uint8_t ufwrevision;
};


extern struct usagetableentry_st usagetable[MAX_NUM_USAGES];

/*============ Exported Variables ============*/
extern uint16_t u34_addr;
extern uint16_t u35_addr;
extern uint8_t  WakeupMode;

/*============ Exported Function Prototypes ============*/
uint8_t build_usage_table(void);
int8_t  find_usage_from_table(uint8_t byUsagenum);
void    configure_HID_PARAMETER_IDs(void);
void    adjust_descriptors_from_HID_PARAMETER_IDs(void);


#endif /* USAGE_BUILDER_H_ */
