/*******************************************************************************
* @file           : Timers_and_LEDs.c
* @author         : James Cameron (TouchNetix)
* @date           : 15 Jan 2021
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
#include "Mode_Control.h"
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_mouse.h"
#include "usbd_mouse_if.h"
#include "Digitizer.h"
#include "Timers_and_LEDs.h"

/*============ Defines ============*/
#define LED_FREQ_DIV    (0x5)
#define TIM3_5MS        (50U)
#define TIM3_50MS       (500U)

/*============ Local Variables ============*/

/*============ Exported Variables ============*/
uint32_t aXiom_activity_counter = 0;
uint32_t USB_activity_counter = 0;
bool     boAxiomActivity = 0;
bool     boUSBActivity = 0;
bool     boFlashAxiomLED = 0;
bool     boFlashUSBLED = 0;


/*============ Local Functions ============*/

/*============ Exported Functions ============*/

void LED_control(void)
{
    if(boFlashAxiomLED == 1)
    {
        static uint32_t AxiomLEDCounter = 0;

        if(AxiomLEDCounter >= 5)
        {
            HAL_GPIO_WritePin(LED_AXIOM_GPIO_Port, LED_AXIOM_Pin, RESET);
            AxiomLEDCounter = 0;
            boFlashAxiomLED = 0;
        }
        else
        {
            HAL_GPIO_WritePin(LED_AXIOM_GPIO_Port, LED_AXIOM_Pin, GPIO_PIN_SET);
            AxiomLEDCounter++;
        }
    }

    if(boFlashUSBLED == 1)
    {
        static uint32_t AxiomUSBCounter = 0;

        if(AxiomUSBCounter >= 5)
        {
            HAL_GPIO_WritePin(LED_USB_GPIO_Port, LED_USB_Pin, RESET);
            AxiomUSBCounter = 0;
            boFlashUSBLED = 0;
        }
        else
        {
            HAL_GPIO_WritePin(LED_USB_GPIO_Port, LED_USB_Pin, GPIO_PIN_SET);
            AxiomUSBCounter++;
        }
    }
}

//--------------------------

void comms_detect_inactivity(void)
{
    uint32_t static heartbeat_axiom = 0;
    uint32_t static heartbeat_usb = 0;

    // check if there is any SPI activity
    if(boAxiomActivity == 0)
    {
        if(aXiom_activity_counter >= 10000)
        {
            // Once no activity has been detected, start a heartbeat pulse
            if(heartbeat_axiom >= 150000)
            {
                HAL_GPIO_WritePin(LED_AXIOM_GPIO_Port, LED_AXIOM_Pin, RESET);
                HAL_Delay(50);
                heartbeat_axiom = 0;
            }
            else
            {
                HAL_GPIO_WritePin(LED_AXIOM_GPIO_Port, LED_AXIOM_Pin, SET);
                heartbeat_axiom++;
            }
        }
        else
        {
            aXiom_activity_counter++;
        }
    }
    else
    {
        heartbeat_axiom = 0;
        boAxiomActivity = 0;
        aXiom_activity_counter = 0;
    }

    // check if there is any USB activity
    if(boUSBActivity == 0)
    {
        if(USB_activity_counter >= 10000)
        {
            // Once no activity has been detected, start a heartbeat pulse
            if(heartbeat_usb >= 150000)
            {
                HAL_GPIO_WritePin(LED_USB_GPIO_Port, LED_USB_Pin, RESET);
                HAL_Delay(50);
                heartbeat_usb = 0;
            }
            else
            {
                HAL_GPIO_WritePin(LED_USB_GPIO_Port, LED_USB_Pin, SET);
                heartbeat_usb++;
            }
        }
        else
        {
            USB_activity_counter++;
        }
    }
    else
    {
        heartbeat_usb = 0;
        boUSBActivity = 0;
        USB_activity_counter = 0;
    }
}

//--------------------------

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim16)
    {
        wd100usTick++; // increment the digitizer timestamp --> this callback is entered every 100us, which is what windows is expecting
    }
    else if (htim == &htim17)
    {
        Bridge_Comms_Switch_State_Machine(RESET_WINDOW_ELAPSED);
    }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    static uint32_t nResetStartTime = 0;

    if (htim == &htim3)
    {
        uint32_t TimeCount = htim->Instance->CCR4;

        // Get the current edge setting.
        // 0 - Configured for rising edge detection.
        // 1 - Configured for falling edge detection.
        uint32_t edge = (htim->Instance->CCER & TIM_CCER_CC4P_Msk) >> TIM_CCER_CC4P_Pos;

        if (edge == 1)
        {
            // Just had a falling edge, re-configure for a rising edge.
            htim->Instance->CCER &= ~((1 << TIM_CCER_CC4NP_Pos) & TIM_CCER_CC4NP_Msk);
            htim->Instance->CCER &= ~((1 << TIM_CCER_CC4P_Pos) & TIM_CCER_CC4P_Msk);

            uint8_t Mode = Get_Bridge_Comms_State();
            if ((Mode == USBMODE_IDLE) || (Mode == I2CMODE_IDLE))
            {
                // First reset seen this window, start the 500ms timer.
                HAL_TIM_Base_Start_IT(&htim17);

                Bridge_Comms_Switch_State_Machine(ACTIVITY_DETECTED);
            }

            // Save the count (used to determine pulse length).
            nResetStartTime = TimeCount;
        }
        else if (edge == 0)
        {
            // Just had a rising edge, re-configure for a falling edge.
            htim->Instance->CCER &= ~((1 << TIM_CCER_CC4NP_Pos) & TIM_CCER_CC4NP_Msk);
            htim->Instance->CCER |= ((1 << TIM_CCER_CC4P_Pos) & TIM_CCER_CC4P_Msk);

            // Calculate how long the pulse was - 1 tick is 100us.
            uint32_t PulseLengthInTicks;
            if (TimeCount < nResetStartTime)
            {
                // The count register has wrapped around.
                PulseLengthInTicks = (TIM3_PERIOD - nResetStartTime) + TimeCount;
            }
            else
            {
                PulseLengthInTicks = TimeCount - nResetStartTime;
            }

            // Only count pulses that are between 5ms and 50ms long.
            if ((PulseLengthInTicks >= TIM3_5MS) && (PulseLengthInTicks <= TIM3_50MS))
            {
                Bridge_Comms_Switch_State_Machine(RESET_PULSE_DETECTED);
            }
        }
        else
        {
            // Invalid setting - configure to look for a falling edge.
            htim->Instance->CCER |= ((1 << TIM_CCER_CC4P_Pos) & TIM_CCER_CC4P_Msk);
            htim->Instance->CCER &= ~((1 << TIM_CCER_CC4NP_Pos) & TIM_CCER_CC4NP_Msk);
        }
    }
}
