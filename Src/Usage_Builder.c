/*******************************************************************************
* @file           : Usage_Builder.c
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


/*============ Includes ============*/
#include "stm32f0xx.h"
#include "Init.h"
#include "usbd_desc.h"
#include "Comms.h"
#include "Command_Processor.h"
#include "Usage_Builder.h"
#include "SPI_comms.h"
#include "Proxy_driver.h"
#include "Digitizer.h"
#include "usbd_mouse_if.h"

/*============ Defines ============*/
#define u34                         (0x34u)
#define u35                         (0x35u)
#define GOT_DATA                     (0x01u)
#define NO_DATA                        (0x0Au)
#define WRITE                       (0x00u)
#define READ                        (0x80u)
#define MAX_NUM_HID_PARAMETERS      (16)
#define VID                         (1)
#define PID                         (2)
#define PHYS_X                      (3)
#define PHYS_Y                      (4)
#define LOGMAX_X                    (5)
#define LOGMAX_Y                    (6)
#define WAKEUP_OPTION               (7)
#define PHYS_X_FIRST_TOUCH_BYTELO   (58)
#define PHYS_X_FIRST_TOUCH_BYTE2    (59)
#define PHYS_X_FIRST_TOUCH_BYTE3    (60)
#define PHYS_X_FIRST_TOUCH_BYTEHI   (61)
#define PHYS_Y_FIRST_TOUCH_BYTELO   (72)
#define PHYS_Y_FIRST_TOUCH_BYTE2    (73)
#define PHYS_Y_FIRST_TOUCH_BYTE3    (74)
#define PHYS_Y_FIRST_TOUCH_BYTEHI   (75)
#define LOGMAX_X_FIRST_TOUCH_LO     (53)
#define LOGMAX_X_FIRST_TOUCH_HI     (54)
#define LOGMAX_Y_FIRST_TOUCH_LO     (67)
#define LOGMAX_Y_FIRST_TOUCH_HI     (68)
#define ARRAY_CONST                 (78)    // number of bytes between touches in digitizer report descriptor
#define MAX_RETRY_NUM               (200)

/*============ Macros ============*/
#define HID_PARAMETER_ID(n)     (2 + (n * 3)) // 2 byte offset needed as first 2 bytes contain comms status and no. Rx bytes
                                               // n is the field ID index (maps to those found in aXiom config)

/*============ Local Variables ============*/
uint8_t numusages = 0;
uint16_t UserVID = 0;
uint16_t UserPID = 0;
uint16_t LogMaxX = 0;
uint16_t LogMaxY = 0;
uint32_t PhysMaxX = 0;
uint32_t PhysMaxY = 0;
bool boUsageTablePopulated = 0;
bool boCustomVIDUsed = 0;
bool boCustomPIDUsed = 0;
bool boPhysicalSensorSizeDefined_X = 0;
bool boPhysicalSensorSizeDefined_Y = 0;
bool boLogicalMaxDefined_X = 0;
bool boLogicalMaxDefined_Y = 0;

/*============ Exported Typedefs ============*/
struct usagetableentry_st usagetable[MAX_NUM_USAGES];

/*============ Exported Variables ============*/
uint16_t u34_addr;
uint16_t u35_addr;

/*============ Local Function Prototypes ============*/

/*============ Local Functions ============*/

/*============ Exported Functions ============*/
/* finds the index of the usage from the stored usage table
 * @param byUsagenum: desired usage
 * @return table index - returns negative 1 if not found
 */
int8_t find_usage_from_table(uint8_t byUsagenum)
{
    uint8_t usage_table_idx;

    if(boUsageTablePopulated == 0)
    {
        build_usage_table();
    }

    for(usage_table_idx = 0; usage_table_idx < numusages; usage_table_idx++)
    {
        if(usagetable[usage_table_idx].usagenum == byUsagenum)
            return(usage_table_idx);
    }
    return(-1); // usage wasn't found so return this value
}

//--------------------------

/* builds the usage table from aXiom at startup
 * @return Status
 */
