/**
  ******************************************************************************
  * @file    usbd_COMPOSITE.h
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief   header file for the usbd_COMPOSITE.c file.
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
#ifndef __USB_COMPOSITEHID_H
#define __USB_COMPOSITEHID_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include  "usbd_ioreq.h"

/** @defgroup USBD_COMPOSITE_HID_Exported_Defines
  * @{
  */

// todo maybe add a compiler check here to make sure the PMA is ordered/sized correctly! e.g. ep0 address should come AFTER the buffer descriptor table end address

#define EP0_EP_IDX                            (0x00)
#define GENERIC_EPOUT_IDX                     (0x01)
#define GENERIC_EPIN_IDX                      (0x02)
#define MOUSE_EPIN_IDX                        (0x03)

#define TOTAL_NUM_ENDPOINTS                   (5U)
#define BUFFER_DESCRIPTOR_TABLE_ENTRY_SIZE    (8U)

#define EP_IN_DIRECTION                       (0x80)

#define EP0_EPOUT_ADDR                        (TOTAL_NUM_ENDPOINTS * BUFFER_DESCRIPTOR_TABLE_ENTRY_SIZE)
#define EP0_EPIN_ADDR                         (EP0_EPOUT_ADDR + 0x40)
#define GENERIC_HID_EPOUT_ADDR                (EP0_EPIN_ADDR + 0x40)
#define GENERIC_HID_EPIN_ADDR                 (GENERIC_HID_EPOUT_ADDR + 0x40)
#define MOUSE_HID_EPIN_ADDR                   (GENERIC_HID_EPIN_ADDR + 0x40)

#define EP0_EPOUT                             (EP0_EP_IDX)
#define EP0_EPIN                              (EP0_EP_IDX | EP_IN_DIRECTION)

#define GENERIC_HID_EPOUT                     (GENERIC_EPOUT_IDX)
#define GENERIC_HID_EPOUT_SIZE                (0x40) // 64 bytes
#define GENERIC_HID_EPIN                      (GENERIC_EPIN_IDX | EP_IN_DIRECTION)
#define GENERIC_HID_EPIN_SIZE                 (0x40) // 64 bytes

#define MOUSE_HID_EPIN                        (MOUSE_EPIN_IDX | EP_IN_DIRECTION)
#define MOUSE_HID_EPIN_SIZE                   (0x36) // 54 bytes

#define USB_COMPOSITE_HID_CONFIG_DESC_SIZE              (66)
#define USB_COMPOSITE_HID_CONFIG_NO_DIGITIZER_DESC_SIZE (41)
#define USB_COMPOSITE_HID_DESC_SIZE                     (9)
#define COMPOSITE_HID_DESCRIPTOR_TYPE                   (0x21)

// usb interface numbers used during enumeration - allows us to understand which interface is being targeted
#define GENERIC_INTERFACE_NUM    (0)
#define MOUSE_INTERFACE_NUM      (2)

// used to determine which interface we're wanting to send a report from (used in function call)
#define GENERIC     (0)
#define MOUSE       (2)

// used to define some other parameters of the usb config
#define RESERVED        (1 << 7)
#define BUS_POWERED     (0 << 6)
#define REMOTE_WAKEUP   (1 << 5)
/**
  * @}
  */


/** @defgroup USBD_CORE_Exported_TypesDefinitions
  * @{
  */
typedef enum
{
    USB_HID_IDLE = 0,
    USB_HID_BUSY,
}
USB_HID_StateTypeDef;
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
extern uint8_t USBD_COMPOSITE_HID_CfgDesc[USB_COMPOSITE_HID_CONFIG_DESC_SIZE];
extern USBD_ClassTypeDef  USBD_COMPOSITE_HID;
extern volatile uint8_t target_interface;
extern uint8_t NumInterfaces;
/**
  * @}
  */

/** @defgroup USB_CORE_Exported_Functions
  * @{
  */
//CUSTOMISED
uint8_t Send_USB_Report(uint8_t interface,USBD_HandleTypeDef  *pdev, uint8_t *report, uint16_t len);
/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif  /* __USB_COMPOSITEHID_H */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
