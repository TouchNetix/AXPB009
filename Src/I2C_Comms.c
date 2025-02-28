/*******************************************************************************
* @file           : I2C_comms.c
* @author         : James Cameron (TouchNetix)
* @date           : 5 Oct 2021
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
#include "I2C_Comms.h"
#include "Init.h"
#include "Timers_and_LEDs.h"
#include "Command_Processor.h"

/*============ Defines ============*/
#define I2C_SPEED_FAST  (0x0000020Bu)
#define I2C_MAX_TIMEOUT      (65000u)

#define MAX_ADDR_SEARCH_ATTEMPTS	(250U)

/*============ Local Variables ============*/
volatile uint8_t i2c_tx_flag = 0;
volatile uint8_t i2c_rx_flag = 0;

/*============ Exported Variables ============*/
uint8_t device_address = 0;

/*============ Local Function Declarations ============*/
HAL_StatusTypeDef I2CSendData(uint8_t *data, uint16_t len);
HAL_StatusTypeDef I2CReceiveData(uint8_t *data, uint16_t len);

/*============ Interrupt Handlers ============*/
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    // raise flag - indicates successful transfer
    i2c_tx_flag = 1;

    // toggle activity LED and tell the main loop that there is comms happening
    boFlashAxiomLED = 1;
    boAxiomActivity = 1;
}

//--------------------------

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    // raise flag - indicates successful transfer
    i2c_rx_flag = 1;

    // toggle activity LED and tell the main loop that there is comms happening
    boFlashAxiomLED = 1;
    boAxiomActivity = 1;
}

/*============ Local Functions ============*/
HAL_StatusTypeDef I2CSendData(uint8_t *data, uint16_t len)
{
    uint8_t  status = HAL_ERROR;
    uint16_t timeout_count = 0;

    // if doing a read from aXiom we want to use a repeated start
    if(aXiom_NumBytesRx)
    {
        status = HAL_I2C_Master_Sequential_Transmit_IT(&hi2c_module, device_address, data, aXiom_NumBytesTx, I2C_FIRST_FRAME);

        // wait for interrupt flag to set, or timeout
        do
        {
            if(timeout_count >= I2C_MAX_TIMEOUT)
            {
                break;
            }

            // start the timeout counter
            timeout_count++;
        }
        while (i2c_tx_flag == 0);

        // clear the flag
        i2c_tx_flag = 0;
    }
    // just doing a write, no need for repeated start
    else
    {
        status = HAL_I2C_Master_Transmit_IT(&hi2c_module, device_address, data, aXiom_NumBytesTx);

        // wait for interrupt flag to set, or timeout
        do
        {
            if(timeout_count >= I2C_MAX_TIMEOUT)
            {
                break;
            }

            // start the timeout counter
            timeout_count++;
        }
        while (i2c_tx_flag == 0);

        aXiom_Rx_Buffer[CircularBufferHead][0] = COMMS_OK_NO_READ;
        aXiom_Rx_Buffer[CircularBufferHead][1] = aXiom_NumBytesRx;

        // clear flags
        boCommsInProcess = 0;
        i2c_tx_flag = 0;
    }

    // if this has reached the max value it means axiom hasn't responded
    if(timeout_count >= I2C_MAX_TIMEOUT)
    {
        status = HAL_TIMEOUT;
    }

    return status;
}

HAL_StatusTypeDef I2CReceiveData(uint8_t *data, uint16_t len)
{
    uint8_t status = HAL_ERROR;
    uint16_t timeout_count = 0;

    // offset by 2 bytes to allow for status bytes
    status = HAL_I2C_Master_Sequential_Receive_IT(&hi2c_module, device_address, &data[2], aXiom_NumBytesRx, I2C_LAST_FRAME);

    // wait for interrupt flag to set, or timeout
    do
    {
        if(timeout_count >= I2C_MAX_TIMEOUT)
        {
            break;
        }

        // start the timeout counter
        timeout_count++;
    }
    while (i2c_rx_flag == 0);

    aXiom_Rx_Buffer[CircularBufferHead][0] = COMMS_OK;
    aXiom_Rx_Buffer[CircularBufferHead][1] = aXiom_NumBytesRx;

    // clear flags
    boCommsInProcess = 0;
    i2c_rx_flag = 0;

    // if this has reached the max value it means axiom hasn't responded
    if(timeout_count >= I2C_MAX_TIMEOUT)
    {
        status = HAL_TIMEOUT;
    }

    return status;
}

