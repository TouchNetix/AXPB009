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

#define DIGITIZER_CONTACT_REPORT                                                                                                                                                      \
                                                                                                                                                                                      \
    /* First contact report */                                                                                                                                                        \
    0x05u, 0x0Du,                         /* USAGE_PAGE (Digitizers) */                                                                                                               \
    0x09u, 0x22u,                         /* USAGE (Finger) */                                                                                                                        \
    0xA1u, 0x02u,                         /*     COLLECTION (Logical) */                                                                                                              \
    0x25u, 0x01u,                         /*     LOGICAL_MAXIMUM (1) */                                                                                                               \
    0x95u, 0x01u,                         /*     REPORT_COUNT (1) */                                                                                                                  \
    0x75u, 0x01u,                         /*     REPORT_SIZE (1) */                                                                                                                   \
                                                                                                                                                                                      \
    0x09u, 0x42u,                         /*     USAGE (Tip Switch) */                                                                                                                \
    0x81u, 0x02u,                         /*     INPUT (Data,Var,Abs)     1 Bit = TipSwitch status (Touch / No Touch) */                                                              \
                                                                                                                                                                                      \
    0x09u, 0x32u,                         /*     USAGE (In Range) */                                                                                                                  \
    0x81u, 0x02u,                         /*     INPUT (Data,Var,Abs)     1 Bits = In range */                                                                                        \
                                                                                                                                                                                      \
    0x09u, 0x47u,                         /*     USAGE (Confidence) */                                                                                                                \
    0x81u, 0x02u,                         /*     INPUT (Data,Var,Abs)     1 Bit = Confidence */                                                                                       \
                                                                                                                                                                                      \
    0x25u, 0x1Fu,                         /*     LOGICAL_MAXIMUM (31) */                                                                                                              \
    0x75u, 0x05u,                         /*     REPORT SIZE (5) */                                                                                                                   \
    0x09u, 0x51u,                         /*     USAGE (CONTACT ID) */                                                                                                                \
    0x81u, 0x02u,                         /*     INPUT (Data,Var,Abs)     5 Bits = Contact ID */                                                                                      \
                                                                                                                                                                                      \
    0x05u, 0x01u,                         /*     USAGE_PAGE (Generic Desktop) */                                                                                                      \
    0x75u, 0x10u,                         /*     REPORT_SIZE (16) */                                                                                                                  \
    /* 36 */                                                                                                                                                                          \
                                                                                                                                                                                      \
    /* NOTE - having to use 4 bytes for logical and physical values here:                                                                                                             \
    * if using 2 bytes to report a user requests the maximum possible value according to TH2 (0xFFFF) then we will end up with a negative value (as this is reported in 2s complement)\
    * so need to stretch to being represented using 4 bytes to prevent this from happening (can't use 3 bytes, hence 4)                                                               \
    * plus the physical value reported can go above 0xFFFF as we multiply the value read from aXiom by 5 to obtain the correct values                                                 \
    * (TH2 increments by 0.5mm whilst in here we increment by 0.1mm)                                                                                                                  \
    */                                                                                                                                                                                \
    0x27u, 0xFFu, 0x0Fu, 0x00u, 0x00u,    /*     LOGICAL_MAXIMUM  X (default = 4095) */ /* 53(low byte), 56(high byte) */                                                             \
    0x47u, 0x00u, 0x00u, 0x00u, 0x00u,    /*     PHYSICAL_MAXIMUM X (default = 0) */    /* 58(low byte), 61(high byte) */                                                             \
    0x09u, 0x30u,                         /*     USAGE (X) */                                                                                                                         \
    0x81u, 0x02u,                         /*     INPUT (Data,Var,Abs)     16 Bits = X Position */                                                                                     \
    /* 50 */                                                                                                                                                                          \
                                                                                                                                                                                      \
    0x27u, 0xFFu, 0x0Fu, 0x00u, 0x00u,    /*     LOGICAL_MAXIMUM  Y (default = 4095) */ /* 67(low), 70(high) */                                                                       \
    0x47u, 0x00u, 0x00u, 0x00u, 0x00u,    /*     PHYSICAL_MAXIMUM Y (default = 0) */    /* 72(low byte), 75(high byte) */                                                             \
    0x09u, 0x31u,                         /*     USAGE (Y) */                                                                                                                         \
    0x81u, 0x02u,                         /*     INPUT (Data,Var,Abs)     16 Bits = Y Position */                                                                                     \
    /* 64 */                                                                                                                                                                          \
                                                                                                                                                                                      \
    /* Touch Pressure - used to convey z values */                                                                                                                                    \
    0x05u, 0x0Du,                         /*     USAGE_PAGE (Digitizer) */                                                                                                            \
    0x09u, 0x30u,                         /*     USAGE (Pressure) */                                                                                                                  \
    0x26u, 0x00u, 0x04u,                  /*     LOGICAL_MAXIMUM (1024) */                                                                                                            \
    0x45u, 0x00u,                         /*     PHYSICAL_MAXIMUM (0) */                                                                                                              \
    0x95u, 0x01u,                         /*     REPORT_COUNT (1) */                                                                                                                  \
    0x81u, 0x02u,                         /*     INPUT (Data,Var,Abs)     16 Bits */                                                                                                  \
                                                                                                                                                                                      \
    0xC0u,                                /* END_COLLECTION (Logical / 1st contact) */                                                                                                \
    /* 78 */


