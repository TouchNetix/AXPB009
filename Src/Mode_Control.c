/*******************************************************************************
* @file           : Mode_Control.c
* @author         : James Cameron (TouchNetix)
* @date           : 29 Oct 2020
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
#include "Init.h"
#include "Comms.h"
#include "Mode_Control.h"
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_mouse.h"
#include "usbd_mouse_if.h"
#include "Usage_Builder.h"

/*============ Defines ============*/
#define COMMSSWITCH_NUM_PULSES_FOR_I2C              (4U)
#define COMMSSWITCH_NUM_PULSES_FOR_USB              (2U)

/*============ Local Variables ============*/
uint8_t     g_NresetCount = 0U;
uint32_t    g_WindowMonitorStartCount = 0U;
uint8_t     g_ModeState = USBMODE_IDLE;

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

/*-----------------------------------------------------------*/

// sends a reset signal to the connected device (aXiom)
void Reset_aXiom(void)
{
    // Configure the nRESET as an output, allowing the bridge to reset aXiom.
    Configure_nRESET(NRESET_OUTPUT);

    HAL_GPIO_WritePin(nRESET_GPIO_Port, NRESET_PIN, RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(nRESET_GPIO_Port, NRESET_PIN, SET);
    HAL_Delay(500); // Gives aXiom time to boot up again.

    // Configure the nRESET as an input, so the bridge can monitor for host activity.
    Configure_nRESET(NRESET_INPUT);
}

/*-----------------------------------------------------------*/

void Configure_nRESET(uint8_t mode)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    HAL_GPIO_DeInit(nRESET_GPIO_Port, NRESET_PIN);

    if (mode == NRESET_OUTPUT)
    {
        HAL_NVIC_DisableIRQ(TIM3_IRQn);

        HAL_GPIO_WritePin(nRESET_GPIO_Port, NRESET_PIN, SET);
        GPIO_InitStruct.Pin  = NRESET_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(nRESET_GPIO_Port, &GPIO_InitStruct);
    }
    else if (mode == NRESET_INPUT)
    {
        GPIO_InitStruct.Pin = NRESET_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM3;
        HAL_GPIO_Init(nRESET_GPIO_Port, &GPIO_InitStruct);

        /* TIM3 interrupt Init */
        HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM3_IRQn);
    }
    else
    {
        // Invalid mode.
    }
}

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

uint8_t Get_Bridge_Comms_State(void)
{
    return g_ModeState;
}

void Latch_Monitor_Window_Count(uint32_t count)
{
    g_WindowMonitorStartCount = count;
}

uint32_t Get_Monitor_Window_Count(void)
{
    return g_WindowMonitorStartCount;
}

void Bridge_Comms_Switch_State_Machine(uint8_t Action)
{
    switch(g_ModeState)
    {
        default:
        case USBMODE_IDLE:
        {
            if (Action == ACTIVITY_DETECTED)
            {
                // Activity detected on nRESET pin.
                g_ModeState = NRESETACTIVITYDETECTED_USB;
            }

            // The host has sent a 5-50ms reset pulse to aXiom, start the timer and look
            // for more pulses of the same length.

            break;
        }

        case NRESETACTIVITYDETECTED_USB:
        {
            if (Action == RESET_PULSE_DETECTED)
            {
                g_NresetCount++;
            }
            else if (RESET_WINDOW_ELAPSED)
            {

            }
            else
            {

            }

            if (g_NresetCount >= COMMSSWITCH_NUM_PULSES_FOR_I2C)
            {
                g_ModeState = I2CMODE_IDLE;
            }

            break;
        }

        case I2CMODE_IDLE:
        {
            // The host has requested to talk to aXiom directly over i2c. The bridge has disconnected from
            // the USB and aXiom is in I2C mode.
            break;
        }

        case NRESETACTIVITYDETECTED_I2C:
        {
            // The host has sent a 5-50ms reset pulse to aXiom, start monitoring for more pulses of
            // the same length.
            if (g_NresetCount == COMMSSWITCH_NUM_PULSES_FOR_USB)
            {
                // Reset the bridge. The host may have upgraded aXiom FW or loaded a new CFG.
            }
            else
            {
                // Not enough pulses sent within the window, reset the count.
                g_NresetCount = 0U;
                g_ModeState = I2CMODE_IDLE;
            }
            break;
        }
    }
}

/*============ Local Functions ============*/
