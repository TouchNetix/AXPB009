/*******************************************************************************
* @file           : Mode_Control.c
* @author         : James Cameron (TouchNetix)
* @date           : 29 Oct 2020
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
#include "stm32f0xx.h"
#include <stdbool.h>
#include "Init.h"
#include "Comms.h"
#include "Mode_Control.h"
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_mouse.h"
#include "usbd_mouse_if.h"
#include "Usage_Builder.h"

/*============ Defines ============*/


/*============ Local Variables ============*/


/*============ Exported Variables ============*/
uint8_t WakeupMode      = 0;
bool    boBlockReports  = 0;    // when the host sends a command that needs a response it's likely the bridge will send back a few proxy reports before replying (not good!) so can use this
                                // to block any sending until the command has been processed

/*============ Local Functions ============*/


/*============ Exported Functions ============*/
// returns true if digitizer/mouse not enabled
bool InMouseOrDigitizerMode(void)
{
    bool status = 0;

    if(BridgeMode >= ABSOLUTE_MOUSE)
        status = true;
    else
        status = false;

    return status;
}

/*-----------------------------------------------------------*/

void RestartBridge(void)
{
    Device_DeInit();
    HAL_NVIC_SystemReset();
}

/*-----------------------------------------------------------*/

bool WakeupHost(uint8_t NumTouches, uint8_t byReportZ)
{
    bool status = false;

    switch(WakeupMode)
    {
        case NO_WAKE:
        {
            status = false;
            break;
        }
        case WAKE_ON_TOUCH:
        {
            if(NumTouches > 0)
            {
                status = true;
            }
            break;
        }
        case WAKE_ON_TOUCH_HOVER:
        {
            // Z value must be greater than 0x80 to be classed as hover --> 0x80 indicates there is a prox detected
            if((NumTouches > 0) || (byReportZ >= 0x81))
            {
                    status = true;
            }
            break;
        }
        case WAKE_ON_TOUCH_HOVER_PROX:
        {
            // z value can be greater than (hover) or equal to (prox) 0x80
            if((NumTouches > 0) | (byReportZ >= 0x80))
            {
                status = true;
            }
            break;
        }
        default:
        {
            // should never be in here!
            status = false;
            break;
        }
    }

    return status;
}


/*============ Local Functions ============*/
// groups all de-initialisations
void Device_DeInit(void)
{
    // Disconnect from USB bus and wait - Windows doesn't seem to like a device disconnect and re-connecting
    // in rapid succession
    USBD_Stop(&hUsbDeviceFS);
    USBD_DeInit(&hUsbDeviceFS);
    HAL_Delay(3000);

    HAL_TIM_Base_Stop_IT(&htim16);

    // Only deinit the comms module in use as the
    // pointer to other will be NULL and will result in a hardfault
    if(comms_mode == SPI)
    {
        HAL_SPI_DeInit(&hspi_module);
        HAL_DMA_DeInit(&hdma_spi_tx);
        HAL_DMA_DeInit(&hdma_spi_rx);
    }
    else if(comms_mode == I2C)
    {
        HAL_I2C_DeInit(&hi2c_module);
    }
    else
    {
        // should never be here BUT we're resetting so shouldn't really make a difference
    }

    HAL_TIM_Base_MspDeInit(&htim16);
    HAL_GPIO_DeInit(LED_USB_GPIO_Port, LED_USB_Pin);
    HAL_GPIO_DeInit(LED_AXIOM_GPIO_Port, LED_AXIOM_Pin);
    HAL_DeInit();
}
