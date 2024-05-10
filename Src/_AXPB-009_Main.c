/*******************************************************************************
* @file           : _AXPB-009_Main.c
* @author         : James Cameron (TouchNetix)
* @created        : 22 Sep 2020
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
#include "_AXPB009_Main.h"
#include "usbd_composite.h"
#include "usb_device.h"
#include "Init.h"
#include "usbd_generic_if.h"
#include "usbd_generic.h"
#include "usbd_mouse_if.h"
#include "usbd_mouse.h"
#include "usbd_press_if.h"
#include "usbd_press.h"
#include "stm32f0xx.h"
#include "SPI_comms.h"
#include "Comms.h"
#include "Command_Processor.h"
#include "Proxy_driver.h"
#include "Digitizer.h"
#include "Press_driver.h"
#include "Usage_Builder.h"
#include "Mode_Control.h"
#include "Timers_and_LEDs.h"
#include "Flash_Control.h"

/*============ TypeDefs ============*/

/*============ Defines ============*/
#define CONTROL_ENDPOINT        (0)
#define MOUSE_ENDPOINT          (1)
#define PRESS_ENDPOINT          (2)
#define USB_STARTUP_DELAY_MS    (200)

#define FILLED_BUFFER   (0U)
#define EMPTIED_BUFFER  (1U)

/*============ Macros ============*/

/*============ Local Variables ============*/
uint16_t USB_disconnect_count = 0;

