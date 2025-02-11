/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_conf.h
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
#ifndef __USBD_CONF__H__
#define __USBD_CONF__H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <_AXPB009_Main.h>
#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/** @addtogroup USBD_OTG_DRIVER
  * @{
  */

/** @defgroup USBD_CONF USBD_CONF
  * @brief Configuration file for Usb otg low level driver.
  * @{
  */

/** @defgroup USBD_CONF_Exported_Variables USBD_CONF_Exported_Variables
  * @brief Public variables.
  * @{
  */
extern PCD_HandleTypeDef hpcd_USB_FS;
/**
  * @}
  */

/** @defgroup USBD_CONF_Exported_Defines USBD_CONF_Exported_Defines
  * @brief Defines for configuration of the Usb device.
  * @{
  */

// XXX: USB configuration parameters
/*---------- -----------*/
#define USBD_MAX_NUM_INTERFACES                 (0x03)
#define USBD_GENERIC_AND_PRESS_INTERFACES_ONLY  (0x02)
/*---------- -----------*/
#define USBD_MAX_NUM_CONFIGURATION              (1)
/*---------- -----------*/
#define USBD_GENERIC_HID_REPORT_IN_SIZE         (64)// limited by USB 2.0 spec, can't be any bigger!
#define USBD_GENERIC_HID_REPORT_OUT_SIZE        (64)
#define USBD_GENERIC_HID_FEATURE_SIZE           (4)
/*---------- -----------*/
#define USBD_MOUSE_HID_REPORT_IN_SIZE           (64)
#define USBD_MOUSE_HID_REPORT_OUT_SIZE          (64)
 /*---------- -----------*/
#define USBD_PRESS_HID_REPORT_IN_SIZE           (64)
#define USBD_PRESS_HID_REPORT_OUT_SIZE          (64)
#define USBD_PRESS_HID_FEATURE_SIZE             (4)
/*---------- -----------*/
#define USBD_MAX_STR_DESC_SIZ                   (512)
/*---------- -----------*/
// XXX USBD_SUPPORT_USER_STRING is set to 1 by default when using generated code (There isn't an option to adjust this in STM32CubeMX) --> results in a crash when Windows asks for it in usbd_ctlreq.c
#define USBD_SUPPORT_USER_STRING                (0)
/*---------- -----------*/
#define USBD_DEBUG_LEVEL                        (0)
/*---------- -----------*/
#define USBD_SELF_POWERED                       (1)
/*---------- -----------*/
#define USBD_GENERIC_HID_OUTREPORT_BUF_SIZE     (64)
#define USBD_MOUSE_HID_OUTREPORT_BUF_SIZE       (64)
#define USBD_PRESS_HID_OUTREPORT_BUF_SIZE       (64)
/*---------- -----------*/
#define USBD_PRESS_HID_REPORT_DESC_SIZE         (53)
#define USBD_MOUSE_ABS_REPORT_DESC_SIZE         (58)
#define USBD_MOUSE_ABS_REPORT_DESC_SIZE_LO      (LOBYTE(USBD_MOUSE_ABS_REPORT_DESC_SIZE))
#define USBD_MOUSE_ABS_REPORT_DESC_SIZE_HI      (HIBYTE(USBD_MOUSE_ABS_REPORT_DESC_SIZE))
#define USBD_MOUSE_PAR_DIGITIZER_DESC_SIZE      (464)//(474)
#define USBD_MOUSE_PAR_DIGITIZER_DESC_SIZE_LO   (LOBYTE(USBD_MOUSE_PAR_DIGITIZER_DESC_SIZE))
#define USBD_MOUSE_PAR_DIGITIZER_DESC_SIZE_HI   (HIBYTE(USBD_MOUSE_PAR_DIGITIZER_DESC_SIZE))
#define USBD_GENERIC_HID_REPORT_DESC_SIZE       (49 + USBD_MOUSE_PAR_DIGITIZER_DESC_SIZE)
 /*---------- -----------*/
/****************************************/
/* #define for FS and HS identification */
#define DEVICE_FS 		0

/**
  * @}
  */

/** @defgroup USBD_CONF_Exported_Macros USBD_CONF_Exported_Macros
  * @brief Aliases.
  * @{
  */

/* Memory management macros */

/** Alias for memory allocation. */
#define USBD_malloc_generic       (uint32_t *)USBD_static_malloc_generic
#define USBD_malloc_press         (uint32_t *)USBD_static_malloc_press
#define USBD_malloc_mouse         (uint32_t *)USBD_static_malloc_mouse

// CUSTOMISED - static malloc size
#define MAX_BYTES_STATIC_ALLOC_SIZE	84 /* HID Class structure size in BYTES - set to be the same size as USBD_GENERIC_HID_HandleTypeDef struct (same as USBD_MOUSE_HID_HandleTypeDef and USBD_PRESS_HID_HandleTypeDef) */

/** Alias for memory release. */
#define USBD_free           USBD_static_free

/** Alias for memory set. */
#define USBD_memset         /* Not used */

/** Alias for memory copy. */
#define USBD_memcpy         /* Not used */

/** Alias for delay. */
#define USBD_Delay          HAL_Delay

/* DEBUG macros */

#if (USBD_DEBUG_LEVEL > 0)
#define USBD_UsrLog(...)    printf(__VA_ARGS__);\
                            printf("\n");
#else
#define USBD_UsrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 1)

#define USBD_ErrLog(...)    printf("ERROR: ") ;\
                            printf(__VA_ARGS__);\
                            printf("\n");
#else
#define USBD_ErrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 2)
#define USBD_DbgLog(...)    printf("DEBUG : ") ;\
                            printf(__VA_ARGS__);\
                            printf("\n");
#else
#define USBD_DbgLog(...)
#endif

/**
  * @}
  */

/** @defgroup USBD_CONF_Exported_Types USBD_CONF_Exported_Types
  * @brief Types.
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_CONF_Exported_FunctionsPrototype USBD_CONF_Exported_FunctionsPrototype
  * @brief Declaration of public functions for Usb device.
  * @{
  */

/* Exported functions -------------------------------------------------------*/
void *USBD_static_malloc_generic(uint32_t size);
void *USBD_static_malloc_press(uint32_t size);
void *USBD_static_malloc_mouse(uint32_t size);
void USBD_static_free(void *p);
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

#endif /* __USBD_CONF__H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
