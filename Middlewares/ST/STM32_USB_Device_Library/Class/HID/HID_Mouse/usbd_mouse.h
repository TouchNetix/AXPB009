/**
  ******************************************************************************
  * @file    usbd_mouse.h
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief   header file for the usbd_mouse.c file.
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
#ifndef __USB_MOUSEHID_H
#define __USB_MOUSEHID_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include  "usbd_ioreq.h"
#include "usbd_composite.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_MOUSE_HID
  * @brief This file is the Header file for USBD_mouse.c
  * @{
  */


/** @defgroup USBD_MOUSE_HID_Exported_Defines
  * @{
  */
#define MOUSE_PARALLEL_DIGITIZER_REPORT_LENGTH  (39)
#define MOUSE_ABS_REPORT_LENGTH                 (5)
#define MOUSE_REL_REPORT_LENGTH                 (4)
#define MOUSE_TOUCHPAD_REPORT_LENGTH            (5U + (PRECISION_TPAD_MAX_CONTACT_CT * 5U))

#define USB_MOUSE_HID_CONFIG_DESC_SIZ           (41)
#define USB_MOUSE_HID_DESC_SIZ                  (9)

#define MOUSE_HID_DESCRIPTOR_TYPE               (0x21)
#define MOUSE_HID_REPORT_DESC                   (0x22)


#define MOUSE_HID_REQ_SET_PROTOCOL              (0x0B)
#define MOUSE_HID_REQ_GET_PROTOCOL              (0x03)

#define MOUSE_HID_REQ_SET_IDLE                  (0x0A)
#define MOUSE_HID_REQ_GET_IDLE                  (0x02)

#define MOUSE_HID_REQ_SET_REPORT                (0x09)
#define MOUSE_HID_REQ_GET_REPORT                (0x01)
/**
  * @}
  */


/** @defgroup USBD_CORE_Exported_TypesDefinitions
  * @{
  */
typedef enum
{
  MOUSE_HID_IDLE = 0,
  MOUSE_HID_BUSY,
}
MOUSE_HID_StateTypeDef;

typedef struct _USBD_MOUSE_HID_Itf
{
  uint8_t                  *pReport;
  int8_t (* Init)          (void);
  int8_t (* DeInit)        (void);
  int8_t (* OutEvent)      (uint8_t*);
  int8_t (* SetFeature)    (uint8_t, uint8_t*);
  int8_t (* GetFeature)    (uint8_t, uint8_t*, uint16_t*);

}USBD_MOUSE_HID_ItfTypeDef;

typedef struct _USBD_MOUSE_HID_HandleTypeDef
{
  uint8_t              Report_buf[USBD_MOUSE_HID_OUTREPORT_BUF_SIZE];
  uint32_t             Protocol;
  uint32_t             IdleState;
  uint32_t             AltSetting;
  uint32_t             IsReportAvailable;
  MOUSE_HID_StateTypeDef     state;
}
USBD_MOUSE_HID_HandleTypeDef;
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

extern USBD_ClassTypeDef  USBD_MOUSE_HID;
#define USBD_MOUSE_HID_CLASS    &USBD_MOUSE_HID
/**
  * @}
  */

/** @defgroup USB_CORE_Exported_Functions
  * @{
  */
//CUSTOMISED
USB_HID_StateTypeDef USBD_MOUSE_HID_GetState     (USBD_HandleTypeDef  *pdev);
uint8_t SendReport_MouseEndpoint(USBD_HandleTypeDef  *pdev, uint8_t *report, uint16_t len);

uint8_t USBD_MOUSE_HID_SendReport (USBD_HandleTypeDef *pdev,
                                 uint8_t *report,
                                 uint16_t len);



uint8_t  USBD_MOUSE_HID_RegisterInterface  (USBD_HandleTypeDef   *pdev,
                                             USBD_MOUSE_HID_ItfTypeDef *fops);

void MatchReportDescriptorToMode(USBD_HandleTypeDef *pdev, uint8_t BridgeMode);
void GetMouseDescriptorLength(uint8_t BridgeMode);
void ConfigurePID(uint8_t BridgeMode);
void ConfigureCfgDescriptor(uint8_t MousMode);

uint8_t  USBD_MOUSE_HID_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx);

uint8_t  USBD_MOUSE_HID_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx);

uint8_t  USBD_MOUSE_HID_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req);

uint8_t  *USBD_MOUSE_HID_GetCfgDesc (uint16_t *length);

uint8_t  *USBD_MOUSE_HID_GetDeviceQualifierDesc (uint16_t *length);

uint8_t  USBD_MOUSE_HID_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);

uint8_t  USBD_MOUSE_HID_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t  USBD_MOUSE_HID_EP0_RxReady (USBD_HandleTypeDef  *pdev);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif  /* __USB_MOUSEHID_H */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
