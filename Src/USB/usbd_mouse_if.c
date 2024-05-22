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

#include "Digitizer.h"
#include "Flash_Control.h"

/*============ Defines ============*/


/*============ Macros ============*/


/*============ Local Variables ============*/


/*============ Local Function Prototypes ============*/
static int8_t MOUSE_HID_Init_FS(void);
static int8_t MOUSE_HID_DeInit_FS(void);
static int8_t MOUSE_HID_OutEvent_FS(uint8_t* state);
static int8_t MOUSE_HID_SetFeature_FS(uint8_t event_idx, uint8_t* buffer);
static int8_t MOUSE_HID_GetFeature_FS(uint8_t event_idx, uint8_t* buffer, uint16_t* length);
static uint16_t MOUSE_HID_FeatureReportLength_FS(uint8_t event_idx);

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

/* USB Mouse HID Report Descriptor - Relative mouse mode */
__ALIGN_BEGIN uint8_t mouse_rel_ReportDesc_FS[USBD_MOUSE_REL_REPORT_DESC_SIZE] __ALIGN_END =
{
/* ======= TOUCHPAD ======= */
    0x05, 0x0D,                         // USAGE_PAGE (Digitizers)
    0x09, 0x05,                         // USAGE (Touch Pad)
    0xA1, 0x01,                         // COLLECTION (Application)
    0x85, REPORT_TOUCHPAD,              //   REPORT_ID (Touch pad)

    // FIRST CONTACT
    0x09, 0x22,                         //   USAGE (Finger)
    0xA1, 0x02,                         //   COLLECTION (Logical)
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)
    0x09, 0x47,                         //     USAGE (Confidence)
    0x09, 0x42,                         //     USAGE (Tip switch)
    0x95, 0x02,                         //     REPORT_COUNT (2)
    0x75, 0x01,                         //     REPORT_SIZE (1)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x95, 0x01,                         //     REPORT_COUNT (1)
    0x75, 0x02,                         //     REPORT_SIZE (2)
    0x25, 0x02,                         //     LOGICAL_MAXIMUM (2)
    0x09, 0x51,                         //     USAGE (Contact Number Identifier)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x75, 0x01,                         //     REPORT_SIZE (1)
    0x95, 0x04,                         //     REPORT_COUNT (4)
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desk..
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x0F,                   //     LOGICAL_MAXIMUM (4095)
    0x75, 0x10,                         //     REPORT_SIZE (16)
    0x55, 0x0E,                         //     UNIT_EXPONENT (-2)
    0x65, 0x13,                         //     UNIT(Inch,EngLinear)
    0x09, 0x30,                         //     USAGE (X)
    0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)
    0x46, 0x90, 0x01,                   //     PHYSICAL_MAXIMUM (400)
    0x95, 0x01,                         //     REPORT_COUNT (1)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x46, 0x13, 0x01,                   //     PHYSICAL_MAXIMUM (275)
    0x09, 0x31,                         //     USAGE (Y)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0xC0,                               //   END_COLLECTION

    // SCAN TIME
    0x55, 0x0C,                         //   UNIT_EXPONENT (-4)
    0x66, 0x01, 0x10,                   //   UNIT (Seconds)
    0x47, 0xFF, 0xFF, 0x00, 0x00,       //   PHYSICAL_MAXIMUM (65535)
    0x27, 0xFF, 0xFF, 0x00, 0x00,       //   LOGICAL_MAXIMUM (65535)
    0x75, 0x10,                         //   REPORT_SIZE (16)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x05, 0x0D,                         //   USAGE_PAGE (Digitizers)
    0x09, 0x56,                         //   USAGE (Scan Time)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)

    // CONTACT COUNT
    0x09, 0x54,                         //   USAGE (Contact count)
    0x25, 0x7F,                         //   LOGICAL_MAXIMUM (127)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x75, 0x08,                         //   REPORT_SIZE (8)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)

    // BUTTONS
    0x05, 0x09,                         //   USAGE_PAGE (Button)
    0x09, 0x01,                         //   USAGE_(Button 1)
    0x09, 0x02,                         //   USAGE_(Button 2)
    0x09, 0x03,                         //   USAGE_(Button 3)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x95, 0x03,                         //   REPORT_COUNT (3)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x95, 0x05,                         //   REPORT_COUNT (5)
    0x81, 0x03,                         //   INPUT (Cnst,Var,Abs)

    // MAXIMUM CONTACTS
    0x05, 0x0d,                         //   USAGE_PAGE (Digitizer)
    0x85, REPORT_FEATURE_MAXCT,         //   REPORT_ID (Feature MAX COUNT)
    0x09, 0x55,                         //   USAGE (Contact Count Maximum)
    0x09, 0x59,                         //   USAGE (Pad TYpe)
    0x75, 0x08,                         //   REPORT_SIZE (8)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x25, 0x05,                         //   LOGICAL_MAXIMUM (5)
    0xB1, 0x02,                         //   FEATURE (Data,Var,Abs)

    // CERTIFICATION BLOB
    0x06, 0x00, 0xFF,                   //   USAGE_PAGE (Vendor Defined)
    0x85, REPORT_FEATURE_PTPHQABLOB,    //   REPORT_ID (PTPHQA)
    0x09, 0xC5,                         //   USAGE (Vendor Usage 0xC5)
    0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,                   //   LOGICAL_MAXIMUM (0xff)
    0x75, 0x08,                         //   REPORT_SIZE (8)
    0x96, 0x00, 0x01,                   //   REPORT_COUNT (0x100 (256))
    0xB1, 0x02,                         //   FEATURE (Data,Var,Abs)
    0xC0,                               // END_COLLECTION

    // INPUT MODE - COLLECTION SELECTION
    0x05, 0x0D,                         // USAGE_PAGE (Digitizer)
    0x09, 0x0E,                         // USAGE (Configuration)
    0xA1, 0x01,                         // COLLECTION (Application)
    0x85, REPORT_FEATURE_INPUTMODE,     //   REPORT_ID (Feature INPUT MODE)
    0x09, 0x22,                         //   USAGE (Finger)
    0xA1, 0x02,                         //   COLLECTION (logical)
    0x09, 0x52,                         //     USAGE (Input Mode)
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
    0x25, 0x0A,                         //     LOGICAL_MAXIMUM (10)
    0x75, 0x08,                         //     REPORT_SIZE (8)
    0x95, 0x01,                         //     REPORT_COUNT (1)
    0xB1, 0x02,                         //     FEATURE (Data,Var,Abs
    0xC0,                               //   END_COLLECTION

    // INPUT MODE - INPUT TYPE
    0x09, 0x22,                         //   USAGE (Finger)
    0xA1, 0x00,                         //   COLLECTION (physical)
    0x85, REPORT_FEATURE_FUNCTIONSWITCH,//     REPORT_ID (Feature FUNCTION SWITCH)
    0x09, 0x57,                         //     USAGE(Surface switch)
    0x09, 0x58,                         //     USAGE(Button switch)
    0x75, 0x01,                         //     REPORT_SIZE (1)
    0x95, 0x02,                         //     REPORT_COUNT (2)
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)
    0xb1, 0x02,                         //     FEATURE (Data,Var,Abs)
    0x95, 0x06,                         //     REPORT_COUNT (6)
    0xB1, 0x03,                         //     FEATURE (Cnst,Var,Abs)
    0xC0,                               //   END_COLLECTION
    0xC0,                               // END_COLLECTION

