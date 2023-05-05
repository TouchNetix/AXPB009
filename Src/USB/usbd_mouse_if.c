/**
  ******************************************************************************
  * @file           : usbd_MOUSE_HID_if.c
  * @version        : v2.0_Cube
  * @brief          : USB Device Mouse HID interface file.
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
#include "usbd_core.h"
#include "usbd_conf.h"
#include "usbd_mouse.h"
#include "usbd_generic.h"
#include "usbd_press.h"
#include "usbd_mouse_if.h"
#include "usbd_generic_if.h"
#include "usbd_press_if.h"
#include <stdbool.h>

#include "Flash_Control.h"

/*============ Defines ============*/


/*============ Macros ============*/


/*============ Local Variables ============*/


/*============ Local Function Prototypes ============*/
static int8_t MOUSE_HID_Init_FS(void);
static int8_t MOUSE_HID_DeInit_FS(void);
static int8_t MOUSE_HID_OutEvent_FS(uint8_t* state);

/*============ Exported Variables ============*/
bool    boMouseReportToSend = 0;
uint8_t BridgeMode          = MODE_TBP_BASIC;
uint8_t byMouseReportLength = 0;
uint8_t usb_hid_mouse_report_in[USBD_MOUSE_HID_REPORT_IN_SIZE] = {0}; // buffer used to send report to host

/* USB Mouse HID Report Descriptor - Absolute mouse mode */
__ALIGN_BEGIN uint8_t mouse_abs_ReportDesc_FS[USBD_MOUSE_ABS_REPORT_DESC_SIZE] __ALIGN_END =
{
        0x05, 0x01,         // Usage Page (Generic Desktop),
        0x09, 0x02,         // Usage (Mouse),
        0xA1, 0x01,         // Collection (Application),
        0x09, 0x01,         //   Usage (Pointer),
        0xA1, 0x00,         //   Collection (Physical),
        0x05, 0x09,         //     Usage Page (Buttons),
        0x19, 0x01,         //     Usage Minimum (01),
        0x29, 0x03,         //     Usage Maximum (03),
        0x15, 0x00,         //     Logical Minimum (0),
        0x25, 0x01,         //     Logical Maximum (1),
        0x75, 0x01,         //     Report Size (1),
        0x95, 0x03,         //     Report Count (3),
        0x81, 0x02,         //     Input (Data, Variable, Absolute)
        0x75, 0x05,         //     Report Size (5),
        0x95, 0x01,         //     Report Count (1),
        0x81, 0x01,         //     Input (Constant),
        0x05, 0x01,         //     Usage Page (Generic Desktop),
        0x16, 0x00, 0x00,   //     Logical Minimum (0),     /* 35(low byte) 36(high byte) */
        0x26, 0xFF, 0x0F,   //     Logical Maximum (4095),  /* 41(low byte) 42(high byte) */
        0x36, 0x00, 0x00,   //     Physical Minimum (0),    /* 38(low byte) 39(high byte) */
        0x46, 0xFF, 0x0F,   //     Physical Maximum (4095), /* 44(low byte) 45(high byte) */
        0x09, 0x30,         //     Usage (X),
        0x09, 0x31,         //     Usage (Y),
        0x75, 0x10,         //     Report Size (16),
        0x95, 0x02,         //     Report Count (2),
        0x81, 0x02,         //     Input (Data, Variable, Absolute)
        0xC0,               //   End Collection,
        0xC0,               // End Collection
};

