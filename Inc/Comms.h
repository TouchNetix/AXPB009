/*******************************************************************************
* @file           : Comms.h
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

/*============ Includes ============*/
#include <stdbool.h>
#include "stm32f0xx.h"
#include "usbd_conf.h"

/*============ Exported Defines ============*/
#define I2C                    (0U)
#define SPI                    (1U)
#define SPI_CMD_BYTES          (4U)
#define SPI_PADDING_BYTES     (32U) //(32 * sizeof(uint8_t))
#define COMMS_OK            (0x00U)
#define COMMS_ERROR         (0x01U)
#define COMMS_TIMEOUT       (0x02U)
#define COMMS_OK_NO_READ    (0x04U)

// STM32F042x6 has a much smaller flash
// increasing no. buffers above 2 seems to have no effect on performance, but since
// we have the space on the other chips it doesn't hurt to have more headroom!
#if defined(STM32F042x6)
    #define MAX_NUM_RX_BUFFERS     (2U)
#elif defined(STM32F070xB) || defined(STM32F072xB) || defined(STM32F072RB_DISCOVERY)
    #define MAX_NUM_RX_BUFFERS    (10U)
#else
#error Undefined chip being used! Please set the number of buffers that can be used (within flash constraints)
#endif


/*============ Exported Variables ============*/
extern  bool     boCommsInProcess;
extern  uint32_t CircularBufferHead;
extern  uint32_t CircularBufferTail;

extern  uint8_t  comms_mode;
extern  uint8_t  aXiom_Rx_Buffer[MAX_NUM_RX_BUFFERS][SPI_CMD_BYTES + SPI_PADDING_BYTES + USBD_GENERIC_HID_REPORT_IN_SIZE];
extern  uint8_t  aXiom_Tx_Buffer[SPI_CMD_BYTES + SPI_PADDING_BYTES + USBD_GENERIC_HID_REPORT_IN_SIZE];
extern  uint16_t aXiom_NumBytesTx;
extern  uint16_t aXiom_NumBytesRx;

/*============ Exported Function ============*/
HAL_StatusTypeDef Comms_Sequence(void);
