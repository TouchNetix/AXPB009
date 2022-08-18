/**
  ******************************************************************************
  * @file    usbd_generic.h
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief   header file for the usbd_generic.c file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_GENERICHID_H
#define __USB_GENERICHID_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include  "usbd_ioreq.h"

#include "usbd_composite.h"
/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_GENERIC_HID
  * @brief This file is the Header file for USBD_generic.c
  * @{
  */


/** @defgroup USBD_GENERIC_HID_Exported_Defines
  * @{
  */

#define USB_GENERIC_HID_CONFIG_DESC_SIZ       (41)
#define USB_GENERIC_HID_DESC_SIZ              (9)

#define GENERIC_HID_DESCRIPTOR_TYPE           (0x21)
#define GENERIC_HID_REPORT_DESC               (0x22)


#define GENERIC_HID_REQ_SET_PROTOCOL          (0x0B)
#define GENERIC_HID_REQ_GET_PROTOCOL          (0x03)

#define GENERIC_HID_REQ_SET_IDLE              (0x0A)
#define GENERIC_HID_REQ_GET_IDLE              (0x02)

#define GENERIC_HID_REQ_SET_REPORT            (0x09)
#define GENERIC_HID_REQ_GET_REPORT            (0x01)
/**
  * @}
  */


/** @defgroup USBD_CORE_Exported_TypesDefinitions
  * @{
  */
typedef enum
{
  GENERIC_HID_IDLE = 0,
  GENERIC_HID_BUSY,
}
GENERIC_HID_StateTypeDef;

typedef struct _USBD_GENERIC_HID_Itf
{
  uint8_t                  *pReport;
  int8_t (* Init)          (void);
  int8_t (* DeInit)        (void);
  int8_t (* OutEvent)      (uint8_t*);

}USBD_GENERIC_HID_ItfTypeDef;

typedef struct _USBD_GENERIC_HID_HandleTypeDef
{
  uint8_t              Report_buf[USBD_GENERIC_HID_OUTREPORT_BUF_SIZE];
  uint32_t             Protocol;
  uint32_t             IdleState;
  uint32_t             AltSetting;
  uint32_t             IsReportAvailable;
  GENERIC_HID_StateTypeDef     state;
}
USBD_GENERIC_HID_HandleTypeDef;
/**
  * @}
  */



/** @defgroup USBD_CORE_Exported_Macros
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_CORE_Exported_Variables
  * @{
  */

extern USBD_ClassTypeDef  USBD_GENERIC_HID;
#define USBD_GENERIC_HID_CLASS    &USBD_GENERIC_HID
/**
  * @}
  */

/** @defgroup USB_CORE_Exported_Functions
  * @{
  */
//CUSTOMISED
USB_HID_StateTypeDef USBD_GENERIC_HID_GetState     (USBD_HandleTypeDef  *pdev);
uint8_t SendReport_ControlEndpoint(USBD_HandleTypeDef  *pdev, uint8_t *report, uint16_t len);

uint8_t USBD_GENERIC_HID_SendReport (USBD_HandleTypeDef *pdev,
                                 uint8_t *report,
                                 uint16_t len);

uint8_t  USBD_GENERIC_HID_RegisterInterface  (USBD_HandleTypeDef   *pdev,
                                             USBD_GENERIC_HID_ItfTypeDef *fops);

uint8_t  USBD_GENERIC_HID_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx);

uint8_t  USBD_GENERIC_HID_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx);

uint8_t  USBD_GENERIC_HID_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req);

uint8_t  *USBD_GENERIC_HID_GetCfgDesc (uint16_t *length);

uint8_t  *USBD_GENERIC_HID_GetDeviceQualifierDesc (uint16_t *length);

uint8_t  USBD_GENERIC_HID_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);

uint8_t  USBD_GENERIC_HID_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t  USBD_GENERIC_HID_EP0_RxReady (USBD_HandleTypeDef  *pdev);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif  /* __USB_GENERICHID_H */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