uint8_t build_usage_table(void)
{
    uint8_t aXiomIsAlive = 0;
    uint8_t UsagesLeftToRead;
    uint8_t BytesOffset = 0;
    uint8_t byPagesMovedThrough;
    uint8_t status;

    uint16_t wdUsageTable_StartingAddress = 0x0100;   // this is where the start of the usage tables on aXiom are found
    uint16_t usage_table_idx = 0;

    // check the nIRQ line to see if aXiom is able to talk
    // pin will be asserted (low) if ready
    if(HAL_GPIO_ReadPin(nIRQ_GPIO_Port, nIRQ_Pin) == 0)
    {
        aXiom_NumBytesTx = 4u;
        aXiom_NumBytesRx = 12u;   // read first 12 bytes to get device info and total no. usages

        /* sets up the tx buffer */
        //Address 0x0000
        aXiom_Tx_Buffer[0] = 0x00;
        aXiom_Tx_Buffer[1] = 0x00;
        //Read 12 bytes
        aXiom_Tx_Buffer[2] = aXiom_NumBytesRx;
        aXiom_Tx_Buffer[3] = READ;

        /* get device info --> number of usages used */
        status = Comms_Sequence();  // do a transfer
        if(status == HAL_ERROR)
        {
            return status;
        }

        numusages = aXiom_Rx_Buffer[0][10]; // this byte tells us how many usages are being used
        if(numusages > MAX_NUM_USAGES)  // fail-safe in case we do a misread --> would lead to us reading too many usages and possibly reading another memory location!
        {
            numusages = MAX_NUM_USAGES;
        }

        //Address 0x0100 --> this is the starting address of where all the usages are
        aXiom_Tx_Buffer[0] = (uint8_t)(wdUsageTable_StartingAddress & 0xFF);
        aXiom_Tx_Buffer[1] = (uint8_t)((wdUsageTable_StartingAddress >> 8) & 0xFF);

        aXiom_NumBytesRx = 6; // number of bytes being read --> a usage is 6 bytes long
        aXiom_NumBytesTx = 4u;
        usage_table_idx = 0;
        UsagesLeftToRead = numusages;
        byPagesMovedThrough = 0;

        memset((uint8_t *)usagetable, 0, sizeof(usagetable));   // clear the table

        // can check here for presence of axiom/if axiom is online
        //  - if axiom is connected but 'dead' or disconnected the bridge will read back all 0xFF
        //  - run a check over all these bytes, if all 12 bytes read back as FF then return an error
        for(uint8_t i = 0; i < 12; i++)
        {
            if(aXiom_Rx_Buffer[0][i+2] != 0xFF || aXiom_Rx_Buffer[0][i+2] != 0x00) //first 2 bytes are status and no.bytes
            {
                // making a guess, if a byte is NOT 0xFF we assume aXiom is present and alive
                aXiomIsAlive++;
            }
        }

        // didn't read anything useful from aXiom - probably not there
        if(aXiomIsAlive == 0)
        {
            status = HAL_ERROR;
            UsagesLeftToRead = 0; // means we won't enter the while loop
        }
        else
        {
            /* Reading one usage at a time:
             * it would be possible to do a max read (64 bytes) each time and store them all in the struct array (addressed by a pointer)
             * however this would require a union, and would be slightly harder to understand what's happening! (plus when the usage table spans 2 pages it would be a bit nasty due to the n ignored bytes between page 1 and 2)
             * Instead, each time this do-while is looped through we grab the next 6 bytes and store in the next struct array entry
             *
             * Each usage table page can store a maximum of 41 usages, so keep track of how many usages we're reading then jump to page 2 (0x0200) when needed */
            while(UsagesLeftToRead != 0)
            {
                aXiom_Tx_Buffer[2] = aXiom_NumBytesRx;    //Read 6 bytes
                aXiom_Tx_Buffer[3] = READ;

                /* Read Usages */
                status = Comms_Sequence();  // do a transfer
                if(status == HAL_ERROR)
                {
                    break;
                }

                /* save the data just read into the usage table structure --> usage_table_idx is used to keep track of how many bytes we've read so we start the next write at the correct point next time */
                memcpy(&usagetable[usage_table_idx], &aXiom_Rx_Buffer[0][2], aXiom_NumBytesRx);

                BytesOffset += aXiom_NumBytesRx;    // keeps track of how many bytes we've read

                /*=====various checks if we have found certain usages/page limits=====*/
                // Usage 34 -TCP reports
                if(usagetable[usage_table_idx].usagenum == u34)
                {
                    u34_addr = (usagetable[usage_table_idx].startpage & 0xFF) << 8;
                }
                // Usage 35 - HID Parameters
                if(usagetable[usage_table_idx].usagenum == u35)
                {
                    u35_addr = (usagetable[usage_table_idx].startpage & 0xFF) << 8;
                }

                // reached the end of the page 1, read from page 2 with correct offset
                if(usage_table_idx == 42)
                {
                    byPagesMovedThrough = 1;
                    BytesOffset = 2;
                }

                UsagesLeftToRead--; // read a usage so decrement this value
                usage_table_idx++;  // increment to next usage

                //set the new starting address and offset (e.g. start at page 1, offset 0 --> read 6, so new starting address is 0x0106)
                aXiom_Tx_Buffer[0] = (uint8_t)((wdUsageTable_StartingAddress & 0xFF) + BytesOffset);
                aXiom_Tx_Buffer[1] = (uint8_t)((wdUsageTable_StartingAddress >> 8) + byPagesMovedThrough);

                boUsageTablePopulated = 1;
            }
        }
    }
    else
    {
        // aXiom isn't ready yet, wait and try again
        HAL_Delay(10);
        status = HAL_ERROR;
    }

    return status;
}

