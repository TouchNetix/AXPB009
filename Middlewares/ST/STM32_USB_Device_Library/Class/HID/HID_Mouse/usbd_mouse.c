/**
  ******************************************************************************
  * @file    usbd_mouse.c
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief   This file provides the MOUSE_HID core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                MOUSE_HID Class  Description
  *          ===================================================================
  *           This module manages the MOUSE_HID class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (MOUSE_HID) Version 1.11 Jun 27, 2001".
  *           This driver implements the following aspects of the specification:
  *             - The Boot Interface Subclass
  *             - Usage Page : Generic Desktop
  *             - Usage : Vendor
  *             - Collection : Application
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
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

/* Includes ------------------------------------------------------------------*/
#include "usbd_mouse.h"
#include "usbd_mouse_if.h"
#include "usb_device.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "usbd_composite.h"
#include "Digitizer.h"
#include "Mode_Control.h"
#include "Flash_Control.h"

/*============ Defines ============*/
#define HID_INPUT       (1U)
#define HID_OUTPUT      (2U)
#define HID_FEATURE     (3U)

/*============ Local Function Prototypes ============*/

USBD_ClassTypeDef  USBD_MOUSE_HID =
{
    USBD_MOUSE_HID_Init,
    USBD_MOUSE_HID_DeInit,
    USBD_MOUSE_HID_Setup,
    NULL, /*EP0_TxSent*/
    USBD_MOUSE_HID_EP0_RxReady, /*EP0_RxReady*/ /* STATUS STAGE IN */
    USBD_MOUSE_HID_DataIn, /*DataIn*/
    USBD_MOUSE_HID_DataOut,
    NULL, /*SOF */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    USBD_MOUSE_HID_GetDeviceQualifierDesc,
};

/* USB MOUSE_HID device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_MOUSE_HID_Desc[USB_MOUSE_HID_DESC_SIZ] __ALIGN_END =
{
    /* 18 */
    0x09,         /*bLength: MOUSE_HID Descriptor size*/
    MOUSE_HID_DESCRIPTOR_TYPE, /*bDescriptorType: MOUSE_HID*/
    0x11,         /*bMOUSE_HIDUSTOM_HID: MOUSE_HID Class Spec release number*/
    0x01,
    0x00,         /*bCountryCode: Hardware target country*/
    0x01,         /*bNumDescriptors: Number of MOUSE_HID class descriptors to follow*/
    0x22,         /*bDescriptorType*/
    0x00,         /*wItemLength: Total length of Report descriptor*/  /* Mouse report descriptor size set during init */
    0x00,
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_MOUSE_HID_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
    USB_LEN_DEV_QUALIFIER_DESC,
    USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
};

