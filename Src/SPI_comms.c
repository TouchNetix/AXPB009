/*******************************************************************************
* @file           : SPI_comms.c
* @author         : James Cameron (TouchNetix)
* @date           : 22 Sep 2020
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
#include "SPI_comms.h"
#include "Comms.h"
#include "Command_Processor.h"
#include "usbd_generic_if.h"
#include "usbd_generic.h"
#include "usbd_mouse_if.h"
#include "usbd_mouse.h"
#include "usbd_press_if.h"
#include "usbd_press.h"
#include "Proxy_driver.h"
#include "Timers_and_LEDs.h"
#include "Init.h"
#include "Delay.h"

/*============ Defines ============*/
#define INVALID_SETUP   (0xFF)
#define LED_FREQ_FACTOR (0x40)

/*============ Local Variables ============*/
uint8_t spi_TxRx_flag = 0;

/*============ Exported Variables =======st=====*/
uint8_t spi_state = TRANSMIT_AND_RECEIVE;

#if defined (STM32F070xB)
uint8_t SPI_Speed_PreScaler = SPI_BAUDRATEPRESCALER_16;
#else
uint8_t SPI_Speed_PreScaler = SPI_BAUDRATEPRESCALER_8;
#endif

/*============ Interrupt Handlers ============*/
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    // raise flag - indicates successful transfer
    spi_TxRx_flag = 1;
    HAL_GPIO_WritePin(nSS_SPI_GPIO_Port, nSS_SPI, GPIO_PIN_SET);

    // toggle activity LED and tell the main loop that there is comms happening
    boFlashAxiomLED = 1;
    boAxiomActivity = 1;
}

//--------------

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    // raise flag - indicates successful transfer
    spi_TxRx_flag = 1;
    HAL_GPIO_WritePin(nSS_SPI_GPIO_Port, nSS_SPI, GPIO_PIN_SET);

    // toggle activity LED and tell the main loop that there is comms happening
    boFlashAxiomLED = 1;
    boAxiomActivity = 1;
}


/*============ Functions ============*/
/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
void MX_SPI_Init(void)
{
    uint8_t temp_send[1] = {0x00};

    /* SPI parameter configuration*/
    hspi_module.Instance = SPI1;
    hspi_module.Init.Mode = SPI_MODE_MASTER;
    hspi_module.Init.Direction = SPI_DIRECTION_2LINES;
    hspi_module.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi_module.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi_module.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi_module.Init.NSS = SPI_NSS_SOFT;
    hspi_module.Init.BaudRatePrescaler = SPI_Speed_PreScaler;
    hspi_module.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi_module.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi_module.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi_module.Init.CRCPolynomial = 7;
    hspi_module.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi_module.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;

    if (HAL_SPI_Init(&hspi_module) != HAL_OK)
    {
    Error_Handler();
    }

    /* I *think* there's some sort of bug with the way the HAL sets up the GPIO clock pin for use in SPI.
    * No matter what I do with the clock pin, even if I FORCE it low before assigning it as an alternate function, it always asserts itself during initialisation (giving the impression of a high idle state)
    *  --> this has the adverse effect of putting aXiom 1 clock cycle ahead, hence everything comes out LS by 1 (this only happens on the first transaction)
    * My VERY jammy way of getting around this is to pretend to send a byte without asserting the CS pin, this somehow 'tricks' the clock into idling low without triggering aXiom
    * I don't like how this works, but it does...oh well!
    */
    HAL_SPI_Transmit(&hspi_module, temp_send, sizeof(temp_send), 0);
    HAL_SPIEx_FlushRxFifo(&hspi_module);

}

//--------------------------

