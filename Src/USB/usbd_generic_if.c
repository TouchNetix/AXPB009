/**
  ******************************************************************************
  * @file           : usbd_generic_hid_if.c
  * @version        : v2.0_Cube
  * @brief          : USB Device Generic HID interface file.
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

/*============ Includes ============*/
#include <_AXPB009_Main.h>
#include "Comms.h"
#include <stdbool.h>
#include "usbd_core.h"
#include "usbd_conf.h"
#include "usbd_mouse.h"
#include "usbd_generic.h"
#include "usbd_press.h"
#include "usbd_mouse_if.h"
#include "usbd_generic_if.h"
#include "usbd_press_if.h"
#include "Mode_Control.h"
#include "stm32f0xx_hal.h"

/*============ Defines ============*/


/*============ Macros ============*/


/*============ Local Variables ============*/


/*============ Exported Types ============*/
USBD_HandleTypeDef hUsbDeviceFS;

/*============ Exported Variables ============*/
volatile bool   boCommandWaitingToDecode = false;
bool    boGenericReportToSend = 0;
uint8_t *pTBPCommandReportGeneric = 0;

 /* USB Generic HID Report Descriptor */
__ALIGN_BEGIN static uint8_t GENERIC_HID_ReportDesc_FS[USBD_GENERIC_HID_REPORT_DESC_SIZE] __ALIGN_END =
{
                 // item code|no. bytes
    0x06, 0xFF, 0xFF,   // 04|2   , Usage Page (vendor defined?)
    0x09, 0x01,         // 08|1   , Usage      (vendor defined
    0xA1, 0x01,         // A0|1   , Collection (Application)
    /* 7 bytes */

    // IN report
    0x09, 0x01,         // 08|1   , Usage      (vendor defined)
    0x15, 0x00,         // 14|1   , Logical Minimum(0 for signed byte?)
    0x26, 0xFF, 0x00,   // 24|1   , Logical Maximum(255 for signed byte?)
    0x75, 0x08,         // 74|1   , Report Size(8) = field size in bits = 1 byte
    0x95, USBD_GENERIC_HID_REPORT_IN_SIZE,		// 94|2 ReportCount(size) = repeat count of previous item, 64 byte IN report
    0x81, 0x02,         // 80|1   , IN report (Data,Variable, Absolute)
    /* 20 bytes */

    // OUT report
    0x09, 0x02,         // 08|1   , Usage      (vendor defined)
    0x15, 0x00,         // 14|1   , Logical Minimum(0 for signed byte?)
    0x26, 0xFF, 0x00,   // 24|1   , Logical Maximum(255 for signed byte?)
    0x75, 0x08,         // 74|1   , Report Size(8) = field size in bits = 1 byte
    0x95, USBD_GENERIC_HID_REPORT_OUT_SIZE,   // 94|2 ReportCount(size) = repeat count of previous item, 64 byte OUT report
    0x91, 0x02,         // 90|1   , OUT report (Data,Variable, Absolute)
    /* 33 bytes */

    // Feature report
    0x09, 0x03,         // 08|1   , Usage      (vendor defined)
    0x15, 0x00,         // 14|1   , LogicalMinimum(0 for signed byte)
    0x26, 0xFF, 0x00,   // 24|1   , Logical Maximum(255 for signed byte)
    0x75, 0x08,         // 74|1   , Report Size(8) =field size in bits = 1 byte
    0x95, USBD_GENERIC_HID_FEATURE_SIZE,         // 94|1   , ReportCount in byte
    0xB1, 0x02,         // B0|1   , Feature report
    0xC0                // C0|0   , End Collection
    /* 47 bytes */
};

/*============ Local Function Prototypes ============*/
static int8_t GENERIC_HID_Init_FS(void);
static int8_t GENERIC_HID_DeInit_FS(void);
static int8_t GENERIC_HID_OutEvent_FS(uint8_t* state);

/*============ TypeDefs ============*/
USBD_GENERIC_HID_ItfTypeDef USBD_GenericHID_fops_FS =
{
  GENERIC_HID_ReportDesc_FS,
  GENERIC_HID_Init_FS,
  GENERIC_HID_DeInit_FS,
  GENERIC_HID_OutEvent_FS
};

/*============ Function ============*/

/**
  * @brief  Initializes the GENERIC HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t GENERIC_HID_Init_FS(void)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  DeInitializes the GENERIC HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t GENERIC_HID_DeInit_FS(void)
{
  /* USER CODE BEGIN 5 */
  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Manage the GENERIC HID class events
  * @param  event_idx: Event index
  * @param  state: Event state
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t GENERIC_HID_OutEvent_FS(uint8_t* state)
{
    /* USER CODE BEGIN 6 */
    GPIOC->ODR ^= GPIO_ODR_9;

    // set pointer to out buffer
    pTBPCommandReportGeneric = state;

    // temporarily stop all reports
    boBlockReports = 1;

    // set flag to tell to process instruction and say which interface it came from
    boCommandWaitingToDecode = 1;
    target_interface = GENERIC_INTERFACE_NUM;
    return (USBD_OK);
    /* USER CODE END 6 */
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

