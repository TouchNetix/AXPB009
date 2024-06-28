/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usb_device.c
  * @version        : v2.0_Cube
  * @brief          : This file implements the USB Device
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "Init.h"
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_composite.h"
#include "usbd_generic_if.h"
#include "usbd_mouse_if.h"
#include "usbd_press_if.h"
#include "Flash_Control.h"
#include "Digitizer.h"
#include "Usage_Builder.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USB Device Core handle declaration. */
USBD_HandleTypeDef hUsbDeviceFS;

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/*
 * -- Insert your external function declaration here --
 */
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * Init USB device Library, add supported class and start the library
  * @retval None
  */
void MX_USB_DEVICE_Init(void)
{
    /* USER CODE BEGIN USB_DEVICE_Init_PreTreatment */
    ConfigurePID(BridgeMode);
    GetMouseDescriptorLength(BridgeMode);
    ConfigureCfgDescriptor(BridgeMode);

    // set various parameters (like VID + PID) if defined by user in aXiom firmware
    if(u35_addr != 0)
    {
        configure_HID_PARAMETER_IDs();
    }

    if(BridgeMode == MODE_PRECISION_TOUCHPAD)
    {
        update_touchpad_threshold();
    }

    /* USER CODE END USB_DEVICE_Init_PreTreatment */

    /* Init Device Library, add supported class and start the library. */
    if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK)
    {
    Error_Handler();
    }
    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_COMPOSITE_HID) != USBD_OK)
    {
    Error_Handler();
    }
    if (USBD_GENERIC_HID_RegisterInterface(&hUsbDeviceFS, &USBD_GenericHID_fops_FS) != USBD_OK)
    {
    Error_Handler();
    }
    if (USBD_PRESS_HID_RegisterInterface(&hUsbDeviceFS, &USBD_PressHID_fops_FS) != USBD_OK)
    {
    Error_Handler();
    }
    if (USBD_MOUSE_HID_RegisterInterface(&hUsbDeviceFS, &USBD_MouseHID_fops_FS) != USBD_OK)
    {
    Error_Handler();
    }

    MatchReportDescriptorToMode(&hUsbDeviceFS, BridgeMode);  // TH2 tells bridge which mode it wants the mouse interface in before reset --> stored in flash so bridge knows mode required at start-up

    if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
    {
    Error_Handler();
    }

    /* USER CODE BEGIN USB_DEVICE_Init_PostTreatment */
    boUSBTimeoutEnabled = true; // starts the countdown timer to enable proxy mode
    /* USER CODE END USB_DEVICE_Init_PostTreatment */
}

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
