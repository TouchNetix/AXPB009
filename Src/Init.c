/*******************************************************************************
* @file           : Init.c
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

/*============ Includes ============*/
#include "stm32f0xx.h"
#include "Init.h"
#include "Usage_Builder.h"
#include "Comms.h"
#include "Command_Processor.h"
#include "SPI_comms.h"
#include "I2C_Comms.h"
#include "usbd_generic_if.h"
#include "usbd_generic.h"
#include "usbd_mouse_if.h"
#include "usbd_mouse.h"
#include "usbd_composite.h"
#include "usb_device.h"
#include "Proxy_driver.h"
#include "Flash_Control.h"
#include "Digitizer.h"
#include "Mode_Control.h"

/*============ Defines ============*/
#define USAGETABLE_MAX_RETRY_NUM    (250U)
#define MAX_WAIT                    (20000U)

/*============ Local Variables ============*/

/*============ Exported Variables ============*/
SPI_HandleTypeDef hspi_module;
I2C_HandleTypeDef hi2c_module;
DMA_HandleTypeDef hdma_spi_tx;
DMA_HandleTypeDef hdma_spi_rx;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim16;

/*============ Local Function Declarations ============*/
static  uint8_t check_comms_mode(void);
static  void    GPIO_Init(void);
static  void    DMA_Init(void);
static  void    TIM3_Init(void);
static  void    TIM16_Init(void);
static  void    LEDs_Init(void);
static  bool    detect_host_presence(void);
static  void    Change_Axiom_Mode(uint8_t comms_sel);

/*============ Functions ============*/

void Device_Init(void)
{
    uint8_t retry = 0;

    /* Reset of all peripherals, Initializes the Flash interface and the Systick
     * Enable the HAL library so we can use those functions */
    HAL_Init();
    SystemClock_Config();

    /* Make sure the USB core is reset properly if leaving bootloader mode */
    HAL_NVIC_DisableIRQ(USB_IRQn);
    __HAL_RCC_USB_FORCE_RESET();
    __HAL_RCC_USB_RELEASE_RESET();

    /* Sets the boot configuration bits so we operate from the correct memory area next reset */
    check_boot_config();

    /* read BridgeMode from flash (located in the option byte) */
    BridgeMode = GetDeviceModeFromFlash();

/*------------ Check for USB host ------------*/

    // First thing to do is put aXiom is I2C mode --> we always assume that someone else (over i2c) is talking to the device (think control board)
    Change_Axiom_Mode(I2C);

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // disable the USB module - ensures the GPIO pins used to detect the host aren't blocked by the hardware module
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
    USB->CNTR |= USB_CNTR_PDWN;

    GPIO_InitStruct.Pin  = GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* We can now check if a host is connected to the USB and decide what to do:
     * Host connected - Put axiom back in SPI mode and run our normal routine
     * Host absent - Release nRESET line and do nothing --> someone else is wanting to control axiom */
    bool host_detected = detect_host_presence();
    if(host_detected)
    {
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11);
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12);
    }
    else
    {
        // no host connection detected - release nRESET in case the i2c host needs control over it
        HAL_GPIO_DeInit(nRESET_GPIO_Port, NRESET_PIN);
        while(host_detected == false)
        {
            // Keep trying to connect to a USB host.
            // If the USB cable is being connected slowly, the power pins are connected before the data pins,
            // so the bridge might assume no host is present!
            HAL_Delay(500);
            host_detected = detect_host_presence();
        }

        // Host was detected! Deinit the pins as they're about to be used for USB
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11);
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_12);
    }

