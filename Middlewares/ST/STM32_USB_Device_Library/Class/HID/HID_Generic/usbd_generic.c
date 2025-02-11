/**
  ******************************************************************************
  * @file    usbd_generic.c
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief   This file provides the GENERIC_HID core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                GENERIC_HID Class  Description
  *          ===================================================================
  *           This module manages the GENERIC_HID class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (GENERIC_HID) Version 1.11 Jun 27, 2001".
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
#include "usbd_generic_if.h"
#include "usbd_generic.h"

#include "usb_device.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "Digitizer.h"

// CUSTOMISED
/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_GENERIC_HID
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_GENERIC_HID_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_GENERIC_HID_Private_Defines
  * @{
  */

/**
  * @}
  */


/** @defgroup USBD_GENERIC_HID_Private_Macros
  * @{
  */
/**
  * @}
  */
/** @defgroup USBD_GENERIC_HID_Private_FunctionPrototypes
  * @{
  */
/**
  * @}
  */

/** @defgroup USBD_GENERIC_HID_Private_Variables
  * @{
  */

USBD_ClassTypeDef  USBD_GENERIC_HID =
{
  USBD_GENERIC_HID_Init,
  USBD_GENERIC_HID_DeInit,
  USBD_GENERIC_HID_Setup,
  NULL, /*EP0_TxSent*/
  USBD_GENERIC_HID_EP0_RxReady, /*EP0_RxReady*/ /* STATUS STAGE IN */
  USBD_GENERIC_HID_DataIn, /*DataIn*/
  USBD_GENERIC_HID_DataOut,
  NULL, /*SOF */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  USBD_GENERIC_HID_GetDeviceQualifierDesc,
};

/* USB GENERIC_HID device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_GENERIC_HID_Desc[USB_GENERIC_HID_DESC_SIZ] __ALIGN_END =
{
  /* 18 */
  0x09,         /*bLength: GENERIC_HID Descriptor size*/
  GENERIC_HID_DESCRIPTOR_TYPE, /*bDescriptorType: GENERIC_HID*/
  0x11,         /*bGENERIC_HIDUSTOM_HID: GENERIC_HID Class Spec release number*/
  0x01,
  0x00,         /*bCountryCode: Hardware target country*/
  0x01,         /*bNumDescriptors: Number of GENERIC_HID class descriptors to follow*/
  0x22,         /*bDescriptorType*/
  LOBYTE(USBD_GENERIC_HID_REPORT_DESC_SIZE), HIBYTE(USBD_GENERIC_HID_REPORT_DESC_SIZE), /*wItemLength: Total length of Report descriptor*/
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_GENERIC_HID_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
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


/**
  * @}
  */

/** @defgroup USBD_GENERIC_HID_Private_Functions
  * @{
  */

/**
  * @brief  USBD_GENERIC_HID_Init
  *         Initialize the GENERIC_HID interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t  USBD_GENERIC_HID_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx)
{
  uint8_t ret = 0;
  USBD_GENERIC_HID_HandleTypeDef     *hhid;
  /* Open EP IN */
  USBD_LL_OpenEP(pdev,
                 GENERIC_HID_EPIN,
                 USBD_EP_TYPE_INTR,
                 GENERIC_HID_EPIN_SIZE);

  /* Open EP OUT */
  USBD_LL_OpenEP(pdev,
                 GENERIC_HID_EPOUT,
                 USBD_EP_TYPE_INTR,
                 GENERIC_HID_EPOUT_SIZE);

  pdev->pClassDataGENERIC = (USBD_GENERIC_HID_HandleTypeDef*)USBD_malloc_generic(sizeof(USBD_GENERIC_HID_HandleTypeDef)); //, GENERIC_INTERFACE_NUM

  if(pdev->pClassDataGENERIC == NULL)
  {
    ret = 1;
  }
  else
  {
    hhid = (USBD_GENERIC_HID_HandleTypeDef*) pdev->pClassDataGENERIC;

    hhid->state = GENERIC_HID_IDLE;
    ((USBD_GENERIC_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceGENERIC)->Init();
          /* Prepare Out endpoint to receive 1st packet */
    USBD_LL_PrepareReceive(pdev, GENERIC_HID_EPOUT, hhid->Report_buf,
                           USBD_GENERIC_HID_OUTREPORT_BUF_SIZE);
  }

  return ret;
}