// CUSTOMISED - change the mouse report descriptor size to match the current mode we're operating in
//            - point to corresponding descriptor
void MatchReportDescriptorToMode(USBD_HandleTypeDef *pdev, uint8_t BridgeMode)
{
    switch(BridgeMode)
    {
        case MODE_ABSOLUTE_MOUSE:
            /* report descriptor length */
            USBD_COMPOSITE_HID_CfgDesc[89] = USBD_MOUSE_ABS_REPORT_DESC_SIZE_LO; // LOBYTE
            USBD_COMPOSITE_HID_CfgDesc[90] = USBD_MOUSE_ABS_REPORT_DESC_SIZE_HI; // HIBYTE

            /* report size */
            USBD_COMPOSITE_HID_CfgDesc[95] = LOBYTE(MOUSE_ABS_REPORT_LENGTH); // LOBYTE
            USBD_COMPOSITE_HID_CfgDesc[96] = HIBYTE(MOUSE_ABS_REPORT_LENGTH); // HIBYTE

            /* point to correct descriptor */
            ((USBD_MOUSE_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceMOUSE)->pReport = mouse_abs_ReportDesc_FS;
            break;

        case MODE_RELATIVE_MOUSE:
            /* report descriptor length */
            USBD_COMPOSITE_HID_CfgDesc[89] = USBD_MOUSE_REL_REPORT_DESC_SIZE_LO; // LOBYTE
            USBD_COMPOSITE_HID_CfgDesc[90] = USBD_MOUSE_REL_REPORT_DESC_SIZE_HI; // HIBYTE

            /* report size */
            USBD_COMPOSITE_HID_CfgDesc[95] = LOBYTE(MOUSE_REL_REPORT_LENGTH); // LOBYTE
            USBD_COMPOSITE_HID_CfgDesc[96] = HIBYTE(MOUSE_REL_REPORT_LENGTH); // HIBYTE

            /* point to correct descriptor */
            ((USBD_MOUSE_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceMOUSE)->pReport = mouse_rel_ReportDesc_FS;
            break;

        case MODE_PARALLEL_DIGITIZER:
            /* report descriptor length */
            USBD_COMPOSITE_HID_CfgDesc[89] = USBD_MOUSE_PAR_DIGITIZER_DESC_SIZE_LO; // LOBYTE
            USBD_COMPOSITE_HID_CfgDesc[90] = USBD_MOUSE_PAR_DIGITIZER_DESC_SIZE_HI; // HIBYTE

            /* report size */
            USBD_COMPOSITE_HID_CfgDesc[95] = LOBYTE(MOUSE_PARALLEL_DIGITIZER_REPORT_LENGTH); // LOBYTE
            USBD_COMPOSITE_HID_CfgDesc[96] = HIBYTE(MOUSE_PARALLEL_DIGITIZER_REPORT_LENGTH); // HIBYTE

            /* point to correct descriptor */
            ((USBD_MOUSE_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceMOUSE)->pReport = mouse_parallel_digitizer_ReportDesc_FS;
            break;

        default:
            /* report descriptor length */
            USBD_COMPOSITE_HID_CfgDesc[89] = USBD_MOUSE_PAR_DIGITIZER_DESC_SIZE_LO; // LOBYTE
            USBD_COMPOSITE_HID_CfgDesc[90] = USBD_MOUSE_PAR_DIGITIZER_DESC_SIZE_HI; // HIBYTE

            /* report size */
            USBD_COMPOSITE_HID_CfgDesc[95] = LOBYTE(MOUSE_PARALLEL_DIGITIZER_REPORT_LENGTH); // LOBYTE
            USBD_COMPOSITE_HID_CfgDesc[96] = HIBYTE(MOUSE_PARALLEL_DIGITIZER_REPORT_LENGTH); // HIBYTE

            /* point to correct descriptor */
            ((USBD_MOUSE_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceMOUSE)->pReport = mouse_parallel_digitizer_ReportDesc_FS;
            break;
    }
}

// CUSTOMISED - finds the report descriptor length depending on what mode we're in
void GetMouseDescriptorLength(uint8_t BridgeMode)
{

    switch(BridgeMode)
    {
        case MODE_ABSOLUTE_MOUSE:
            byMouseReportLength = MOUSE_ABS_REPORT_LENGTH;
            break;

        case MODE_RELATIVE_MOUSE:
            byMouseReportLength = MOUSE_REL_REPORT_LENGTH;
            break;

        case MODE_PARALLEL_DIGITIZER:
            byMouseReportLength = MOUSE_PARALLEL_DIGITIZER_REPORT_LENGTH;
            break;

        default:
            byMouseReportLength = MOUSE_PARALLEL_DIGITIZER_REPORT_LENGTH;
            break;
    }
}

// CUSTOMISED - alters the number of interfaces presented to host --> enables/disables the mouse interface
void ConfigureCfgDescriptor(uint8_t MousMode)
{
    if(BridgeMode == MODE_ABSOLUTE_MOUSE || BridgeMode == MODE_RELATIVE_MOUSE || BridgeMode == MODE_PARALLEL_DIGITIZER)  // mouse/digitizer enabled
    {
        boMouseEnabled = true;
        NumInterfaces = 3;
    }
    else
    {
        boMouseEnabled = false;
        NumInterfaces = 2;
    }
}

// CUSTOMISED - adjust the PID of the device to match the mode we're operating in (allows TouchHub to know what to expect)
void ConfigurePID(uint8_t BridgeMode)
{
    switch(BridgeMode)
    {
        case MODE_ABSOLUTE_MOUSE:
            USBD_FS_DeviceDesc[10] = LOBYTE(DEVICE_MODE_PID_ABSOLUTE_MOUSE); // LOBYTE
            USBD_FS_DeviceDesc[11] = HIBYTE(DEVICE_MODE_PID_ABSOLUTE_MOUSE); // HIBYTE
            break;

        case MODE_RELATIVE_MOUSE:
            USBD_FS_DeviceDesc[10] = LOBYTE(DEVICE_MODE_PID_RELATIVE_MOUSE); // LOBYTE
            USBD_FS_DeviceDesc[11] = HIBYTE(DEVICE_MODE_PID_RELATIVE_MOUSE); // HIBYTE
            break;

        case MODE_PARALLEL_DIGITIZER:
            USBD_FS_DeviceDesc[10] = LOBYTE(DEVICE_MODE_PID_PARALLEL_DIGITIZER); // LOBYTE
            USBD_FS_DeviceDesc[11] = HIBYTE(DEVICE_MODE_PID_PARALLEL_DIGITIZER); // HIBYTE
            break;

        default:
            USBD_FS_DeviceDesc[10] = LOBYTE(DEVICE_MODE_PID_TBP); // LOBYTE
            USBD_FS_DeviceDesc[11] = HIBYTE(DEVICE_MODE_PID_TBP); // HIBYTE
            break;
    }
}


/**
  * @brief  USBD_MOUSE_HID_Init
  *         Initialize the MOUSE_HID interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t  USBD_MOUSE_HID_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx)
{
  uint8_t ret = 0;
  USBD_MOUSE_HID_HandleTypeDef     *hhid;
  /* Open EP IN */
  USBD_LL_OpenEP(pdev,
                 MOUSE_HID_EPIN,
                 USBD_EP_TYPE_INTR,
                 MOUSE_HID_EPIN_SIZE);

  pdev->pClassDataMOUSE = (USBD_MOUSE_HID_HandleTypeDef*)USBD_malloc_mouse(sizeof(USBD_MOUSE_HID_HandleTypeDef)); //, MOUSE_INTERFACE_NUM

  if(pdev->pClassDataMOUSE == NULL)
  {
    ret = 1;
  }
  else
  {
    hhid = (USBD_MOUSE_HID_HandleTypeDef*) pdev->pClassDataMOUSE;

    hhid->state = MOUSE_HID_IDLE;
    ((USBD_MOUSE_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceMOUSE)->Init();
  }

  return ret;
}

/**
  * @brief  USBD_MOUSE_HID_DeInit
  *         DeInitialize the MOUSE_HID layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t  USBD_MOUSE_HID_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx)
{
  /* Close MOUSE_HID EP IN */
  USBD_LL_CloseEP(pdev,
                  MOUSE_HID_EPIN);

  /* FRee allocated memory */
  if(pdev->pClassDataMOUSE != NULL)
  {
    ((USBD_MOUSE_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceMOUSE)->DeInit();
    USBD_free(pdev->pClassDataMOUSE);
    pdev->pClassDataMOUSE = NULL;
  }
  return USBD_OK;
}

/**
  * @brief  USBD_MOUSE_HID_Setup
  *         Handle the MOUSE_HID specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
//CUSTOMISED
uint8_t  USBD_MOUSE_HID_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req)
{
  uint16_t len = 0;
  uint8_t  *pbuf = NULL;
  USBD_MOUSE_HID_HandleTypeDef     *hhid = (USBD_MOUSE_HID_HandleTypeDef*)pdev->pClassDataMOUSE;
  uint8_t buffer[USBD_MOUSE_HID_REPORT_IN_SIZE];
  int8_t state;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
      case USB_REQ_TYPE_CLASS :
          switch (req->bRequest)
          {


          case MOUSE_HID_REQ_SET_PROTOCOL:
            hhid->Protocol = (uint8_t)(req->wValue);
            break;

          case MOUSE_HID_REQ_GET_PROTOCOL:
            USBD_CtlSendData (pdev,
                              (uint8_t *)&hhid->Protocol,
                              1);
            break;

          case MOUSE_HID_REQ_SET_IDLE:
            hhid->IdleState = (uint8_t)(req->wValue >> 8);
            break;

          case MOUSE_HID_REQ_GET_IDLE:
            USBD_CtlSendData (pdev,
                              (uint8_t *)&hhid->IdleState,
                              1);
            break;

// ============ CUSTOMISED BEGIN ============
          case MOUSE_HID_REQ_SET_REPORT:
              hhid->IsReportAvailable = 1;
              USBD_CtlPrepareRx (pdev, hhid->Report_buf, (uint8_t)(req->wLength));
              // Think the data from here gets sent to the control endpoint (0) rather than being directed to
              // this interface.
              break;

          case MOUSE_HID_REQ_GET_REPORT:
              len = sizeof(buffer) - 1;
              state = ((USBD_MOUSE_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceMOUSE)->GetFeature(req->wValue & 0xff,
                                                                                                      &buffer[1],
                                                                                                      &len);
              if(state == USBD_OK)
              {
                 // Copy ReportID and adjust length as ID must also be considered
                 buffer[0] = req->wValue & 0xff;
                 len++;

                 // Length MUST NOT be bigger than USBD_CUSTOMHID_INREPORT_BUF_SIZE
                 if(len > USBD_MOUSE_HID_REPORT_IN_SIZE)
                 {
                    len = USBD_MOUSE_HID_REPORT_IN_SIZE;
                 }
                 USBD_CtlSendData (pdev,
                                   buffer,
                                   len);
              }
              else
              {
                 USBD_CtlError (pdev, req);
                 return USBD_FAIL;
              }
              break;
// ============ CUSTOMISED END ============

          default:
            USBD_CtlError (pdev, req);
            return USBD_FAIL;
          }
          break;

  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    // CUSTOMISED
    /* From my understanding:
     * wValue (uint16) is bDescriptorIndex + bDescriptorType, hence the 8 bit RShift below
     * wIndex (uint16) is wInterfaceNumber
     * (from the USBD_SetupReqTypedef structure found at the top of this function)
     */
    case USB_REQ_GET_DESCRIPTOR:
      if( req->wValue >> 8 == MOUSE_HID_REPORT_DESC)
      {
          // sets correct report length to give to host
          uint8_t DescSize_LoByte = USBD_COMPOSITE_HID_CfgDesc[89];
          uint8_t DescSize_HiByte = USBD_COMPOSITE_HID_CfgDesc[90];
          uint16_t MouseReportDescSize = (DescSize_HiByte << 8) | (DescSize_LoByte); // concatenates the low and bytes into a 16 bit value

          len = MIN(MouseReportDescSize , req->wLength);
          pbuf =  ((USBD_MOUSE_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceMOUSE)->pReport;
      }
      else if( req->wValue >> 8 == MOUSE_HID_DESCRIPTOR_TYPE)
      {
        pbuf = USBD_MOUSE_HID_Desc;
        len = MIN(USB_MOUSE_HID_DESC_SIZ , req->wLength);
      }

      USBD_CtlSendData (pdev,
                        pbuf,
                        len);
      break;

    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev,
                        (uint8_t *)&hhid->AltSetting,
                        1);
      break;

    case USB_REQ_SET_INTERFACE :
      hhid->AltSetting = (uint8_t)(req->wValue);
      break;
    }
  }
  return USBD_OK;
}

/**
  * @brief  USBD_MOUSE_HID_SendReport
  *         Send MOUSE_HID Report
  * @param  pdev: device instance
  * @param  buff: pointer to report
  * @retval status
  */
uint8_t USBD_MOUSE_HID_SendReport     (USBD_HandleTypeDef  *pdev,
                                 uint8_t *report,
                                 uint16_t len)
{
  USBD_MOUSE_HID_HandleTypeDef     *hhid = (USBD_MOUSE_HID_HandleTypeDef*)pdev->pClassDataMOUSE;

  if (pdev->dev_state == USBD_STATE_CONFIGURED )
  {
    if(hhid->state == MOUSE_HID_IDLE)
    {
      hhid->state = MOUSE_HID_BUSY;
      USBD_LL_Transmit (pdev,
                        MOUSE_HID_EPIN,
                        report,
                        len);
    }
  }
  return USBD_OK;
}

// CUSTOMISED
// this gets called first to reduce code clutter from the while loop
uint8_t SendReport_MouseEndpoint(USBD_HandleTypeDef  *pdev, uint8_t *report, uint16_t len)
{
    uint8_t status;

    status = USBD_MOUSE_HID_SendReport(pdev, report, len);

    return status;
}

// CUSTOMISED
USB_HID_StateTypeDef USBD_MOUSE_HID_GetState     (USBD_HandleTypeDef  *pdev)
{
  USBD_MOUSE_HID_HandleTypeDef     *hhid = (USBD_MOUSE_HID_HandleTypeDef*)pdev->pClassDataMOUSE;

  return hhid->state;
}

/**
  * @brief  USBD_MOUSE_HID_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t  USBD_MOUSE_HID_DataIn (USBD_HandleTypeDef *pdev,
                              uint8_t epnum)
{

  /* Ensure that the FIFO is empty before a new transfer, this condition could
  be caused by  a new transfer before the end of the previous transfer */
  ((USBD_MOUSE_HID_HandleTypeDef *)pdev->pClassDataMOUSE)->state = MOUSE_HID_IDLE;

  return USBD_OK;
}

