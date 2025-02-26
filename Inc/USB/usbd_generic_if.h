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
#ifndef __USBD_GENERIC_HID_IF_H__
#define __USBD_GENERIC_HID_IF_H__

#ifdef __cplusplus
 extern "C" {
#endif

/*============ Includes ============*/
#include "usbd_generic.h"
#include <stdbool.h>
#include "Comms.h"

/*============ Exported Defines ============*/
#define REPORT_ID_CONTROL           (0x01U)
#define REPORT_ID_DIGITIZER         (0x02U)
#define REPORT_ID_DIGI_MAX_COUNT    (0x03U)
#define REPORT_ID_MOUSE             (0x04U)  

/*============ Exported Types ============*/
extern USBD_GENERIC_HID_ItfTypeDef USBD_GenericHID_fops_FS; /** GENERIC_HID Interface callback. */

/*============ Exported Macros ============*/

/*============ Exported Types ============*/

/*============ Exported Variables ============*/
extern volatile bool boCommandWaitingToDecode;
extern bool     boGenericReportToSend;
extern uint8_t *pTBPCommandReportGeneric;
//uint8_t generic_proxy_array_report_in[2][SPI_CMD_BYTES + SPI_PADDING_BYTES + USBD_GENERIC_HID_REPORT_IN_SIZE];

#ifdef __cplusplus
}
#endif

#endif /* __USBD_GENERIC_HID_IF_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
