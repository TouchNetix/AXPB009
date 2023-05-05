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

/*============ Defines ============*/
#define LED_FREQ_DIV  (0x5)

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

// callback function when TIM16 has reset --> used to increment the digitizer timestamp
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim == &htim16)
    {
        wd100usTick++; // increment the digitizer timestamp --> this callback is entered every 100us, which is what windows is expecting
    }
}
