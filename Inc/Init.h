/*******************************************************************************
* @file           : Init.h
* @author         : James Cameron (TouchNetix)
* @date           : 24 Sep 2020
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

#ifndef INIT_H_
#define INIT_H_

/*============ Includes ============*/
#include "stm32f0xx.h"

/*============ Exported Defines ============*/
// gpio banks used for exti setup function
#define GPIO_A  (0)
#define GPIO_B  (1)
#define GPIO_C  (2)

#define RISING  (0)
#define FALLING (1)
#define BOTH    (2)

#if defined(STM32F070xB)
#define SYSTEMCLOCK_IN_MHZ  (48U)
#else
#define SYSTEMCLOCK_IN_MHZ  (32U)
#endif

/*============ Exported Variables ============*/
extern SPI_HandleTypeDef hspi_module;
extern I2C_HandleTypeDef hi2c_module;
extern DMA_HandleTypeDef hdma_spi_tx;
extern DMA_HandleTypeDef hdma_spi_rx;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim16;
extern TIM_HandleTypeDef htim17;

/*============ Exported Functions ============*/
void Device_Init(void);
void SystemClock_Config(void);
void Custom_EXTI_Setup(uint8_t GPIO_Pin, uint8_t GPIOx, uint8_t trigger_mode);

#endif /* INIT_H_ */
