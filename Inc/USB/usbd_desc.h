/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_desc.c
  * @version        : v2.0_Cube
  * @brief          : Header for usbd_conf.c file.
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_DESC__C__
#define __USBD_DESC__C__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_def.h"

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_DESC USBD_DESC
  * @brief Usb device descriptors module.
  * @{
  */
  
/** @defgroup USBD_DESC_Exported_Constants USBD_DESC_Exported_Constants
  * @brief Constants.
  * @{
  */
#define         DEVICE_ID1          (UID_BASE)
#define         DEVICE_ID2          (UID_BASE + 0x4)
#define         DEVICE_ID3          (UID_BASE + 0x8)

#define  USB_SIZ_STRING_SERIAL       (0x1A)

/* USER CODE BEGIN EXPORTED_CONSTANTS */

/* USER CODE END EXPORTED_CONSTANTS */

/**
  * @}
  */

/** @defgroup USBD_DESC_Exported_Defines USBD_DESC_Exported_Defines
  * @brief Defines.
  * @{
  */

/* USER CODE BEGIN EXPORTED_DEFINES */
// XXX USB Firmware revision, interface names, VID and PIDs
#define USB_DEVICE_MAJOR_VERSION                (0x03)
#define USB_DEVICE_MINOR_VERSION                (0x00) // these are both BCD, e.g. 0x12 means .12
#define USBD_VID                                (0x0483)
#define DEVICE_MODE_PID_TBP                     (0x6F02u)
#define DEVICE_MODE_PID_PARALLEL_DIGITIZER      (0x2F04u)
#define DEVICE_MODE_PID_ABSOLUTE_MOUSE          (0x2F08u)
#define USBD_LANGID_STRING                      (1033)
#define USBD_MANUFACTURER_STRING                "TouchNetix"
#define USBD_PRODUCT_STRING_FS                  "AXPB009"
#define USBD_CONFIGURATION_STRING_FS            "AXPB009"
#define USBD_INTERFACE_GENERIC_STRING_FS        "AXPB009 Control"
#define USBD_INTERFACE_GENERIC_DIGI_STRING_FS   "AXPB009 Control"
#define USBD_INTERFACE_DIGITIZER_ON_STRING_FS   "AXPB009 Digitizer"
#define USBD_INTERFACE_ABS_MOUSE_ON_STRING_FS   "AXPB009 AbsMouse"
#define USBD_INTERFACE_PRESS_STRING_FS          "AXPB009 Press Data"
/* USER CODE END EXPORTED_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_DESC_Exported_TypesDefinitions USBD_DESC_Exported_TypesDefinitions
  * @brief Types.
  * @{
  */

/* USER CODE BEGIN EXPORTED_TYPES */

/* USER CODE END EXPORTED_TYPES */

/**
  * @}
  */

/** @defgroup USBD_DESC_Exported_Macros USBD_DESC_Exported_Macros
  * @brief Aliases.
  * @{
  */

/* USER CODE BEGIN EXPORTED_MACRO */

/* USER CODE END EXPORTED_MACRO */

/**
  * @}
  */

/** @defgroup USBD_DESC_Exported_Variables USBD_DESC_Exported_Variables
  * @brief Public variables.
  * @{
  */

/** Descriptor for the Usb device. */
extern USBD_DescriptorsTypeDef FS_Desc;

/* USER CODE BEGIN EXPORTED_VARIABLES */
extern uint8_t USBD_FS_DeviceDesc[USB_LEN_DEV_DESC];
/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_DESC_Exported_FunctionsPrototype USBD_DESC_Exported_FunctionsPrototype
  * @brief Public functions declaration.
  * @{
  */

/* USER CODE BEGIN EXPORTED_FUNCTIONS */

/* USER CODE END EXPORTED_FUNCTIONS */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __USBD_DESC__C__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