/*--------------------------------------------*/

    /* Initialize the rest of the peripherals */
    TIM16_Init();
    GPIO_Init();
    DMA_Init();
    LEDs_Init();

    HAL_GPIO_TogglePin(LED_AXIOM_GPIO_Port, LED_AXIOM_Pin);

    /* Comms selection */
    if(check_comms_mode() == I2C)
    {
        // aXiom already in I2C mode so no need to change mode
        MX_I2C_Init();
        device_address = get_i2c_address();
    }
    else
    {
        Change_Axiom_Mode(SPI);
        MX_SPI_Init();
    }

    // If we're in I2C mode and aXiom hasn't been found, don't bother trying to read the usage table.
    // We've waited ~12s already, so just come up in Bridge Only mode (won't be able to read usage table anyway).
    if (((check_comms_mode() == I2C) && (device_address != 0)) || (check_comms_mode() == SPI))
    {
        /* Parse TCP device */
        do
        {
            uint8_t status = build_usage_table();    // parse usages BEFORE initialising USB --> won't work correctly otherwise as host won't get any reponses whilst table is being generated
            static uint8_t led_count = 0;

            if(status == HAL_OK)
            {
                // usages parsed successfully
                break;
            }
            else
            {
                // failed to parse usages, wait for a bit then try again
                HAL_Delay(50U); //ms

                if(led_count > 1U)
                {
                    HAL_GPIO_TogglePin(LED_AXIOM_GPIO_Port, LED_AXIOM_Pin);
                    HAL_GPIO_TogglePin(LED_USB_GPIO_Port, LED_USB_Pin);
                    led_count = 0;
                }
                else
                {
                    led_count++;
                }
            }

            retry++;
        } while(retry < USAGETABLE_MAX_RETRY_NUM);   // holds device here for ~12 seconds, enough time for aXiom to exit bootloader if in there for some reason
    }

    HAL_GPIO_WritePin(LED_AXIOM_GPIO_Port, LED_AXIOM_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_USB_GPIO_Port, LED_USB_Pin, GPIO_PIN_RESET);

    // Setup the nRESET monitoring state machine.
//    Configure_nRESET(NRESET_INPUT);
    HAL_GPIO_DeInit(nRESET_GPIO_Port, NRESET_PIN);
    TIM3_Init();
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_4);

    if(BridgeMode == PARALLEL_DIGITIZER) // set comms parameters for device to work in digitizer mode (noone else to set these!)
    {
        HAL_TIM_Base_Start_IT(&htim16); // starts the timer used for digitizer timestamps
    }

    InitProxyInterruptMode();   // sets pin PA0 as input --> the bridge will enter proxy mode at startup

    /* USB Initialisation */
    MX_USB_DEVICE_Init();   // USB is final thing to setup --> means nothing else will hold up the device, allowing it to respond correctly (otherwise it would be unresponsive/dead in the hosts' eyes)
}

//--------------------------

/**
  * @brief Sets up the selected pin as an EXTI interrupt
  * @param GPIO_Pin pin desired to be used as interrupt trigger
  * @param GPIOx bank where the desired pin is situated --> 0x0 for GPIOA, 0x1 for GPIOB etc.
  * @param mode user can decide if pin triggers interrupt on falling or rising edge, or both
  */
void Custom_EXTI_Setup(uint8_t GPIO_Pin, uint8_t GPIOx, uint8_t trigger_mode)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // set SYSCFG to connect pin EXTI line to GPIOx
    SYSCFG->EXTICR[(GPIO_Pin/4)] &=   ~(0xF << ((GPIO_Pin % 4) * 4));
    SYSCFG->EXTICR[(GPIO_Pin/4)] |=  (GPIOx << ((GPIO_Pin % 4) * 4));

    // setup the pin EXTI line as an interrupt
    EXTI->IMR  |=  (1 << GPIO_Pin);

    // disable edge triggers
    EXTI->RTSR &= ~(1 << GPIO_Pin);
    EXTI->FTSR &= ~(1 << GPIO_Pin);

    if(trigger_mode == RISING) // enable falling edge trigger
        EXTI->RTSR  |= (1 << GPIO_Pin);

    if(trigger_mode == FALLING) // enable rising edge trigger
        EXTI->FTSR  |= (1 << GPIO_Pin);

    if(trigger_mode == BOTH)    // enable rising and falling edge trigger
    {
        EXTI->FTSR  |= (1 << GPIO_Pin);
        EXTI->RTSR  |= (1 << GPIO_Pin);
    }

}

/*============ Initialisation Functions ============*/
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks
  */
#if defined(STM32F070xB)
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
#else
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
#endif

  /** Initializes the CPU, AHB and APB busses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
#if defined(STM32F070xB)
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
#else
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
#endif

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

//--------------------------

/**
  * @brief DMA Initialization Function
  * @param None
  * @retval None
  */
static void DMA_Init(void)
{
    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA interrupt init */

    /* DMA1_Channel2_3_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
}

//--------------------------

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* slave select for SPI */
    HAL_GPIO_WritePin(nSS_SPI_GPIO_Port, nSS_SPI, SET);
    GPIO_InitStruct.Pin  = nSS_SPI;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(nSS_SPI_GPIO_Port, &GPIO_InitStruct);

    /* Used as reset for aXiom */
    Configure_nRESET(NRESET_OUTPUT);
}

