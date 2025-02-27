/*******************************************************************************
* @file           : Comms.c
* @author         : James Cameron (TouchNetix)
* @date           : 22 Sep 2020
*******************************************************************************/

/*
******************************************************************************
* Copyright (c) 2025 TouchNetix
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
#include "Comms.h"
#include "Command_Processor.h"
#include "SPI_comms.h"
#include "I2C_Comms.h"
#include "Proxy_driver.h"
#include "Usage_Builder.h"
#include "usbd_generic.h"
#include "usbd_generic_if.h"
#include "usb_device.h"

/*============ Defines ============*/
#define TIMEOUT_RETRY_LIMIT (10000)

/*============ Local Variables ============*/

/*============ Exported Variables ============*/
bool        boCommsInProcess = 0;          // command waiting to be sent to connected device --> starts and comms

uint32_t    CircularBufferHead  = 0;
uint32_t    CircularBufferTail = 0;

uint8_t     comms_mode = 1;

uint8_t     aXiom_Tx_Buffer[SPI_CMD_BYTES + SPI_PADDING_BYTES + USBD_GENERIC_HID_REPORT_IN_SIZE] = {0};    // Used as the Tx buffer in SPI mode, holds the CMD bytes when in proxy i2c mode (Tx bytes are reloaded from here)
uint8_t     aXiom_Rx_Buffer[MAX_NUM_RX_BUFFERS][SPI_CMD_BYTES + SPI_PADDING_BYTES + USBD_GENERIC_HID_REPORT_IN_SIZE] = {0};    // Used as Tx and Rx buffer when in I2C mode, only used for Rx when in SPI mode
                                                                                                       // 4 + 32 + 2*64 = plenty of space! doing this to prevent funky memory stuff
uint16_t    aXiom_NumBytesTx = 0; // number of bytes we're writing to aXiom
uint16_t    aXiom_NumBytesRx = 0; // number of bytes we're reading from aXiom

/*============ Local Function Prototypes ============*/

/*============ Functions ============*/
HAL_StatusTypeDef Comms_Sequence(void)
{
    uint8_t status = 0;
    uint8_t timeout_counter = 0;

    // equivalent to using memset, but this is more space efficient
    for(uint8_t i = 0; i < (SPI_CMD_BYTES + SPI_PADDING_BYTES + USBD_GENERIC_HID_REPORT_IN_SIZE); i++)
    {
        aXiom_Rx_Buffer[CircularBufferHead][i] = 0;
    }

    boCommsInProcess = 1;

    /* while loop keeps us in this until comms has finished (or exited from a timeout) */
    while(boCommsInProcess != 0)
    {
        if(comms_mode == SPI)
        {
            status = do_spi_comms();
        }
        else if(comms_mode == I2C)
        {
            status = do_i2c_comms();
        }
        else
        {
            status = HAL_ERROR;
        }

        if(status != HAL_OK)
        {
            if(status == HAL_ERROR)
            {
                aXiom_Rx_Buffer[CircularBufferHead][0] = COMMS_ERROR;
            }

            if(status == HAL_TIMEOUT)
            {
                aXiom_Rx_Buffer[CircularBufferHead][0] = COMMS_TIMEOUT;
            }

            break;
        }

        // check if spi comms have finished when leaving the function (checked regularly as this is spun through whilst boCommsInProcess is true)
        if(boCommsInProcess == 0)
        {
            if((boProxyEnabled == 1) || (boInternalProxy == 1) || (ProxyMP_TotalNumBytesRx != 0))
            {
                // SPI comms from a proxy event
                boProxyReportToProcess = 1;
            }

            status = HAL_OK;
        }

        if(timeout_counter >= TIMEOUT_RETRY_LIMIT)  // shouldn't cycle through here many times - if we're stuck in here then exit function and throw error code
        {
            timeout_counter = 0;
            boCommsInProcess = 0;
            aXiom_Rx_Buffer[CircularBufferHead][0] = COMMS_TIMEOUT;
            status = HAL_TIMEOUT;
            break;
        }

        timeout_counter++;  // increment the timeout counter
    }

    return status;
}

//--------------------------