/**
  * @brief  USBD_GENERIC_HID_Init
  *         DeInitialize the GENERIC_HID layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t  USBD_GENERIC_HID_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx)
{
  /* Close GENERIC_HID EP IN */
  USBD_LL_CloseEP(pdev,
                  GENERIC_HID_EPIN);

  /* Close GENERIC_HID EP OUT */
  USBD_LL_CloseEP(pdev,
                  GENERIC_HID_EPOUT);

  /* FRee allocated memory */
  if(pdev->pClassDataGENERIC != NULL)
  {
    ((USBD_GENERIC_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceGENERIC)->DeInit();
    USBD_free(pdev->pClassDataGENERIC);
    pdev->pClassDataGENERIC = NULL;
  }
  return USBD_OK;
}

/**
  * @brief  USBD_GENERIC_HID_Setup
  *         Handle the GENERIC_HID specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
uint8_t  USBD_GENERIC_HID_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req)
{
  uint16_t len = 0;
  uint8_t  *pbuf = NULL;
  USBD_GENERIC_HID_HandleTypeDef     *hhid = (USBD_GENERIC_HID_HandleTypeDef*)pdev->pClassDataGENERIC;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    switch (req->bRequest)
    {


    case GENERIC_HID_REQ_SET_PROTOCOL:
      hhid->Protocol = (uint8_t)(req->wValue);
      break;

    case GENERIC_HID_REQ_GET_PROTOCOL:
      USBD_CtlSendData (pdev,
                        (uint8_t *)&hhid->Protocol,
                        1);
      break;

    case GENERIC_HID_REQ_SET_IDLE:
      hhid->IdleState = (uint8_t)(req->wValue >> 8);
      break;

    case GENERIC_HID_REQ_GET_IDLE:
      USBD_CtlSendData (pdev,
                        (uint8_t *)&hhid->IdleState,
                        1);
      break;

    case GENERIC_HID_REQ_SET_REPORT:
      hhid->IsReportAvailable = 1;
      USBD_CtlPrepareRx (pdev, hhid->Report_buf, (uint8_t)(req->wLength));
      break;

    case GENERIC_HID_REQ_GET_REPORT:
        if ((uint8_t)req->wValue == REPORT_ID_DIGI_MAX_COUNT)
        {
            u34_TCP_report[0] = REPORT_ID_DIGI_MAX_COUNT;  // report ID
            u34_TCP_report[1] = 0x05u;  // max no. contacts
            USBD_CtlSendData(pdev, u34_TCP_report, req->wLength);
        }
        break;

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
      if( req->wValue >> 8 == GENERIC_HID_REPORT_DESC)
      {
        len = MIN(USBD_GENERIC_HID_REPORT_DESC_SIZE , req->wLength);

        USBD_GENERIC_HID_ItfTypeDef *pPtr_generic = (USBD_GENERIC_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceGENERIC;
        if(pPtr_generic != NULL)
        {
            pbuf = pPtr_generic->pReport;
        }

        //pbuf =  ((USBD_GENERIC_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceGENERIC)->pReport;
      }
      else if( req->wValue >> 8 == GENERIC_HID_DESCRIPTOR_TYPE)
      {
        pbuf = USBD_GENERIC_HID_Desc;
        len = MIN(USB_GENERIC_HID_DESC_SIZ , req->wLength);
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
  * @brief  USBD_GENERIC_HID_SendReport
  *         Send GENERIC_HID Report
  * @param  pdev: device instance
  * @param  buff: pointer to report
  * @retval status
  */