//--------------------------

static bool detect_host_presence(void)
{
    uint32_t    host_detection_time_limit = 0;
    uint16_t    host_detected_count = 0;
    bool        host_detected = false;

    do
    {
        /* Physical connections of the USB data line (before USB module is enabled)
         * Host side:
         *      ~20k pull downs on D+ and D-
         * Bridge side:
         *      220k pull ups on D+ and D-
         *
         * If host is present:
         *      D+ and D- will read 0
         *
         * Host not present
         *      D+ and D- will read 1
         *
         * In the event of a transient (such as emc testing) we need to be certain the host is actually there,
         * so we check for multiple loops and reset our counter if we 'lose' the host
         */
        if((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11) == 0) && (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12) == 0))
        {
            if(host_detected_count < 2000)
            {
                host_detected_count++;
            }
        }
        else
        {
            host_detected_count = 0;
        }

        if(host_detected_count >= 2000)
        {
            host_detected = true;
        }
        else
        {
            host_detected = false;
        }

        host_detection_time_limit++;
    }
    while(host_detection_time_limit <= MAX_WAIT);

    return host_detected;
}

//--------------------------

/**
  * @brief Reads the comms mode select pin, either SPI or I2C
  * @param None
  * @retval Comms mode
  */
uint8_t check_comms_mode(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // initialise the pin first so we can read it
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin  = COMMS_SELECT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(COMMS_SELECT_GPIO_Port, &GPIO_InitStruct);

    comms_mode = HAL_GPIO_ReadPin(COMMS_SELECT_GPIO_Port, COMMS_SELECT_Pin);

    return comms_mode;
}

//--------------------------

/**
  * @brief LED Initialization Function
  * @param None
  * @retval None
  */
static void LEDs_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    HAL_GPIO_WritePin(LED_USB_GPIO_Port, LED_USB_Pin, RESET);
    HAL_GPIO_WritePin(LED_AXIOM_GPIO_Port, LED_AXIOM_Pin, RESET);

    /* aXiom activity LED */
    GPIO_InitStruct.Pin = LED_USB_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LED_USB_GPIO_Port, &GPIO_InitStruct);

    /* USB activity LED */
    GPIO_InitStruct.Pin = LED_AXIOM_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LED_AXIOM_GPIO_Port, &GPIO_InitStruct);
}

//--------------------------

static void TIM3_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_IC_InitTypeDef sConfigIC = {0};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = (SYSTEMCLOCK_IN_MHZ * 3200U) - 1U; // Increment every 100us.
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 10U;//65535U; // Max period.
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_IC_Init(&htim3) != HAL_OK)
    {
      Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
    {
      Error_Handler();
    }
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 15;
    if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
    {
      Error_Handler();
    }
}

//--------------------------

static void TIM16_Init(void)
{
    htim16.Instance = TIM16;
    htim16.Init.Prescaler = SYSTEMCLOCK_IN_MHZ - 1; // Increment every 1us.
    htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim16.Init.Period = 100 - 1; // want a period of 100us
    htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim16.Init.RepetitionCounter = 0;
    htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
    {
      Error_Handler();
    }
}

//--------------------------

// enables the necessary GPIOs to put axiom into chosen mode and sends a reset signal
static void Change_Axiom_Mode(uint8_t comms_sel)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    static bool gpios_initialised = 0;

    // this saves us setting up the pins again if coming here for a 2nd time (e.g. if putting axiom back in SPI mode)
    if(gpios_initialised == 0)
    {
        // enable GPIO clocks
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();

        // initialise MISO pin as an output
        GPIO_InitStruct.Pin  = SPI_MISO_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(SPI_MISO_GPIO_Port, &GPIO_InitStruct);

        // initialise nRESET
        Configure_nRESET(NRESET_OUTPUT);

        gpios_initialised = 1;
    }

    switch(comms_sel)
    {
        case I2C:
        {
            // Hold MISO low and reset axiom
            HAL_GPIO_WritePin(SPI_MISO_GPIO_Port, SPI_MISO_Pin, RESET);
            break;
        }

        case SPI:
        {
            // Release (de-init) MISO --> tells axiom to boot in SPI mode next reset
            HAL_GPIO_DeInit(SPI_MISO_GPIO_Port, SPI_MISO_Pin);
            break;
        }
    }

    // put axiom in I2C mode
    Reset_aXiom();
}