/* ======= MOUSE ======= */
    0x05, 0x01,                         // Usage Page (Generic Desktop),
    0x09, 0x02,                         // Usage (Mouse),
    0xA1, 0x01,                         // Collection (Application),
    0x85, REPORT_REL_MOUSE,             //   Report ID (Mouse)
    0x09, 0x01,                         //   Usage (Pointer),
    0xA1, 0x00,                         //   Collection (Physical),

    // BUTTONS
    0x05, 0x09,                         //     Usage Page (Button),
    0x19, 0x01,                         //     Usage Minimum (01),                  3 buttons for left-click, right-click and middle-click.
    0x29, 0x03,                         //     Usage Maximum (03),
    0x15, 0x00,                         //     Logical Minimum (0),                 Each button can be in state 0 or 1.
    0x25, 0x01,                         //     Logical Maximum (1),
    0x75, 0x01,                         //     Report Size (1),                     Each button represented by a single bit,
    0x95, 0x03,                         //     Report Count (3),                    which is repeated 3 times (one bit for each button).
    0x81, 0x02,                         //     Input (Data, Variable, Absolute)
    0x75, 0x05,                         //     Report Size (5),                     5 bits of padding to reach a full byte,
    0x95, 0x01,                         //     Report Count (1),                    which is repeated once.
    0x81, 0x01,                         //     Input (Constant),                    Mark the padding as constant (i.e. ignore them).
    0x05, 0x01,                         //     Usage Page (Generic Desktop),

    // XY MOVEMENT
    0x09, 0x30,                         //     Usage (X),
    0x09, 0x31,                         //     Usage (Y),
    0x15, 0x81,                         //     Logical Minimum (-127),              X and Y can send values between -127 and 127.
    0x25, 0x7F,                         //     Logical Maximum (127),
    0x75, 0x08,                         //     Report Size (8),                     Values sent is represented in a byte,
    0x95, 0x02,                         //     Report Count (2),                    1 byte each for X and Y.
    0x81, 0x06,                         //     Input (Data, Variable, Relative)     The values are relative to the previous report (i.e. host remembers cursor position)
    0xC0,                               //   End Collection,
    0xC0,                               // End Collection
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

