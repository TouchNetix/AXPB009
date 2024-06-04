/**
  ******************************************************************************
  * @file           : usbd_MOUSE_HID_if.h
  * @version        : v2.0_Cube
  * @brief          : Header for usbd_MOUSE_HID_if.c file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_MOUSE_HID_IF_H__
#define __USBD_MOUSE_HID_IF_H__

#ifdef __cplusplus
 extern "C" {
#endif

/*============ Includes ============*/
#include "usbd_mouse.h"
#include "usbd_conf.h"
#include <stdbool.h>

/*============ Exported Defines ============*/
// HID report IDs
#define REPORT_TOUCHPAD                 (0x01)
#define REPORT_FEATURE_MAXCT            (0x02)
#define REPORT_FEATURE_PTPHQABLOB       (0x03)
#define REPORT_FEATURE_INPUTMODE        (0x04)  // Host dictate whether to report as a mouse or touch-pad
#define REPORT_FEATURE_FUNCTIONSWITCH   (0x05)
#define REPORT_REL_MOUSE                (0x06)

#define USBHID_THQL_BLOB_SIZE           (256)

#define DIGITIZER_MAX_CONTACT_CT        (0x05U)
#define PRECISION_TPAD_MAX_CONTACT_CT   (0x05U)

/*============ Exported Types ============*/

/*============ Exported Macros ============*/

/*============ Exported Variables ============*/
extern bool    boMouseReportToSend;
extern uint8_t BridgeMode;
extern uint8_t byMouseReportLength;
extern uint8_t usb_hid_mouse_report_in[USBD_MOUSE_HID_REPORT_IN_SIZE];

/** MOUSEHID Interface callback. */
extern USBD_MOUSE_HID_ItfTypeDef USBD_MouseHID_fops_FS;
extern uint8_t mouse_abs_ReportDesc_FS                  [USBD_MOUSE_ABS_REPORT_DESC_SIZE];
extern uint8_t mouse_rel_ReportDesc_FS                  [USBD_TPAD_REPORT_DESC_SIZE];
extern uint8_t mouse_parallel_digitizer_ReportDesc_FS   [USBD_MOUSE_PAR_DIGITIZER_DESC_SIZE];

/*============ Exported Functions ============*/


#ifdef __cplusplus
}
#endif

#endif /* __USBD_MOUSE_HID_IF_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
