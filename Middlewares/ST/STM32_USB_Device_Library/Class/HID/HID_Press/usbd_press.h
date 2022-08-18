/*
 * usbd_press.h
 *
 *  Created on: 28 Sep 2020
 *      Author: James Cameron (TouchNetix)
 */

#ifndef __USB_PRESSHID_H
#define __USB_PRESSHID_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include  "usbd_ioreq.h"
#include "usbd_composite.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_PRESS_HID_press.c
  * @{
  */


/** @defgroup USBD_PRESS_HID_Exported_Defines
  * @{
  */

#define USB_PRESS_HID_CONFIG_DESC_SIZ       (41)
#define USB_PRESS_HID_DESC_SIZ              (9)

#define PRESS_HID_DESCRIPTOR_TYPE           (0x21)
#define PRESS_HID_REPORT_DESC               (0x22)


#define PRESS_HID_REQ_SET_PROTOCOL          (0x0B)
#define PRESS_HID_REQ_GET_PROTOCOL          (0x03)

#define PRESS_HID_REQ_SET_IDLE              (0x0A)
#define PRESS_HID_REQ_GET_IDLE              (0x02)

#define PRESS_HID_REQ_SET_REPORT            (0x09)
#define PRESS_HID_REQ_GET_REPORT            (0x01)
/**
  * @}
  */


/** @defgroup USBD_CORE_Exported_TypesDefinitions
  * @{
  */
typedef enum
{
  PRESS_HID_IDLE = 0,
  PRESS_HID_BUSY,
}
PRESS_HID_StateTypeDef;

typedef struct _USBD_PRESS_HID_Itf
{
  uint8_t                  *pReport;
  int8_t (* Init)          (void);
  int8_t (* DeInit)        (void);
  int8_t (* OutEvent)      (uint8_t*);

}USBD_PRESS_HID_ItfTypeDef;

typedef struct _USBD_PRESS_HID_HandleTypeDef
{
  uint8_t              Report_buf[USBD_PRESS_HID_OUTREPORT_BUF_SIZE];
  uint32_t             Protocol;
  uint32_t             IdleState;
  uint32_t             AltSetting;
  uint32_t             IsReportAvailable;
  PRESS_HID_StateTypeDef     state;
}
USBD_PRESS_HID_HandleTypeDef;
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

extern USBD_ClassTypeDef  USBD_PRESS_HID;
#define USBD_PRESS_HID_CLASS    &USBD_PRESS_HID
/**
  * @}
  */

/** @defgroup USB_CORE_Exported_Functions
  * @{
  */
//CUSTOMISED
USB_HID_StateTypeDef USBD_PRESS_HID_GetState     (USBD_HandleTypeDef  *pdev);
uint8_t SendReport_PressEndpoint(USBD_HandleTypeDef  *pdev, uint8_t *report, uint16_t len);

uint8_t USBD_PRESS_HID_SendReport (USBD_HandleTypeDef *pdev,
                                 uint8_t *report,
                                 uint16_t len);



uint8_t  USBD_PRESS_HID_RegisterInterface  (USBD_HandleTypeDef   *pdev,
                                             USBD_PRESS_HID_ItfTypeDef *fops);

uint8_t  USBD_PRESS_HID_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx);

uint8_t  USBD_PRESS_HID_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx);

uint8_t  USBD_PRESS_HID_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req);

uint8_t  *USBD_PRESS_HID_GetCfgDesc (uint16_t *length);

uint8_t  *USBD_PRESS_HID_GetDeviceQualifierDesc (uint16_t *length);

uint8_t  USBD_PRESS_HID_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);

uint8_t  USBD_PRESS_HID_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t  USBD_PRESS_HID_EP0_RxReady (USBD_HandleTypeDef  *pdev);

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __USB_PRESSHID_H */
