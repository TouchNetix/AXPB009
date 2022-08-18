/**
  ******************************************************************************
  * @file           : usbd_GENERIC_HID_if.h
  * @version        : v2.0_Cube
  * @brief          : Header for usbd_GENERIC_HID_if.c file.
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
#ifndef __USBD_PRESS_HID_IF_H__
#define __USBD_PRESS_HID_IF_H__

#ifdef __cplusplus
 extern "C" {
#endif

 /*============ Includes ============*/
#include "usbd_press.h"
#include <stdbool.h>

 /*============ Exported Defines ============*/

 /*============ Exported Types ============*/

/*============ Exported Macros ============*/

/*============ Exported Variables ============*//*============ Exported Variables ============*/
extern bool     boPressReportToSend;
extern uint8_t  usb_hid_press_report_in[USBD_PRESS_HID_REPORT_IN_SIZE]; // buffer used to send report to host
extern USBD_PRESS_HID_ItfTypeDef USBD_PressHID_fops_FS; /** PRESSHID Interface callback. */
extern uint8_t *pTBPCommandReportPress;

/*============ Exported Functions ============*/


#ifdef __cplusplus
}
#endif

#endif /* __USBD_PRESS_HID_IF_H__ */
