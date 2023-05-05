/*******************************************************************************
* @file           : SPI_comms.h
* @author         : James Cameron (TouchNetix)
* @date           : 22 Sep 2020
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

#ifndef INC_SPI_COMMS_H_
#define INC_SPI_COMMS_H_

/*============ Include ============*/
#include "stm32f0xx.h"
#include "Comms.h"

/*============ Defines ============*/
#define TRANSMIT_AND_RECEIVE    (1)
#define WAIT_FOR_FLAG           (2)

/*============ Exported Variables ============*/
extern  uint8_t spi_state;
extern  uint8_t SPI_Speed_PreScaler;

/*============ Exported Functions ============*/
void MX_SPI_Init(void);
uint8_t do_spi_comms(void);

#endif /* INC_SPI_COMMS_H_ */