// A generic certification 'blob' from MicroSoft, for development purposes
__ALIGN_BEGIN uint8_t hid_THQL_digital_blob[USBHID_THQL_BLOB_SIZE] __ALIGN_END =
{
    0xfc, 0x28, 0xfe, 0x84, 0x40, 0xcb, 0x9a, 0x87, 0x0d, 0xbe, 0x57, 0x3c, 0xb6, 0x70, 0x09, 0x88,
    0x07, 0x97, 0x2d, 0x2b, 0xe3, 0x38, 0x34, 0xb6, 0x6c, 0xed, 0xb0, 0xf7, 0xe5, 0x9c, 0xf6, 0xc2,
    0x2e, 0x84, 0x1b, 0xe8, 0xb4, 0x51, 0x78, 0x43, 0x1f, 0x28, 0x4b, 0x7c, 0x2d, 0x53, 0xaf, 0xfc,
    0x47, 0x70, 0x1b, 0x59, 0x6f, 0x74, 0x43, 0xc4, 0xf3, 0x47, 0x18, 0x53, 0x1a, 0xa2, 0xa1, 0x71,
    0xc7, 0x95, 0x0e, 0x31, 0x55, 0x21, 0xd3, 0xb5, 0x1e, 0xe9, 0x0c, 0xba, 0xec, 0xb8, 0x89, 0x19,
    0x3e, 0xb3, 0xaf, 0x75, 0x81, 0x9d, 0x53, 0xb9, 0x41, 0x57, 0xf4, 0x6d, 0x39, 0x25, 0x29, 0x7c,
    0x87, 0xd9, 0xb4, 0x98, 0x45, 0x7d, 0xa7, 0x26, 0x9c, 0x65, 0x3b, 0x85, 0x68, 0x89, 0xd7, 0x3b,
    0xbd, 0xff, 0x14, 0x67, 0xf2, 0x2b, 0xf0, 0x2a, 0x41, 0x54, 0xf0, 0xfd, 0x2c, 0x66, 0x7c, 0xf8,
    0xc0, 0x8f, 0x33, 0x13, 0x03, 0xf1, 0xd3, 0xc1, 0x0b, 0x89, 0xd9, 0x1b, 0x62, 0xcd, 0x51, 0xb7,
    0x80, 0xb8, 0xaf, 0x3a, 0x10, 0xc1, 0x8a, 0x5b, 0xe8, 0x8a, 0x56, 0xf0, 0x8c, 0xaa, 0xfa, 0x35,
    0xe9, 0x42, 0xc4, 0xd8, 0x55, 0xc3, 0x38, 0xcc, 0x2b, 0x53, 0x5c, 0x69, 0x52, 0xd5, 0xc8, 0x73,
    0x02, 0x38, 0x7c, 0x73, 0xb6, 0x41, 0xe7, 0xff, 0x05, 0xd8, 0x2b, 0x79, 0x9a, 0xe2, 0x34, 0x60,
    0x8f, 0xa3, 0x32, 0x1f, 0x09, 0x78, 0x62, 0xbc, 0x80, 0xe3, 0x0f, 0xbd, 0x65, 0x20, 0x08, 0x13,
    0xc1, 0xe2, 0xee, 0x53, 0x2d, 0x86, 0x7e, 0xa7, 0x5a, 0xc5, 0xd3, 0x7d, 0x98, 0xbe, 0x31, 0x48,
    0x1f, 0xfb, 0xda, 0xaf, 0xa2, 0xa8, 0x6a, 0x89, 0xd6, 0xbf, 0xf2, 0xd3, 0x32, 0x2a, 0x9a, 0xe4,
    0xcf, 0x17, 0xb7, 0xb8, 0xf4, 0xe1, 0x33, 0x08, 0x24, 0x8b, 0xc4, 0x43, 0xa5, 0xe5, 0x24, 0xc2,
};

