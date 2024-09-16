/*******************************************************************************
* @file           : _AXPB-009_Main.h
* @author         : James Cameron (TouchNetix)
* @date           : 22 Sep 2020
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define nSS_SPI             GPIO_PIN_4
#define nSS_SPI_GPIO_Port   GPIOA

#define NRESET_PIN          GPIO_PIN_1
#define nRESET_GPIO_Port    GPIOB
#define nIRQ_Pin            GPIO_PIN_0
#define nIRQ_GPIO_Port      GPIOA

#define SWDIO_Pin           GPIO_PIN_13
#define SWDIO_GPIO_Port     GPIOA
#define SWCLK_Pin           GPIO_PIN_14
#define SWCLK_GPIO_Port     GPIOA

#define USBF4_DM_Pin        GPIO_PIN_11
#define USBF4_DM_GPIO_Port  GPIOA
#define USBF4_DP_Pin        GPIO_PIN_12
#define USBF4_DP_GPIO_Port  GPIOA

#if defined(STM32F042x6)
    #define COMMS_SELECT_Pin        GPIO_PIN_2
    #define COMMS_SELECT_GPIO_Port  GPIOA

    #define I2C_CLK_Pin             GPIO_PIN_1
    #define I2C_CLK_GPIO_Port       GPIOF
    #define I2C_SDA_Pin             GPIO_PIN_0
    #define I2C_SDA_GPIO_Port       GPIOF

    #define SPI_SCK_Pin             GPIO_PIN_5
    #define SPI_SCK_GPIO_Port       GPIOA
    #define SPI_MISO_Pin            GPIO_PIN_6
    #define SPI_MISO_GPIO_Port      GPIOA
    #define SPI_MOSI_Pin            GPIO_PIN_7
    #define SPI_MOSI_GPIO_Port      GPIOA

    #define LED_USB_Pin             GPIO_PIN_8
    #define LED_USB_GPIO_Port       GPIOB
    #define LED_AXIOM_Pin           GPIO_PIN_1
    #define LED_AXIOM_GPIO_Port     GPIOA
#endif


#if (defined(STM32F070xB)) || ((defined(STM32F072xB)) && !defined(STM32F072RB_DISCOVERY))
    #define COMMS_SELECT_Pin        GPIO_PIN_2
    #define COMMS_SELECT_GPIO_Port  GPIOB

    #define I2C_CLK_Pin             GPIO_PIN_6
    #define I2C_CLK_GPIO_Port       GPIOB
    #define I2C_SDA_Pin             GPIO_PIN_7
    #define I2C_SDA_GPIO_Port       GPIOB

    #define SPI_SCK_Pin             GPIO_PIN_5
    #define SPI_SCK_GPIO_Port       GPIOA
    #define SPI_MISO_Pin            GPIO_PIN_6
    #define SPI_MISO_GPIO_Port      GPIOA
    #define SPI_MOSI_Pin            GPIO_PIN_7
    #define SPI_MOSI_GPIO_Port      GPIOA

    #define LED_USB_Pin             GPIO_PIN_8
    #define LED_USB_GPIO_Port       GPIOB
    #define LED_AXIOM_Pin           GPIO_PIN_1
    #define LED_AXIOM_GPIO_Port     GPIOA
#endif


#if defined(STM32F072xB) && defined(STM32F072RB_DISCOVERY)
    #define COMMS_SELECT_Pin        GPIO_PIN_2
    #define COMMS_SELECT_GPIO_Port  GPIOB

    #define I2C_CLK_Pin             GPIO_PIN_6
    #define I2C_CLK_GPIO_Port       GPIOB
    #define I2C_SDA_Pin             GPIO_PIN_7
    #define I2C_SDA_GPIO_Port       GPIOB

    #define SPI_SCK_Pin             GPIO_PIN_5
    #define SPI_SCK_GPIO_Port       GPIOA
    #define SPI_MISO_Pin            GPIO_PIN_4
    #define SPI_MISO_GPIO_Port      GPIOB
    #define SPI_MOSI_Pin            GPIO_PIN_5
    #define SPI_MOSI_GPIO_Port      GPIOB

    #define LED_USB_Pin             GPIO_PIN_7
    #define LED_USB_GPIO_Port       GPIOC
    #define LED_AXIOM_Pin           GPIO_PIN_8
    #define LED_AXIOM_GPIO_Port     GPIOC
#endif


/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