//--------------------------

void configure_HID_PARAMETER_IDs(void)
{
    bool boCustomParameterSet = 0;
    uint8_t  parameter_count = 0; // keeps track of how many parameters we've read
    uint16_t PhysMaxX_Temp = 0;
    uint16_t PhysMaxY_Temp = 0;


    // set comms parameters
    // set target address to start reading from - u35 holds various parameters needed for HID descriptors such as VID, PID, sensor dimension etc.
    aXiom_NumBytesRx = MAX_NUM_HID_PARAMETERS * 3; // each parameter consists of 1 ID field byte and 2 value field bytes --> each takes up 3 bytes of space
    aXiom_Tx_Buffer[0] = u35_addr & 0x00FF;
    aXiom_Tx_Buffer[1] = (u35_addr & 0xFF00) >> 8;
    aXiom_Tx_Buffer[2] = aXiom_NumBytesRx;
    aXiom_Tx_Buffer[3] = READ;

    Comms_Sequence();

    /* Read from aXiom */
    while(parameter_count < MAX_NUM_HID_PARAMETERS)
    {
        // first byte contains the ID of the parameter
        switch(aXiom_Rx_Buffer[0][HID_PARAMETER_ID(parameter_count)])
        {
            case VID:
            {
                UserVID = (uint16_t)((aXiom_Rx_Buffer[0][2 + HID_PARAMETER_ID(parameter_count)] << 8) | aXiom_Rx_Buffer[0][1 + HID_PARAMETER_ID(parameter_count)]);
                boCustomVIDUsed = 1;
                boCustomParameterSet = 1;
                break;
            }
            case PID:
            {
                UserPID = (uint16_t)((aXiom_Rx_Buffer[0][2 + HID_PARAMETER_ID(parameter_count)] << 8) | aXiom_Rx_Buffer[0][1 + HID_PARAMETER_ID(parameter_count)]);
                boCustomPIDUsed = 1;
                boCustomParameterSet = 1;
                break;
            }
            case PHYS_X:
            {
                PhysMaxX_Temp = (uint16_t)((aXiom_Rx_Buffer[0][2 + HID_PARAMETER_ID(parameter_count)] << 8) | aXiom_Rx_Buffer[0][1 + HID_PARAMETER_ID(parameter_count)]);
                PhysMaxX = PhysMaxX_Temp * 5;   // multiply the read value by 5 to produce the value requested by the user --> TH2 assumes this value increases in steps of 0.5mm, but we increase it in steps of 0.1mm
                boPhysicalSensorSizeDefined_X = 1;
                boCustomParameterSet = 1;
                break;
            }
            case PHYS_Y:
            {
                PhysMaxY_Temp = (uint16_t)((aXiom_Rx_Buffer[0][2 + HID_PARAMETER_ID(parameter_count)] << 8) | aXiom_Rx_Buffer[0][1 + HID_PARAMETER_ID(parameter_count)]);
                PhysMaxY = PhysMaxY_Temp * 5;   // multiply the read value by 5 to produce the value requested by the user --> TH2 assumes this value increases in steps of 0.5mm, but we increase it in steps of 0.1mm
                boPhysicalSensorSizeDefined_Y = 1;
                boCustomParameterSet = 1;
                break;
            }
            case LOGMAX_X:
            {
                LogMaxX = (uint16_t)((aXiom_Rx_Buffer[0][2 + HID_PARAMETER_ID(parameter_count)] << 8) | aXiom_Rx_Buffer[0][1 + HID_PARAMETER_ID(parameter_count)]);
                boLogicalMaxDefined_X = 1;
                boCustomParameterSet = 1;
                break;
            }
            case LOGMAX_Y:
            {
                LogMaxY = (uint16_t)((aXiom_Rx_Buffer[0][2 + HID_PARAMETER_ID(parameter_count)] << 8) | aXiom_Rx_Buffer[0][1 + HID_PARAMETER_ID(parameter_count)]);
                boLogicalMaxDefined_Y = 1;
                boCustomParameterSet = 1;
                break;
            }
            case WAKEUP_OPTION:
            {
                WakeupMode = aXiom_Rx_Buffer[0][1 + HID_PARAMETER_ID(parameter_count)];
            }
            default:    // any other ID field means not used
            {
                break;
            }
        }
        parameter_count++;  // track how many parameter fields we've checked
    }

    // if a user has define a parameter we need to change some descriptors before starting the USB
    if(boCustomParameterSet == 1)
    {
        adjust_descriptors_from_HID_PARAMETER_IDs();
    }
}