/*============ Macros ============*/


/*============ Local Variables ============*/


/*============ Exported Types ============*/


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
#if COMBINED_REPORT
    0x85u, REPORT_ID_CONTROL, // Report ID
    /* 9 bytes */
#else
    /* 7 bytes */
#endif

    // IN report
    0x09, 0x01,         // 08|1   , Usage      (vendor defined)
    0x15, 0x00,         // 14|1   , Logical Minimum(0 for signed byte?)
    0x26, 0xFF, 0x00,   // 24|1   , Logical Maximum(255 for signed byte?)
    0x75, 0x08,         // 74|1   , Report Size(8) = field size in bits = 1 byte
    0x95, (USBD_GENERIC_HID_REPORT_IN_SIZE - 1),		// 94|2 ReportCount(size) = repeat count of previous item, 64 byte IN report
    0x81, 0x02,         // 80|1   , IN report (Data,Variable, Absolute)
    /* 22 bytes */

    // OUT report
    0x09, 0x02,         // 08|1   , Usage      (vendor defined)
    0x15, 0x00,         // 14|1   , Logical Minimum(0 for signed byte?)
    0x26, 0xFF, 0x00,   // 24|1   , Logical Maximum(255 for signed byte?)
    0x75, 0x08,         // 74|1   , Report Size(8) = field size in bits = 1 byte
    0x95, (USBD_GENERIC_HID_REPORT_OUT_SIZE - 1),   // 94|2 ReportCount(size) = repeat count of previous item, 64 byte OUT report
    0x91, 0x02,         // 90|1   , OUT report (Data,Variable, Absolute)
    /* 35 bytes */

    // Feature report
    0x09, 0x03,         // 08|1   , Usage      (vendor defined)
    0x15, 0x00,         // 14|1   , LogicalMinimum(0 for signed byte)
    0x26, 0xFF, 0x00,   // 24|1   , Logical Maximum(255 for signed byte)
    0x75, 0x08,         // 74|1   , Report Size(8) =field size in bits = 1 byte
    0x95, USBD_GENERIC_HID_FEATURE_SIZE,         // 94|1   , ReportCount in byte
    0xB1, 0x02,         // B0|1   , Feature report
    0xC0,               // C0|0   , End Collection
    /* 49 bytes */

#if COMBINED_REPORT

    // Top Level Collection - Touchscreen Digitizer */
    0x05u, 0x0Du,                         // USAGE_PAGE (Digitizers) */
    0x09u, 0x04u,                         // USAGE (Touch Screen) */
    0xA1u, 0x01u,                         // COLLECTION (Application) */
    0x85u, REPORT_ID_DIGITIZER,           // REPORT_ID (Touch) */
    /* 57 */

    //bring all the common parts between touch reports out here to reduce the descriptor size (else end up repeating same settings many times)
    0x35u, 0x00u,                         //     PHYSICAL MINIMUM (0) */
    0x15u, 0x00u,                         //     LOGICAL MINIMUM (0) */
    0x55u, 0x0Eu,                         //     Unit exponent (-2) */
    0x65u, 0x11u,                         //     UNIT (cm) */
    /* 65 */

    // First contact report */
    DIGITIZER_CONTACT_REPORT
    /* 93 */

    // Second Contact */
    DIGITIZER_CONTACT_REPORT
    /* 172 */

    // Third Contact */
    DIGITIZER_CONTACT_REPORT

    // Fourth Contact */
    DIGITIZER_CONTACT_REPORT

    // Fifth Contact */
    DIGITIZER_CONTACT_REPORT

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
    0x85u, REPORT_ID_DIGI_MAX_COUNT,      /*   Report ID (Feature) */
    0x75u, 0x08u,                         /*   REPORT SIZE (8) */
    0x09u, 0x55u,                         /*   USAGE (Maximum Count) */
    0x25u, 0x0Au,                         /*   Logical maximum (10) */
    0xB1u, 0x02u,                         /*   Feature (Data, Var, Abs) */

    0xC0u,                               // END_COLLECTION */

    
    0x05, 0x01,         // Usage Page (Generic Desktop),
    0x09, 0x02,         // Usage (Mouse),
    0xA1, 0x01,         // Collection (Application),
    0x85u, REPORT_ID_MOUSE,      /*   Report ID (Feature) */
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
#endif
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