/*============ Exported Functions ============*/
/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C_Init(void)
{
    hi2c_module.Instance = I2C1;
    hi2c_module.Init.Timing = I2C_SPEED_FAST;
    hi2c_module.Init.OwnAddress1 = 0;
    hi2c_module.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c_module.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c_module.Init.OwnAddress2 = 0;
    hi2c_module.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c_module.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c_module.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c_module) != HAL_OK)
    {
      Error_Handler();
    }

    // Configure Analogue filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c_module, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
        Error_Handler();
    }

    // Configure Digital filter
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c_module, 0) != HAL_OK)
    {
        Error_Handler();
    }
}

//--------------------------

/**
  * @brief top level comms function
  * @param None
  * @retval Status
  */
HAL_StatusTypeDef do_i2c_comms(void)
{
    HAL_StatusTypeDef status = HAL_ERROR;

    // always do a transmit regardless of read or write - never writing less than 4, if we are it's an error
    if(aXiom_NumBytesTx >= 4)
    {
        status = I2CSendData(aXiom_Tx_Buffer, aXiom_NumBytesTx);
    }
    else
    {
        status = HAL_ERROR;
    }

    // receive data - only process if the write didn't fail/we are expecting to read
    if(aXiom_NumBytesRx && (status == HAL_OK))
    {
        status = I2CReceiveData(aXiom_Rx_Buffer[CircularBufferHead], aXiom_NumBytesRx);
    }

    // Make sure I2C recovers to a known state if comms has failed
    if(status != HAL_OK)
    {
    	HAL_I2C_Master_Abort_IT(&hi2c_module, device_address);
    }

    return status;
}

//--------------------------

/**
 * @brief       Scans the 7-bit i2c address range looking for a device to connect to.
 * @details     aXiom typically has address 0x66 or 0x67.
 * @return      Address of connected device in 7-bit format, left shifted by 1.
  */
uint8_t get_i2c_address(void)
{
    uint8_t temp_address;
    uint8_t retry_count = 0;
    bool    address_found = false;

    // Waits for a maximum of ~12.5 seconds for aXiom to ACK an address request
    do
    {
        uint8_t buf[1U];
        buf[0] = 0x00U;
        static uint8_t led_count = 0;

        for (temp_address = 0x66U; temp_address < 0x68U; temp_address++)
        {
            // Sends 0 bytes, we're only looking for the device to return an ACK
            // This is a blocking function meaning device will lock up if no device connected and Timeout duration is set to HAL_MAX_DELAY, probably want to choose a smaller value eventually
            if (HAL_I2C_Master_Transmit(&hi2c_module, (temp_address << 1U), buf, 0U, HAL_MAX_DELAY) == HAL_OK)
            {
                // Device found
                address_found = true;
                break;
            }
        }
        // Flash some LEDs
        if(led_count > 1)
        {
            HAL_GPIO_TogglePin(LED_AXIOM_GPIO_Port, LED_AXIOM_Pin);
            HAL_GPIO_TogglePin(LED_USB_GPIO_Port, LED_USB_Pin);
            led_count = 0;
        }
        else
        {
            led_count++;
        }

        // Sleep for a bit before trying again
        HAL_Delay(50U);
        retry_count++;
    } while ((address_found == false) && (retry_count < MAX_ADDR_SEARCH_ATTEMPTS));

    if(address_found == false)
    {
        // address not found
        temp_address = 0U;
    }

    return (temp_address << 1U);
}
