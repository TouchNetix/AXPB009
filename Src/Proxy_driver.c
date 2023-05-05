/*******************************************************************************
* @file           : Proxy_driver.c
* @author         : James Cameron (TouchNetix)
* @date           : 7 Oct 2020
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
#include "stm32f0xx.h"
#include <stdbool.h>
#include "Proxy_driver.h"
#include "Init.h"
#include "Comms.h"
#include "Command_Processor.h"
#include "SPI_comms.h"
#include "usbd_conf.h"
#include "usbd_generic.h"
#include "Digitizer.h"
#include "Mode_Control.h"
#include "Usage_Builder.h"

/*============ Defines ============*/
#define NO_DATA        (0)
#define GOT_DATA     (1)
#define POLLING     (0x00) // polls the connected device periodically --> timer based
#define INTERRUPT   (0x03) // waits for connected device to pull nIRQ line low --> indicates report available
#define READ        (0x80)
#define PROXY_FLAG  (0x9Au) // "magic flag" for repeat proxy data

/*============ Local Variables ============*/

/*============ Exported Variables ============*/
volatile bool boProxyReportAvailable    = 0;    // indicates aXiom has a report ready
bool    boProxyEnabled                  = 0;    // does what is says on the tin really
bool    boProxyReportToProcess          = 0;    // status flag if the report has been read off aXiom yet
bool    boInternalProxy                 = 0;    // flag to say whether proxy mode has been triggered internally or by the host --> reports shouldn't come out of the generic endpoint unless proxy requested by host!

//-------------- Multipage Read --------------
bool     boReadInProgress           = 0;    // indicates whether we're already doing a read (prevents parameters being set again)
uint16_t ProxyMP_TotalNumBytesRx    = 0;    // total number of bytes that will be read from connected device
uint8_t  byProxyMP_PageLength       = 0;    // legnth of a page in aXiom
uint16_t wdProxyMP_AddrStart        = 0;    // multi-page start target address
uint16_t wdProxyMP_BytesRead        = 0;    // variable keeps track of how many bytes in we are

/*============ Local Function Prototypes ============*/

/*============ Local Functions ============*/

/*============ Exported Functions ============*/
void InitProxyInterruptMode(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin: nIRQ_Pin --> GPIOA0 */
    GPIO_InitStruct.Pin  = nIRQ_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(nIRQ_GPIO_Port, &GPIO_InitStruct);
}

/*-----------------------------------------------------------*/

/* De-initialise GPIO pin: nIRQ_Pin --> GPIOA 0 */
void DeInitProxyInterruptMode(void)
{
    HAL_GPIO_DeInit(GPIOA, nIRQ_Pin);
}

/*-----------------------------------------------------------*/
/* @brief: Controls the operation of autonomously reading and reporting touch reports from connected device
 * @param: boMultiPageRead if doing a block read (3D drawing) this is true, allows re-use of this function
 *
 */
bool ProxyExecute(bool boMultiPageRead)
{
    uint8_t status = 0;

    if((ProxyMP_TotalNumBytesRx == 0) && (boProxyEnabled == 0) && (boInternalProxy == 0))
    {
        status = NO_DATA;
    }
    else
    {
        // Enters this off the nIRQ signal when in proxy mode OR if in multipage read mode
        if((boProxyReportAvailable == 1) || ((boMultiPageRead == true) && (boReadInProgress == 0)))
        {
            if((boProxyEnabled == 1) || (boInternalProxy == 1))
            {
                memset(aXiom_Tx_Buffer, 0, sizeof(aXiom_Tx_Buffer));

                aXiom_NumBytesTx   = NUMPROXYBYTES_TX;          // num bytes to send
                aXiom_NumBytesRx   = NUMPROXYBYTES_RX;          // num bytes to read (1 report)
                aXiom_Tx_Buffer[0] = u34_addr & 0x00FF;         // offset of u34
                aXiom_Tx_Buffer[1] = (u34_addr & 0xFF00) >> 8;  // page address of u34
                aXiom_Tx_Buffer[2] = NUMPROXYBYTES_RX;          // proxy mode so hard code to read 64 bytes
                aXiom_Tx_Buffer[3] = 0x00 | READ;
            }
            else if(boMultiPageRead == true)
            {
                aXiom_Tx_Buffer[2] = (aXiom_NumBytesRx & 0x00FF);    // in case host asks for more than 64 bytes (larger than endpoint)
                aXiom_Tx_Buffer[3] = ((aXiom_NumBytesRx & 0xFF00) >> 8) | READ;  // read/write byte
            }

            if(Comms_Sequence() == HAL_OK)
            {
                // If doing a multi-page read we don't want to copy read data to report buffer
                if(boMultiPageRead == true)
                {
                    boReadInProgress = 1; // flag used when in multi-page read mode, means we don't come in here again once a read as started
                }
                else
                {
                    memcpy(u34_TCP_report, &aXiom_Rx_Buffer[CircularBufferHead][2], aXiom_NumBytesRx); // copies from 3rd byte as first 2 have already been reserved for status header
                    CRC_Checksum();
                }
            }
            else
            {
                boReadInProgress = 0;
            }

            boProxyReportAvailable = 0; // report has been extracted from the connected device so clear the flag
        }

        // report has been received, set status bytes and then flag to send report to host
        if(boProxyReportToProcess == 1)
        {
            aXiom_Rx_Buffer[CircularBufferHead][0] = PROXY_FLAG; // "magic flag" for repeat proxy data
            aXiom_Rx_Buffer[CircularBufferHead][1] = COMMS_OK; // Comms_status = all OK, read data

            boProxyReportToProcess = 0;
            boReadInProgress = 0;

            status = GOT_DATA;
        }
        else
        {
            /* don't do anything! */
        }
    }

    return status;
}