//--------------------------

/* Checks if the user has set any of these parameters and adjusts them as necessary by manipulating relevant array structures */
void adjust_descriptors_from_HID_PARAMETER_IDs(void)
{
    uint8_t touch_num = 0;

    if(boCustomVIDUsed)
    {
        USBD_FS_DeviceDesc[8] = LOBYTE(UserVID);
        USBD_FS_DeviceDesc[9] = HIBYTE(UserVID);
    }

    if(boCustomPIDUsed)
    {
        USBD_FS_DeviceDesc[10] = LOBYTE(UserPID);
        USBD_FS_DeviceDesc[11] = HIBYTE(UserPID);
    }

    if(boPhysicalSensorSizeDefined_X)
    {
        // sets physical dimension X for each touch
        /* ARRAY_CONST is the number of array elements between touches - allows for cleaner code! */
        for(touch_num = 0; touch_num < 5; touch_num++)
        {
            mouse_parallel_digitizer_ReportDesc_FS[PHYS_X_FIRST_TOUCH_BYTELO + (touch_num * ARRAY_CONST)] = (uint8_t)((PhysMaxX & 0x000000FF) >> 0);  // Low byte
            mouse_parallel_digitizer_ReportDesc_FS[PHYS_X_FIRST_TOUCH_BYTE2  + (touch_num * ARRAY_CONST)] = (uint8_t)((PhysMaxX & 0x0000FF00) >> 8);  // Second byte
            mouse_parallel_digitizer_ReportDesc_FS[PHYS_X_FIRST_TOUCH_BYTE3  + (touch_num * ARRAY_CONST)] = (uint8_t)((PhysMaxX & 0x00FF0000) >> 16); // Third byte
            mouse_parallel_digitizer_ReportDesc_FS[PHYS_X_FIRST_TOUCH_BYTEHI + (touch_num * ARRAY_CONST)] = (uint8_t)((PhysMaxX & 0xFF000000) >> 24); // High byte
        }
    }

    if(boPhysicalSensorSizeDefined_Y)
    {
        // sets physical dimension Y for each touch
        for(touch_num = 0; touch_num < 5; touch_num++)
        {
            mouse_parallel_digitizer_ReportDesc_FS[PHYS_Y_FIRST_TOUCH_BYTELO + (touch_num * ARRAY_CONST)] = (uint8_t)((PhysMaxY & 0x000000FF) >> 0);  // Low byte
            mouse_parallel_digitizer_ReportDesc_FS[PHYS_Y_FIRST_TOUCH_BYTE2  + (touch_num * ARRAY_CONST)] = (uint8_t)((PhysMaxY & 0x0000FF00) >> 8);  // Second byte
            mouse_parallel_digitizer_ReportDesc_FS[PHYS_Y_FIRST_TOUCH_BYTE3  + (touch_num * ARRAY_CONST)] = (uint8_t)((PhysMaxY & 0x00FF0000) >> 16); // Third byte
            mouse_parallel_digitizer_ReportDesc_FS[PHYS_Y_FIRST_TOUCH_BYTEHI + (touch_num * ARRAY_CONST)] = (uint8_t)((PhysMaxY & 0xFF000000) >> 24); // High byte
        }
    }

    if(boLogicalMaxDefined_X)
    {
        // sets max logical X for each touch
        for(touch_num = 0; touch_num < 5; touch_num++)
        {
            mouse_parallel_digitizer_ReportDesc_FS[LOGMAX_X_FIRST_TOUCH_LO + (touch_num * ARRAY_CONST)] = LOBYTE(LogMaxX); // Low byte
            mouse_parallel_digitizer_ReportDesc_FS[LOGMAX_X_FIRST_TOUCH_HI + (touch_num * ARRAY_CONST)] = HIBYTE(LogMaxX); // High byte
        }
    }

    if(boLogicalMaxDefined_Y)
    {
        // sets max logical Y for each touch
        for(touch_num = 0; touch_num < 5; touch_num++)
        {
            mouse_parallel_digitizer_ReportDesc_FS[LOGMAX_Y_FIRST_TOUCH_LO + (touch_num * ARRAY_CONST)] = LOBYTE(LogMaxX); // Low byte
            mouse_parallel_digitizer_ReportDesc_FS[LOGMAX_Y_FIRST_TOUCH_HI + (touch_num * ARRAY_CONST)] = HIBYTE(LogMaxX); // High byte
        }
    }
}
