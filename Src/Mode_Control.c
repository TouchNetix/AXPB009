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
#include "Proxy_driver.h"

/*============ Defines ============*/
#define COMMSSWITCH_NUM_PULSES_FOR_I2C              (4U)
#define COMMSSWITCH_NUM_PULSES_FOR_USB              (3U)

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
    Device_DeInit(true);
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
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM3;
        HAL_GPIO_Init(nRESET_GPIO_Port, &GPIO_InitStruct);
    }
    else
    {
        // Invalid mode.
    }
}

void Device_DeInit(bool DoDelay)
{
    // Disconnect from USB bus and wait - Windows doesn't seem to like a device disconnect and re-connecting
    // in rapid succession
    USBD_Stop(&hUsbDeviceFS);
    USBD_DeInit(&hUsbDeviceFS);
    if (DoDelay == true)
    {
        HAL_Delay(3000);
    }

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

void Bridge_Comms_Switch_State_Machine(uint8_t Action)
{
    switch(g_ModeState)
    {
        default:
        case USBMODE_IDLE:
        {
            // Monitor nRESET.

            if (Action == ACTIVITY_DETECTED)
            {
                // Activity detected on nRESET pin.
                g_ModeState = NRESETACTIVITYDETECTED_USB;
            }
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
                if (g_NresetCount == COMMSSWITCH_NUM_PULSES_FOR_I2C)
                {
                    // Correct number of resets seen within the window.

                    g_NresetCount = 0;

                    // Disconnect from USB, put aXiom into i2c mode and send a 100ms pulse to signify to host
                    // that the mode is switching.
                    USBD_Stop(&hUsbDeviceFS);
                    USBD_DeInit(&hUsbDeviceFS);

                    // De-initialise GPIOs.
                    HAL_GPIO_DeInit(GPIOA, SPI_SCK_Pin|SPI_MISO_Pin|SPI_MOSI_Pin|nSS_SPI);
                    HAL_GPIO_DeInit(GPIOB, I2C_CLK_Pin|I2C_SDA_Pin);
                    HAL_GPIO_DeInit(nIRQ_GPIO_Port, nIRQ_Pin);

                    // Re-initialise nRESET (PB1), nSLVI2C/MISO (PA6) and I2CADDRSEL/MOSI (PA7).
                    HAL_GPIO_WritePin(SPI_MISO_GPIO_Port, SPI_MISO_Pin, RESET); // I2C mode
                    HAL_GPIO_WritePin(SPI_MOSI_GPIO_Port, SPI_MOSI_Pin, RESET); // I2C address 0x66
                    GPIO_InitTypeDef GPIO_InitStruct = {0};
                    GPIO_InitStruct.Pin  = SPI_MISO_Pin|SPI_MOSI_Pin;
                    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
                    GPIO_InitStruct.Pull = GPIO_NOPULL;
                    HAL_GPIO_Init(SPI_MISO_GPIO_Port, &GPIO_InitStruct);
                    Configure_nRESET(NRESET_OUTPUT);

                    // Send a 100ms pulse to signify to host that the mode is switching.
                    // Temporarily raise the systick interrupt so we can delay here.
                    // A bit dirty, but we're about to become idle anyway.
                    HAL_GPIO_WritePin(nRESET_GPIO_Port, NRESET_PIN, RESET);
                    HAL_Delay(150);
                    HAL_GPIO_WritePin(nRESET_GPIO_Port, NRESET_PIN, SET);

                    // Re-initialise nRESET for TIM3.
                    Configure_nRESET(NRESET_INPUT);

                    // Make sure TIM3 is configured to look for the correct edge.
                    htim3.Instance->CCER &= ~((1 << TIM_CCER_CC4NP_Pos) & TIM_CCER_CC4NP_Msk);
                    htim3.Instance->CCER |= ((1 << TIM_CCER_CC4P_Pos) & TIM_CCER_CC4P_Msk);

                    boProxyEnabled = false;
                    boInternalProxy = false;

                    g_ModeState = I2CMODE_IDLE;
                }
                else
                {
                    // Not enough resets were seen, reset and wait for another window to be started.
                    g_NresetCount = 0;
                    g_ModeState = USBMODE_IDLE;
                }
            }

            break;
        }

        case I2CMODE_IDLE:
        {
            // Monitor nRESET.

            if (Action == ACTIVITY_DETECTED)
            {
                // Activity detected on nRESET pin.
                g_ModeState = NRESETACTIVITYDETECTED_I2C;
            }

            break;
        }

        case NRESETACTIVITYDETECTED_I2C:
        {
            if (Action == RESET_PULSE_DETECTED)
            {
                g_NresetCount++;
            }
            else if (RESET_WINDOW_ELAPSED)
            {
                if (g_NresetCount == COMMSSWITCH_NUM_PULSES_FOR_USB)
                {
                    // Correct number of resets seen within the window.

                    g_NresetCount = 0;
                    g_ModeState = USBMODE_IDLE;

                    // Send a 100ms pulse on nRESET to confirm the mode switch.
                    Configure_nRESET(NRESET_OUTPUT);

                    // Temporarily raise the systick interrupt so we can delay here.
                    // A bit dirty, but we're about to become idle anyway.
                    HAL_GPIO_WritePin(nRESET_GPIO_Port, NRESET_PIN, RESET);
                    HAL_Delay(150);
                    HAL_GPIO_WritePin(nRESET_GPIO_Port, NRESET_PIN, SET);

                    // Reset the bridge to restart USB comms.
                    Device_DeInit(false);
                    RestartBridge();
                }
                else
                {
                    // Not enough resets were seen, reset and wait for another window to be started.
                    g_NresetCount = 0;
                    g_ModeState = I2CMODE_IDLE;
                }
            }
            break;
        }
    }
}

/*============ Local Functions ============*/