uint8_t do_spi_comms(void)
{
    uint8_t status = 0;

    switch(spi_state)
    {
        // start transmission
        case TRANSMIT_AND_RECEIVE:
        {
            HAL_SPIEx_FlushRxFifo(&hspi_module);
            HAL_GPIO_WritePin(nSS_SPI_GPIO_Port, nSS_SPI, GPIO_PIN_RESET); /* Pull select line low */
            spi_state = WAIT_FOR_FLAG;

            if((aXiom_NumBytesTx == 4) && (aXiom_NumBytesRx != 0)) // doing a read from aXiom --> requires a write first (asks aXiom to prepare a read)
            {
                // read from aXiom
                status = HAL_SPI_TransmitReceive_DMA(&hspi_module, aXiom_Tx_Buffer, aXiom_Rx_Buffer[CircularBufferHead], SPI_CMD_BYTES + SPI_PADDING_BYTES + aXiom_NumBytesRx);
                break;
            }
            else if ((aXiom_NumBytesTx >= 5) && (aXiom_NumBytesRx == 0)) // just doing a write (writing 1 or more bytes, reading none)
            {
                /* reason for doing this if writing:
                 * padding is required when sending a command to aXiom over SPI to give it time to prepare the registers (32 bytes exactly)
                 * the first 4 bytes contain: {usage_page_high, usage_page_low, num_bytes_to_read, read_write}
                 * these first 4 bytes tell aXiom what to prepare for, so need the 32 byte padding between these and the data to write.
                 * e.g. 4 byte setup --> 32 byte padding --> n bytes to write
                 * SPI_Rx_Buffer starts at the 4th byte (array entry [3]) of the input command from the host --> so aXiom command bytes are in array entry [0] to [3] of SPI_Rx_Buffer
                 * payload starts at array entry [4], so need to move payload along by 32 bytes (this has the effect of adding the padding bytes between aXiom command and payload)
                 */
                uint16_t PayloadLength;
                PayloadLength = (aXiom_NumBytesTx - 4) & 0x7FFF;

                memcpy(aXiom_Rx_Buffer[CircularBufferHead], aXiom_Tx_Buffer, aXiom_NumBytesTx);
                memmove(&aXiom_Rx_Buffer[CircularBufferHead][4 + SPI_PADDING_BYTES], &aXiom_Rx_Buffer[CircularBufferHead][4], PayloadLength);

                // write to aXiom
                status = HAL_SPI_Transmit_DMA(&hspi_module, aXiom_Rx_Buffer[CircularBufferHead], SPI_CMD_BYTES + SPI_PADDING_BYTES + PayloadLength);
                break;
            }
            else // every other case is invalid, should never be in this state unless in error
            {
                HAL_GPIO_WritePin(nSS_SPI_GPIO_Port, nSS_SPI, GPIO_PIN_SET); // Set the nSS line high again
                spi_state = WAIT_FOR_FLAG;
                aXiom_Rx_Buffer[CircularBufferHead][0] = INVALID_SETUP; // comms_status byte
                spi_state = TRANSMIT_AND_RECEIVE; // make sure we go back to the start of the state machine
                boCommsInProcess = 0; // comms haven't actually started but we also don't want to stay in here, so this is how we get out
            }
            break;
        }

        // wait for flag, then transmit status to host via USB
        case WAIT_FOR_FLAG:
        {
            if(spi_TxRx_flag)
            {
                /* doing a read from aXiom */
                if((aXiom_NumBytesTx == 4) && (aXiom_NumBytesRx != 0))
                {
                    // shifts result back to start of the buffer, offset by 2 bytes to allow for "comms_status" and "SPI_NumBytesRx" bytes
                    memmove(&aXiom_Rx_Buffer[CircularBufferHead][2], &aXiom_Rx_Buffer[CircularBufferHead][SPI_CMD_BYTES + SPI_PADDING_BYTES], aXiom_NumBytesRx);
                    aXiom_Rx_Buffer[CircularBufferHead][0] = COMMS_OK; // comms_status byte
                }
                /* just doing a write (writing 4 or more bytes, reading none) */
                else if ((aXiom_NumBytesTx >= 5) && (aXiom_NumBytesRx == 0))
                {
                    aXiom_Rx_Buffer[CircularBufferHead][0] = COMMS_OK_NO_READ; // comms_status byte
                }
                /* every other case is invalid, should never be in this state unless in error */
                else
                {

                }

                aXiom_Rx_Buffer[CircularBufferHead][1] = aXiom_NumBytesRx; // no. read bytes
                spi_state = TRANSMIT_AND_RECEIVE;
                boCommsInProcess = 0; // stop executing this function
                spi_TxRx_flag = 0;

                // Delay to prevent reading in quick succession. Need to give aXiom time to setup the next transfer.
                // Only required for SPI.
                delay_1us(100U);
            }

            break;
        }

        default:
        {
            spi_state = TRANSMIT_AND_RECEIVE;
            break;
        }
    }

    return status;
}