uint8_t USBD_GENERIC_HID_SendReport     (USBD_HandleTypeDef  *pdev,
                                 uint8_t *report,
                                 uint16_t len)
{
  USBD_GENERIC_HID_HandleTypeDef     *hhid = (USBD_GENERIC_HID_HandleTypeDef*)pdev->pClassDataGENERIC;

  if (pdev->dev_state == USBD_STATE_CONFIGURED )
  {
    if(hhid->state == GENERIC_HID_IDLE)
    {
      hhid->state = GENERIC_HID_BUSY;
      USBD_LL_Transmit (pdev,
                        GENERIC_HID_EPIN,
                        report,
                        len);
    }
  }
  return USBD_OK;
}

// CUSTOMISED
// this gets called first to reduce code clutter from the while loop
uint8_t SendReport_ControlEndpoint(USBD_HandleTypeDef  *pdev, uint8_t *report, uint16_t len)
{
    uint8_t status;

    status = USBD_GENERIC_HID_SendReport(pdev, report, len);

    return status;
}

// CUSTOMISED
USB_HID_StateTypeDef USBD_GENERIC_HID_GetState     (USBD_HandleTypeDef  *pdev)
{
  USBD_GENERIC_HID_HandleTypeDef     *hhid = (USBD_GENERIC_HID_HandleTypeDef*)pdev->pClassDataGENERIC;

  return hhid->state;
}

/**
  * @brief  USBD_GENERIC_HID_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t  USBD_GENERIC_HID_DataIn (USBD_HandleTypeDef *pdev,
                              uint8_t epnum)
{

  /* Ensure that the FIFO is empty before a new transfer, this condition could
  be caused by  a new transfer before the end of the previous transfer */
  ((USBD_GENERIC_HID_HandleTypeDef *)pdev->pClassDataGENERIC)->state = GENERIC_HID_IDLE;

  return USBD_OK;
}

/**
  * @brief  USBD_GENERIC_HID_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t  USBD_GENERIC_HID_DataOut (USBD_HandleTypeDef *pdev,
                              uint8_t epnum)
{

  USBD_GENERIC_HID_HandleTypeDef     *hhid = (USBD_GENERIC_HID_HandleTypeDef*)pdev->pClassDataGENERIC;

  ((USBD_GENERIC_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceGENERIC)->OutEvent(hhid->Report_buf);

  USBD_LL_PrepareReceive(pdev, GENERIC_HID_EPOUT , hhid->Report_buf,
                         USBD_GENERIC_HID_OUTREPORT_BUF_SIZE);

  return USBD_OK;
}

/**
  * @brief  USBD_GENERIC_HID_EP0_RxReady
  *         Handles control request data.
  * @param  pdev: device instance
  * @retval status
  */
uint8_t USBD_GENERIC_HID_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_GENERIC_HID_HandleTypeDef     *hhid = (USBD_GENERIC_HID_HandleTypeDef*)pdev->pClassDataGENERIC;

  if (hhid->IsReportAvailable == 1)
  {
    ((USBD_GENERIC_HID_ItfTypeDef *)pdev->pClassSpecificInterfaceGENERIC)->OutEvent(hhid->Report_buf);
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
uint8_t  *USBD_GENERIC_HID_GetDeviceQualifierDesc (uint16_t *length)
{
  *length = sizeof (USBD_GENERIC_HID_DeviceQualifierDesc);
  return USBD_GENERIC_HID_DeviceQualifierDesc;
}

/**
* @brief  USBD_GENERIC_HID_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: GENERICHID Interface callback
  * @retval status
  */
uint8_t  USBD_GENERIC_HID_RegisterInterface  (USBD_HandleTypeDef   *pdev,
                                             USBD_GENERIC_HID_ItfTypeDef *fops)
{
  uint8_t  ret = USBD_FAIL;

  if(fops != NULL)
  {
    pdev->pClassSpecificInterfaceGENERIC = fops;
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