/*============ Typedefs ============*/
USBD_MOUSE_HID_ItfTypeDef USBD_MouseHID_fops_FS =
{
  NULL, // to be populated by MatchReportDescriptorSizeToMode() during initialisation
  MOUSE_HID_Init_FS,
  MOUSE_HID_DeInit_FS,
  MOUSE_HID_OutEvent_FS,
  MOUSE_HID_SetFeature_FS,
  MOUSE_HID_GetFeature_FS,
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

/**
 * @brief  MOUSE_HID_SetFeature_FS
 *         Manage the MOUSE HID SetFeature request.
 *         Host -> Device
 * @param  event_idx: Report Number
 * @param  buffer: Received Data
 * @retval USBD_OK
 */
static int8_t MOUSE_HID_SetFeature_FS(uint8_t event_idx, uint8_t* buffer)
{
    switch(event_idx)
    {
        case REPORT_FEATURE_INPUTMODE: /* Host dictates if bridge should report mouse or touch-pad reports */
            SetTouchPadMode(buffer[0]);
            break;

        case REPORT_FEATURE_FUNCTIONSWITCH:
            SetTouchPadInputSelection(buffer[0]);
            break;

        default: /* Report does not exist */
            return (USBD_FAIL);
            break;
    }

    return (USBD_OK);
}

/**
 * @brief  MOUSE_HID_GetFeature_FS
 *         Manage the MOUSE HID GetFeature request.
 *         Device -> Host
 * @param  event_idx: Requested Report Number
 * @param  buffer: Data to transmit including ReportID
 * @param  length: Length of the buffer
 * @retval length: Number of bytes to send
 * @retval USBD_OK
 */
static int8_t MOUSE_HID_GetFeature_FS(uint8_t event_idx, uint8_t* pBuffer, uint16_t* length)
{
    // clear transmission data array
    memset(pBuffer, 0x00, *length);

    switch(event_idx)
    {
        case REPORT_FEATURE_MAXCT:
            pBuffer[0] = 0x05;
            *length = sizeof(uint8_t);
            break;

        case REPORT_FEATURE_PTPHQABLOB:
            *length = sizeof(hid_THQL_digital_blob);
            memcpy(pBuffer, hid_THQL_digital_blob, sizeof(hid_THQL_digital_blob));
            break;

        default: /* Report does not exist */
            return (USBD_FAIL);
            break;
    }

    return (USBD_OK);
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