/* USB Mouse HID Report Descriptor - Parallel Digitizer mode */
__ALIGN_BEGIN uint8_t mouse_parallel_digitizer_ReportDesc_FS[USBD_MOUSE_PAR_DIGITIZER_DESC_SIZE] __ALIGN_END =
{
    // Top Level Collection - Touchscreen Digitizer */
    0x05u, 0x0Du,                         // USAGE_PAGE (Digitizers) */
    0x09u, 0x04u,                         // USAGE (Touch Screen) */
    0xA1u, 0x01u,                         // COLLECTION (Application) */
    0x85u, 0x01,                          // REPORT_ID (Touch) */
    /* 7 */

    //bring all the common parts between touch reports out here to reduce the descriptor size (else end up repeating same settings many times)
    0x35u, 0x00u,                         //     PHYSICAL MINIMUM (0) */
    0x15u, 0x00u,                         //     LOGICAL MINIMUM (0) */
    0x55u, 0x0Eu,                         //     Unit exponent (-2) */
    0x65u, 0x11u,                         //     UNIT (cm) */
    /* 13 */

    // First contact report */
    0x05u, 0x0Du,                         // USAGE_PAGE (Digitizers) */
    0x09u, 0x22u,                         // USAGE (Finger) */
    0xA1u, 0x02u,                         //     COLLECTION (Logical) */
    0x25u, 0x01u,                         //     LOGICAL_MAXIMUM (1) */
    0x95u, 0x01u,                         //     REPORT_COUNT (1) */
    0x75u, 0x01u,                         //     REPORT_SIZE (1) */

    0x09u, 0x42u,                         //     USAGE (Tip Switch) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bit = TipSwitch status (Touch / No Touch) */

    0x09u, 0x32u,                         //     USAGE (In Range) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bits = In range */

    0x09u, 0x47u,                         //     USAGE (Confidence) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bit = Confidence */

    0x25u, 0x1Fu,                         //     LOGICAL_MAXIMUM (31) */
    0x75u, 0x05u,                         //     REPORT SIZE (5) */
    0x09u, 0x51u,                         //     USAGE (CONTACT ID) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     5 Bits = Contact ID */

    0x05u, 0x01u,                         //     USAGE_PAGE (Generic Desktop) */
    0x75u, 0x10u,                         //     REPORT_SIZE (16) */
    /* 50 */

    /* NOTE - having to use 4 bytes for logical and physical values here:
    * if using 2 bytes to report a user requests the maximum possible value according to TH2 (0xFFFF) then we will end up with a negative value (as this is reported in 2s complement)
    * so need to stretch to being represented using 4 bytes to prevent this from happening (can't use 3 bytes, hence 4)
    * plus the physical value reported can go above 0xFFFF as we multiply the value read from aXiom by 5 to obtain the correct values
    * (TH2 increments by 0.5mm whilst in here we increment by 0.1mm)
    */
    0x27u, 0xFFu, 0x0Fu, 0x00u, 0x00u,    //     LOGICAL_MAXIMUM  X (default = 4095) */ /* 53(low byte), 56(high byte) */
    0x47u, 0x00u, 0x00u, 0x00u, 0x00u,    //     PHYSICAL_MAXIMUM X (default = 0) */    /* 58(low byte), 61(high byte) */
    0x09u, 0x30u,                         //     USAGE (X) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits = X Position */
    /* 64 */

    0x27u, 0xFFu, 0x0Fu, 0x00u, 0x00u,    //     LOGICAL_MAXIMUM  Y (default = 4095) */ /* 67(low), 70(high) */
    0x47u, 0x00u, 0x00u, 0x00u, 0x00u,    //     PHYSICAL_MAXIMUM Y (default = 0) */    /* 72(low byte), 75(high byte) */
    0x09u, 0x31u,                         //     USAGE (Y) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits = Y Position */
    /* 78 */

    /* Touch Pressure - used to convey z values */
    0x05u, 0x0Du,                         //     USAGE_PAGE (Digitizer) */
    0x09u, 0x30u,                         //     USAGE (Pressure) */
    0x26u, 0x00u, 0x04u,                  //     LOGICAL_MAXIMUM (1024) */
    0x45u, 0x00u,                         //     PHYSICAL_MAXIMUM (0) */
    0x95u, 0x01u,                         //     REPORT_COUNT (1) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits */

    0xC0u,                                // END_COLLECTION (Logical / 1st contact) */
    /* 93 */

    // Second Contact */
    0x05u, 0x0Du,                         // USAGE_PAGE (Digitizers) */
    0x09u, 0x22u,                         // USAGE (Finger) */
    0xA1u, 0x02u,                         //     COLLECTION (Logical) */
    0x25u, 0x01u,                         //     LOGICAL_MAXIMUM (1) */
    0x95u, 0x01u,                         //     REPORT_COUNT (1) */
    0x75u, 0x01u,                         //     REPORT_SIZE (1) */

    0x09u, 0x42u,                         //     USAGE (Tip Switch) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bit = TipSwitch status (Touch / No Touch) */

    0x09u, 0x32u,                         //     USAGE (In Range) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bits = In range */

    0x09u, 0x47u,                         //     USAGE (Confidence) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bit = Confidence */

    0x25u, 0x1Fu,                         //     LOGICAL_MAXIMUM (31) */
    0x75u, 0x05u,                         //     REPORT SIZE (5) */
    0x09u, 0x51u,                         //     USAGE (CONTACT ID) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     5 Bits = Contact ID */

    0x05u, 0x01u,                         //     USAGE_PAGE (Generic Desktop) */
    0x75u, 0x10u,                         //     REPORT_SIZE (16) */
    /* 129 */

    0x27u, 0xFFu, 0x0Fu, 0x00u, 0x00u,    //     LOGICAL_MAXIMUM  X (default = 4095) */
    0x47u, 0x00u, 0x00u, 0x00u, 0x00u,    //     PHYSICAL_MAXIMUM X (default = 0) */
    0x09u, 0x30u,                         //     USAGE (X) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits = X Position */
    /* 143 */

    0x27u, 0xFFu, 0x0Fu, 0x00u, 0x00u,    //     LOGICAL_MAXIMUM  Y (default = 4095) */
    0x47u, 0x00u, 0x00u, 0x00u, 0x00u,    //     PHYSICAL_MAXIMUM Y (default = 0) */
    0x09u, 0x31u,                         //     USAGE (Y) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits = Y Position */
    /* 157 */

    /* Touch Pressure - used to convey z values */
    0x05u, 0x0Du,                         //     USAGE_PAGE (Digitizer) */
    0x09u, 0x30u,                         //     USAGE (Pressure) */
    0x26u, 0x00u, 0x04u,                  //     LOGICAL_MAXIMUM (1024) */
    0x45u, 0x00u,                         //     PHYSICAL_MAXIMUM (0) */
    0x95u, 0x01u,                         //     REPORT_COUNT (1) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits */

    0xC0u,                                // END_COLLECTION (Logical / 2nd contact) */
    /* 172 */

    // Third Contact */
    0x05u, 0x0Du,                         // USAGE_PAGE (Digitizers) */
    0x09u, 0x22u,                         // USAGE (Finger) */
    0xA1u, 0x02u,                         //     COLLECTION (Logical) */
    0x25u, 0x01u,                         //     LOGICAL_MAXIMUM (1) */
    0x95u, 0x01u,                         //     REPORT_COUNT (1) */
    0x75u, 0x01u,                         //     REPORT_SIZE (1) */

    0x09u, 0x42u,                         //     USAGE (Tip Switch) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bit = TipSwitch status (Touch / No Touch) */

    0x09u, 0x32u,                         //     USAGE (In Range) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bits = In range */

    0x09u, 0x47u,                         //     USAGE (Confidence) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bit = Confidence */

    0x25u, 0x1Fu,                         //     LOGICAL_MAXIMUM (31) */
    0x75u, 0x05u,                         //     REPORT SIZE (5) */
    0x09u, 0x51u,                         //     USAGE (CONTACT ID) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     5 Bits = Contact ID */

    0x05u, 0x01u,                         //     USAGE_PAGE (Generic Desktop) */
    0x75u, 0x10u,                         //     REPORT_SIZE (16) */

    0x27u, 0xFFu, 0x0Fu, 0x00u, 0x00u,    //     LOGICAL_MAXIMUM  X (default = 4095) */ /* 163(low), 164(high) */
    0x47u, 0x00u, 0x00u, 0x00u, 0x00u,    //     PHYSICAL_MAXIMUM X (default = 0) */    /* 166(low), 167(high) */
    0x09u, 0x30u,                         //     USAGE (X) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits = X Position */

    0x27u, 0xFFu, 0x0Fu, 0x00u, 0x00u,    //     LOGICAL_MAXIMUM  Y (default = 4095) */ /* 173(low), 174(high) */
    0x47u, 0x00u, 0x00u, 0x00u, 0x00u,    //     PHYSICAL_MAXIMUM Y (default = 0) */    /* 176(low), 177(high) */
    0x09u, 0x31u,                         //     USAGE (Y) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits = Y Position */

    /* Touch Pressure - used to convey z values */
    0x05u, 0x0Du,                         //     USAGE_PAGE (Digitizer) */
    0x09u, 0x30u,                         //     USAGE (Pressure) */
    0x26u, 0x00u, 0x04u,                  //     LOGICAL_MAXIMUM (1024) */
    0x45u, 0x00u,                         //     PHYSICAL_MAXIMUM (0) */
    0x95u, 0x01u,                         //     REPORT_COUNT (1) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits */

    0xC0u,                                // END_COLLECTION (Logical / 3rd contact) */

    // Fourth Contact */
    0x05u, 0x0Du,                         // USAGE_PAGE (Digitizers) */
    0x09u, 0x22u,                         // USAGE (Finger) */
    0xA1u, 0x02u,                         //     COLLECTION (Logical) */
    0x25u, 0x01u,                         //     LOGICAL_MAXIMUM (1) */
    0x95u, 0x01u,                         //     REPORT_COUNT (1) */
    0x75u, 0x01u,                         //     REPORT_SIZE (1) */

    0x09u, 0x42u,                         //     USAGE (Tip Switch) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bit = TipSwitch status (Touch / No Touch) */

    0x09u, 0x32u,                         //     USAGE (In Range) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bits = In range */

    0x09u, 0x47u,                         //     USAGE (Confidence) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bit = Confidence */

    0x25u, 0x1Fu,                         //     LOGICAL_MAXIMUM (31) */
    0x75u, 0x05u,                         //     REPORT SIZE (5) */
    0x09u, 0x51u,                         //     USAGE (CONTACT ID) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     5 Bits = Contact ID */

    0x05u, 0x01u,                         //     USAGE_PAGE (Generic Desktop) */
    0x75u, 0x10u,                         //     REPORT_SIZE (16) */

    0x27u, 0xFFu, 0x0Fu, 0x00u, 0x00u,    //     LOGICAL_MAXIMUM  X (default = 4095) */
    0x47u, 0x00u, 0x00u, 0x00u, 0x00u,    //     PHYSICAL_MAXIMUM X (default = 0) */
    0x09u, 0x30u,                         //     USAGE (X) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits = X Position */

    0x27u, 0xFFu, 0x0Fu, 0x00u, 0x00u,    //     LOGICAL_MAXIMUM  Y (default = 4095) */
    0x47u, 0x00u, 0x00u, 0x00u, 0x00u,    //     PHYSICAL_MAXIMUM Y (default = 0) */
    0x09u, 0x31u,                         //     USAGE (Y) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits = Y Position */

    /* Touch Pressure - used to convey z values */
    0x05u, 0x0Du,                         //     USAGE_PAGE (Digitizer) */
    0x09u, 0x30u,                         //     USAGE (Pressure) */
    0x26u, 0x00u, 0x04u,                  //     LOGICAL_MAXIMUM (1024) */
    0x45u, 0x00u,                         //     PHYSICAL_MAXIMUM (0) */
    0x95u, 0x01u,                         //     REPORT_COUNT (1) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits */

    0xC0u,                                // END_COLLECTION (Logical / 4th contact) */

    // Fifth Contact */
    0x05u, 0x0Du,                         // USAGE_PAGE (Digitizers) */
    0x09u, 0x22u,                         // USAGE (Finger) */
    0xA1u, 0x02u,                         //     COLLECTION (Logical) */
    0x25u, 0x01u,                         //     LOGICAL_MAXIMUM (1) */
    0x95u, 0x01u,                         //     REPORT_COUNT (1) */
    0x75u, 0x01u,                         //     REPORT_SIZE (1) */

    0x09u, 0x42u,                         //     USAGE (Tip Switch) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bit = TipSwitch status (Touch / No Touch) */

    0x09u, 0x32u,                         //     USAGE (In Range) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bits = In range */

    0x09u, 0x47u,                         //     USAGE (Confidence) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     1 Bit = Confidence */

    0x25u, 0x1Fu,                         //     LOGICAL_MAXIMUM (31) */
    0x75u, 0x05u,                         //     REPORT SIZE (5) */
    0x09u, 0x51u,                         //     USAGE (CONTACT ID) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     5 Bits = Contact ID */

    0x05u, 0x01u,                         //     USAGE_PAGE (Generic Desktop) */
    0x75u, 0x10u,                         //     REPORT_SIZE (16) */

    0x27u, 0xFFu, 0x0Fu, 0x00u, 0x00u,     //     LOGICAL_MAXIMUM  X (default = 4095) */
    0x47u, 0x00u, 0x00u, 0x00u, 0x00u,     //     PHYSICAL_MAXIMUM X (default = 0) */
    0x09u, 0x30u,                         //     USAGE (X) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits = X Position */

    0x27u, 0xFFu, 0x0Fu, 0x00u, 0x00u,    //     LOGICAL_MAXIMUM  Y (default = 4095) */
    0x47u, 0x00u, 0x00u, 0x00u, 0x00u,    //     PHYSICAL_MAXIMUM Y (default = 0) */
    0x09u, 0x31u,                         //     USAGE (Y) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits = Y Position */

    /* Touch Pressure - used to convey z values */
    0x05u, 0x0Du,                         //     USAGE_PAGE (Digitizer) */
    0x09u, 0x30u,                         //     USAGE (Pressure) */
    0x26u, 0x00u, 0x04u,                  //     LOGICAL_MAXIMUM (1024) */
    0x45u, 0x00u,                         //     PHYSICAL_MAXIMUM (0) */
    0x95u, 0x01u,                         //     REPORT_COUNT (1) */
    0x81u, 0x02u,                         //     INPUT (Data,Var,Abs)     16 Bits */

    0xC0u,                                // END_COLLECTION (Logical / 5th contact) */

    // Timestamp */
    0x05, 0x0d,                         //    USAGE_PAGE (Digitizers)
    0x55, 0x0C,                         //    UNIT_EXPONENT (-4)   !!!MUST be -4 i.e. 100us for Win8
    0x66, 0x01, 0x10,                   //    UNIT (Seconds)
    0x47, 0xff, 0xff, 0x00, 0x00,       //    PHYSICAL_MAXIMUM (65535)
    0x27, 0xff, 0xff, 0x00, 0x00,       //    LOGICAL_MAXIMUM (65535)
    0x75, 0x10,                         //    REPORT_SIZE (16)
    0x95, 0x01,                         //    REPORT_COUNT (1)
    0x09, 0x56,                         //    USAGE (Scan Time)
    0x81, 0x02,                         //    INPUT (Data,Var,Abs)


    // Contact Count */
    0x05u, 0x0Du,                         // USAGE_PAGE (Digitizers) */
    0x15u, 0x00u,                         // LOGICAL_MINIMUM (0) */
    0x25u, 0x1Fu,                         // LOGICAL_MAXIMUM (31) */
    0x75u, 0x05u,                         // REPORT SIZE (5) */
    0x09u, 0x54u,                         // USAGE (Contact Count) */
    0x95u, 0x01u,                         // REPORT COUNT (1) */
    0x81u, 0x02u,                         // INPUT (Data,Var,Abs)     5 Bits = Contact count */

    0x75u, 0x03u,                         // REPORT_SIZE (3) */
    0x25u, 0x01u,                         // LOGICAL_MAXIMUM (1) */
    0x95u, 0x01u,                         // REPORT COUNT (1) */
    0x81u, 0x03u,                         // Input (Cnst,Var,Abs)     3 Bits = Padding */

    /* Feature report notification */
    0x85u, 0x02u,      /*   Report ID (Feature) */
    0x75u, 0x08u,                         /*   REPORT SIZE (8) */
    0x09u, 0x55u,                         /*   USAGE (Maximum Count) */
    0x25u, 0x0Au,                         /*   Logical maximum (10) */
    0xB1u, 0x02u,                         /*   Feature (Data, Var, Abs) */

    0xC0u,                               // END_COLLECTION */
};

/*============ Typedefs ============*/
USBD_MOUSE_HID_ItfTypeDef USBD_MouseHID_fops_FS =
{
  NULL, // to be populated by MatchReportDescriptorSizeToMode() during initialisation
  MOUSE_HID_Init_FS,
  MOUSE_HID_DeInit_FS,
  MOUSE_HID_OutEvent_FS
};

/*============ Functions ============*/
/**
  * @brief  Initializes the MOUSE HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t MOUSE_HID_Init_FS(void)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  DeInitializes the MOUSE HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t MOUSE_HID_DeInit_FS(void)
{
  /* USER CODE BEGIN 5 */
  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Manage the MOUSE HID class events
  * @param  event_idx: Event index
  * @param  state: Event state
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t MOUSE_HID_OutEvent_FS(uint8_t* state)
{
    target_interface = MOUSE_INTERFACE_NUM;
    return (USBD_OK);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