/*============ Local Function Prototypes ============*/
void MoveCircularBuffer(uint8_t operation);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    // initialise device: peripherals, clocks, GPIO pins, USB, timers etc.
    Device_Init();

    /* Infinite loop */
    while (1)
    {
        /* Waits a bit before enabling proxy mode */
        if(wdUSB1msTick > USB_STARTUP_DELAY_MS)
        {
            setup_proxy_for_digitizer();
            boUSBTimeoutEnabled = false;
            boInternalProxy = 1;    // tells us that this is an internal proxy call - don't send generic reports!
        }

        /* check if host has sent command to bridge */
        if(boCommandWaitingToDecode == 1)
        {
            ProcessTBPCommand();
        }

        // send response to host
        if((boGenericTBPResponseWaiting == 1) && (USBD_GENERIC_HID_GetState(&hUsbDeviceFS) == USB_HID_IDLE))
        {
            Send_USB_Report(GENERIC, &hUsbDeviceFS, pTBPCommandReport, USBD_GENERIC_HID_REPORT_IN_SIZE);
            boGenericTBPResponseWaiting = 0;
        }


        if((boPressTBPResponseWaiting == 1) && (USBD_PRESS_HID_GetState(&hUsbDeviceFS) == USB_HID_IDLE))
        {
            Send_USB_Report(PRESS, &hUsbDeviceFS, pTBPCommandReport, USBD_PRESS_HID_REPORT_IN_SIZE);
            boPressTBPResponseWaiting = 0;
        }

        /* send reports to host flat out! */
        // only performs a proxy cycle if proxy mode is enabled, a command hasn't been sent by the host AND the connected device has a report available
        if(((boProxyEnabled == 1) || (boInternalProxy == 1)) && (boCommandWaitingToDecode == 0) && (HAL_GPIO_ReadPin(GPIOA, nIRQ_Pin) == 0))
        {
            bool boGotData = 0;

            boProxyReportAvailable = 1;
            boGotData = ProxyExecute(false);

            if(boGotData == 1)
            {
                if(boInternalProxy == 0)
                {
                    /* host requested proxy mode so send reports up the generic endpoint */
                    boGenericReportToSend = 1;

                    MoveCircularBuffer(FILLED_BUFFER);
                }

                if(boMouseEnabled == true)  // only enable digitizer/mouse reports if we're in the correct mode!
                {
                    if(BridgeMode == MODE_PARALLEL_DIGITIZER) // check if we're in multipoint digitizer mode
                    {
                        MultiPointDigitizer();
                    }
                    else if(BridgeMode == MODE_ABSOLUTE_MOUSE)
                    {
                        MouseDigitizer();
                    }
                    else if(BridgeMode == MODE_RELATIVE_MOUSE)
                    {
                        MouseDigitizer();
                    }
                }

                // press endpoint is always active so this is always executed
                BuildPressReportFromPressAndTouch();

            }
        }
        else if(ProxyMP_TotalNumBytesRx != 0)   // if this variable is non-zero, it means a 3D read is happening so enter here
        {
            bool boGotData = 0;

            // will continue to return 'false' until transfer is complete and host has received a payload
            boGotData = ProxyExecute(true);

            //waits for data to be read from device and sent to host before preparing the next read
            if(boGotData == true)
            {
                uint8_t byPagesMovedThrough;
                uint8_t byBytesOffsetIntoPage;

                MoveCircularBuffer(FILLED_BUFFER);

                // if we're doing 3D data we don't care about any other endpoint so fine to wait here for the hardware to become free
                while(USBD_GENERIC_HID_GetState(&hUsbDeviceFS) == USB_HID_BUSY);
                Send_USB_Report(GENERIC, &hUsbDeviceFS, aXiom_Rx_Buffer[CircularBufferTail], USBD_GENERIC_HID_REPORT_IN_SIZE);
                MoveCircularBuffer(EMPTIED_BUFFER);

                ProxyMP_TotalNumBytesRx -= aXiom_NumBytesRx;  //subract the number of bytes just read --> leaves how may bytes left to read
                wdProxyMP_BytesRead += aXiom_NumBytesRx;  // also keeps track of number of bytes read, but counts up from 0 (allows us to set starting page and offset correctly for next transfer)

                // set no. read bytes to either 58 (maximum read size) or however many bytes are left
                // --> 3 bytes come from 'nonsense' at end of read(?)
                //     2 bytes for header (command and comms status)
                //     1 byte to make it match what TH2 is expecting!?
                aXiom_NumBytesRx = (ProxyMP_TotalNumBytesRx < NUMBYTES_RX_MP) ? ProxyMP_TotalNumBytesRx : NUMBYTES_RX_MP;

                // calculate the next starting address and offset required (e.g. start at 0, read 64, so new starting address is 0x0064)
                byPagesMovedThrough   = wdProxyMP_BytesRead / (byProxyMP_PageLength == 0 ? (uint16_t)256 : (uint16_t)byProxyMP_PageLength);
                byBytesOffsetIntoPage = wdProxyMP_BytesRead % (byProxyMP_PageLength == 0 ? (uint16_t)256 : (uint16_t)byProxyMP_PageLength);

                // set address bytes for Tx
                aXiom_Tx_Buffer[0] = (uint8_t)((wdProxyMP_AddrStart & 0xFF) + byBytesOffsetIntoPage);
                aXiom_Tx_Buffer[1] = (uint8_t)((wdProxyMP_AddrStart >> 8) + byPagesMovedThrough);
            }
        }

        /* Linux won't accept/deal with a packet unless an application is run to handle it so the code will lock up if the endpoint checks are tied together
         * (like they used to be)
         * e.g. Press endpoint always sends packets and will be a hog until the host has received it, which Linux won't without an application running
         * Instead, each endpoint needs an independent check that blocks a new packet from being sent if one is in process, but doesn't prevent other endpoints
         * from sending */
        if((boGenericReportToSend == 1) && (USBD_GENERIC_HID_GetState(&hUsbDeviceFS) == USB_HID_IDLE) && (usb_remote_wake_state == RESUMED))
        {
            Send_USB_Report(GENERIC, &hUsbDeviceFS, aXiom_Rx_Buffer[CircularBufferTail], USBD_GENERIC_HID_REPORT_IN_SIZE);
            MoveCircularBuffer(EMPTIED_BUFFER);

            if(CircularBufferTail == CircularBufferHead)
            {
                boGenericReportToSend = 0;
            }
        }

        if((boMouseReportToSend == 1) && (USBD_MOUSE_HID_GetState(&hUsbDeviceFS) == USB_HID_IDLE) && (usb_remote_wake_state == RESUMED))
        {
            Send_USB_Report(MOUSE, &hUsbDeviceFS, usb_hid_mouse_report_in, byMouseReportLength);
            boMouseReportToSend = 0;
        }

        if((boPressReportToSend == 1) && (USBD_PRESS_HID_GetState(&hUsbDeviceFS) == USB_HID_IDLE) && (usb_remote_wake_state == RESUMED))
        {
            Send_USB_Report(PRESS, &hUsbDeviceFS, usb_hid_press_report_in, USBD_PRESS_HID_REPORT_IN_SIZE);
            boPressReportToSend = 0;
        }

        // turns off the LEDs if a recent comms event has turned them on
        LED_control();
        comms_detect_inactivity();
    }
}

//============= Interrupt handlers =============//

//============= Local Functions =============//

void MoveCircularBuffer(uint8_t operation)
{
    if(operation == FILLED_BUFFER)
    {
        CircularBufferHead ++;
        CircularBufferHead %= MAX_NUM_RX_BUFFERS;
    }
    else //operation == EMPTIED_BUFFER
    {
        CircularBufferTail ++;
        CircularBufferTail %= MAX_NUM_RX_BUFFERS;
    }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add their own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
