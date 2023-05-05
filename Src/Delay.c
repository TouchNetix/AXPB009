/*******************************************************************************
*                                    NOTICE
*
* Copyright (c) 2010 - 2023 TouchNetix Limited
* ALL RIGHTS RESERVED.
*
* The source  code contained  or described herein  and all documents  related to
* the source code ("Material") are owned by TouchNetix Limited ("TouchNetix") or
* its suppliers  or licensors. Title to the Material  remains with TouchNetix or
* its   suppliers  and  licensors. The  Material  contains  trade  secrets   and
* proprietary  and confidential information  of TouchNetix or its  suppliers and
* licensors.  The  Material  is  protected  by  worldwide  copyright  and  trade
* secret  laws and  treaty  provisions.  No part  of the Material  may be  used,
* copied,  reproduced,  modified,   published,  uploaded,  posted,  transmitted,
* distributed or disclosed in any way without TouchNetix's prior express written
* permission.
*
* No  license under any  patent, copyright,  trade secret or other  intellectual
* property  right is granted to or conferred upon you by disclosure or  delivery
* of  the Materials, either  expressly, by implication, inducement, estoppel  or
* otherwise.  Any  license  under  such  intellectual  property rights  must  be
* expressly approved by TouchNetix in writing.
*
*******************************************************************************/

/*******************************************************************************
 * Include Directives
 ******************************************************************************/
#include "Delay.h"

/*******************************************************************************
 * File Scope Conditional Compilation Flags
 ******************************************************************************/


/*******************************************************************************
 * File Scope Data Types
 ******************************************************************************/


/*******************************************************************************
 * Exported Variables
 ******************************************************************************/


/*******************************************************************************
 * Exported Constants
 ******************************************************************************/


/*******************************************************************************
 * File Scope Constants
 ******************************************************************************/
#if defined(STM32F070xB)
#define NOPS_FOR_1US_SLEEP  (24U)
#else
#define NOPS_FOR_1US_SLEEP  (16U)
#endif

/*******************************************************************************
 * File Scope Variables
 ******************************************************************************/


/*******************************************************************************
 * File Scope Macros
 ******************************************************************************/


/*******************************************************************************
 * File Scope Inline Functions
 ******************************************************************************/


/*******************************************************************************
 * File Scope Function Prototypes
 ******************************************************************************/


/*******************************************************************************
 * Exported Function Definitions
 ******************************************************************************/
/**
 * @brief   Delays program for desired amount.
 * @note    Due to loop overhead, the delay time may be slightly longer.
 * @note    Verified on the STM32F072CB, a 100us request results in ~107us sleep in reality. Currently not verified on the STM32F070CB.
 * @param[in]   delay   Time in micro-seconds the program should wait for.
 */
void delay_1us(uint32_t delay)
{
    // Divide by 8 as we do 8 nops in a loop (reduces head-room from servicing the for loop)
    for (uint32_t wait = 0; wait < (delay * (NOPS_FOR_1US_SLEEP / 8U)); wait++)
    {
        __ASM volatile ("nop");
        __ASM volatile ("nop");
        __ASM volatile ("nop");
        __ASM volatile ("nop");
        __ASM volatile ("nop");
        __ASM volatile ("nop");
        __ASM volatile ("nop");
        __ASM volatile ("nop");
    }
}


/*******************************************************************************
 * File Scope Function Definitions
 ******************************************************************************/
