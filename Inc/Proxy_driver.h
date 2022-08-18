/*******************************************************************************
* @file           : Proxy_driver.h
* @author         : James Cameron (TouchNetix)
* @date           : 7 Oct 2020
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

#ifndef PROXY_DRIVER_H_
#define PROXY_DRIVER_H_

/*============ Includes ============*/
#include "stdio.h"
#include "Comms.h"
#include <stdbool.h>
#include "usbd_conf.h"

/*============ Defines ============*/
#define NUMBYTES_RX_MP      (58)
#define NUMPROXYBYTES_TX     (4)
#define NUMPROXYBYTES_RX    (64)

/*============ Exported Variables ============*/
extern volatile bool    boProxyReportAvailable;
extern          bool    boProxyEnabled;
extern          bool    boProxyReportToProcess;
extern          bool    boInternalProxy;

//-------------- Multipage Read --------------
extern bool     boReadInProgress;
extern uint16_t ProxyMP_TotalNumBytesRx;
extern uint8_t  byProxyMP_PageLength;
extern uint16_t wdProxyMP_AddrStart;
extern uint16_t wdProxyMP_BytesRead;

/*============ Exported Functions ============*/
void InitProxyInterruptMode(void);
void DeInitProxyInterruptMode(void);
bool ProxyExecute(bool boMultiPageRead);

#endif /* PROXY_DRIVER_H_ */