/**
  * @brief  USBD_MOUSE_HID_EP0_RxReady
  *         Handles control request data.
  * @param  pdev: device instance
  * @retval status
  */
uint8_t USBD_MOUSE_HID_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_MOUSE_HID_HandleTypeDef     *hhid = (USBD_MOUSE_HID_HandleTypeDef*)pdev->pClassDataMOUSE;

  if (hhid->IsReportAvailable == 1)
  {
//    ((USBD_MOUSE_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceMOUSE)->OutEvent(hhid->Report_buf);
      ((USBD_MOUSE_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceMOUSE)->SetFeature(hhid->Report_buf[0],
                                                                                  &hhid->Report_buf[1]);
    hhid->IsReportAvailable = 0;
  }

  return USBD_OK;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t  *USBD_MOUSE_HID_GetDeviceQualifierDesc (uint16_t *length)
{
  *length = sizeof (USBD_MOUSE_HID_DeviceQualifierDesc);
  return USBD_MOUSE_HID_DeviceQualifierDesc;
}

/**
* @brief  USBD_MOUSE_HID_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: MOUSEHID Interface callback
  * @retval status
  */
uint8_t  USBD_MOUSE_HID_RegisterInterface  (USBD_HandleTypeDef   *pdev,
                                             USBD_MOUSE_HID_ItfTypeDef *fops)
{
  uint8_t  ret = USBD_FAIL;

  if(fops != NULL)
  {
    pdev->pClassSpecificInterfaceMOUSE = fops;
    ret = USBD_OK;
  }

  return ret;
}
/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
