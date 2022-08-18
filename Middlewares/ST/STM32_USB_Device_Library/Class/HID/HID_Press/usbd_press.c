/*
 * usbd_press.c
 *
 *  Created on: 28 Sep 2020
 *      Author: James Cameron (TouchNetix)
 */

/*          ===================================================================
 *                                PRESS_HID Class  Description
 *          ===================================================================
 *           This module manages the PRESS_HID class V1.11 following the "Device Class Definition
 *           for Human Interface Devices (PRESS_HID) Version 1.11 Jun 27, 2001".
 *           This driver implements the following aspects of the specification:
 *             - The Boot Interface Subclass
 *             - Usage Page : Generic Desktop
 *             - Usage : Vendor
 *             - Collection : Application
 *
 * @note     In HS mode and when the DMA is used, all variables and data structures
 *           dealing with the DMA during the transaction process should be 32-bit aligned.
 */

/* Includes ------------------------------------------------------------------*/
#include "usb_device.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "usbd_press.h"
#include "usbd_press_if.h"
#include "Digitizer.h"
/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_PRESS_HID
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_PRESS_HID_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_PRESS_HID_Private_Defines
  * @{
  */

/**
  * @}
  */


/** @defgroup USBD_PRESS_HID_Private_Macros
  * @{
  */
/**
  * @}
  */
/** @defgroup USBD_PRESS_HID_Private_FunctionPrototypes
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_PRESS_HID_Private_Variables
  * @{
  */

USBD_ClassTypeDef  USBD_PRESS_HID =
{
  USBD_PRESS_HID_Init,
  USBD_PRESS_HID_DeInit,
  USBD_PRESS_HID_Setup,
  NULL, /*EP0_TxSent*/
  USBD_PRESS_HID_EP0_RxReady, /*EP0_RxReady*/ /* STATUS STAGE IN */
  USBD_PRESS_HID_DataIn, /*DataIn*/
  USBD_PRESS_HID_DataOut,
  NULL, /*SOF */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  USBD_PRESS_HID_GetDeviceQualifierDesc,
};

/* USB PRESS_HID device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_PRESS_HID_Desc[USB_PRESS_HID_DESC_SIZ] __ALIGN_END =
{
  /* 18 */
  0x09,         /*bLength: PRESS_HID Descriptor size*/
  PRESS_HID_DESCRIPTOR_TYPE, /*bDescriptorType: PRESS_HID*/
  0x11,         /*bPRESS_HIDUSTOM_HID: PRESS_HID Class Spec release number*/
  0x01,
  0x00,         /*bCountryCode: Hardware target country*/
  0x01,         /*bNumDescriptors: Number of PRESS_HID class descriptors to follow*/
  0x22,         /*bDescriptorType*/
  USBD_PRESS_HID_REPORT_DESC_SIZE,/*wItemLength: Total length of Report descriptor*/
  0x00,
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_PRESS_HID_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
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

/** @defgroup USBD_PRESS_HID_Private_Functions
  * @{
  */

/**
  * @brief  USBD_PRESS_HID_Init
  *         Initialize the PRESS_HID interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t  USBD_PRESS_HID_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx)
{
  uint8_t ret = 0;
  USBD_PRESS_HID_HandleTypeDef     *hhid;
  /* Open EP IN */
  USBD_LL_OpenEP(pdev,
                 PRESS_HID_EPIN,
                 USBD_EP_TYPE_INTR,
                 PRESS_HID_EPIN_SIZE);

  /* Open EP OUT */
  USBD_LL_OpenEP(pdev,
                 PRESS_HID_EPOUT,
                 USBD_EP_TYPE_INTR,
                 PRESS_HID_EPOUT_SIZE);

  pdev->pClassDataPRESS = (USBD_PRESS_HID_HandleTypeDef*)USBD_malloc_press(sizeof(USBD_PRESS_HID_HandleTypeDef)); //, PRESS_INTERFACE_NUM

  if(pdev->pClassDataPRESS == NULL)
  {
    ret = 1;
  }
  else
  {
    hhid = (USBD_PRESS_HID_HandleTypeDef*) pdev->pClassDataPRESS;

    hhid->state = PRESS_HID_IDLE;
    ((USBD_PRESS_HID_ItfTypeDef *)pdev->pClassSpecificInterfacePRESS)->Init();
          /* Prepare Out endpoint to receive 1st packet */
    USBD_LL_PrepareReceive(pdev, PRESS_HID_EPOUT, hhid->Report_buf,
                           USBD_PRESS_HID_OUTREPORT_BUF_SIZE);
  }

  return ret;
}

/**
  * @brief  USBD_PRESS_HID_Init
  *         DeInitialize the PRESS_HID layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t  USBD_PRESS_HID_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx)
{
  /* Close PRESS_HID EP IN */
  USBD_LL_CloseEP(pdev,
                  PRESS_HID_EPIN);

  /* Close PRESS_HID EP OUT */
  USBD_LL_CloseEP(pdev,
                  PRESS_HID_EPOUT);

  /* FRee allocated memory */
  if(pdev->pClassDataPRESS != NULL)
  {
    ((USBD_PRESS_HID_ItfTypeDef *)pdev->pClassSpecificInterfacePRESS)->DeInit();
    USBD_free(pdev->pClassDataPRESS);
    pdev->pClassDataPRESS = NULL;
  }
  return USBD_OK;
}

/**
  * @brief  USBD_PRESS_HID_Setup
  *         Handle the PRESS_HID specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
uint8_t  USBD_PRESS_HID_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req)
{
  uint16_t len = 0;
  uint8_t  *pbuf = NULL;
  USBD_PRESS_HID_HandleTypeDef     *hhid = (USBD_PRESS_HID_HandleTypeDef*)pdev->pClassDataPRESS;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    switch (req->bRequest)
    {


    case PRESS_HID_REQ_SET_PROTOCOL:
      hhid->Protocol = (uint8_t)(req->wValue);
      break;

    case PRESS_HID_REQ_GET_PROTOCOL:
      USBD_CtlSendData (pdev,
                        (uint8_t *)&hhid->Protocol,
                        1);
      break;

    case PRESS_HID_REQ_SET_IDLE:
      hhid->IdleState = (uint8_t)(req->wValue >> 8);
      break;

    case PRESS_HID_REQ_GET_IDLE:
      USBD_CtlSendData (pdev,
                        (uint8_t *)&hhid->IdleState,
                        1);
      break;

    case PRESS_HID_REQ_SET_REPORT:
      hhid->IsReportAvailable = 1;
      USBD_CtlPrepareRx (pdev, hhid->Report_buf, (uint8_t)(req->wLength));

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
      if( req->wValue >> 8 == PRESS_HID_REPORT_DESC)
      {
        len = MIN(USBD_PRESS_HID_REPORT_DESC_SIZE , req->wLength);

        USBD_PRESS_HID_ItfTypeDef *pPtr_press = (USBD_PRESS_HID_ItfTypeDef *)pdev->pClassSpecificInterfacePRESS;
        if(pPtr_press != NULL)
        {
            pbuf = pPtr_press->pReport;
        }

        //pbuf =  ((USBD_PRESS_HID_ItfTypeDef *)pdev->pClassSpecificInterfacePRESS)->pReport;
      }
      else if( req->wValue >> 8 == PRESS_HID_DESCRIPTOR_TYPE)
      {
        pbuf = USBD_PRESS_HID_Desc;
        len = MIN(USB_PRESS_HID_DESC_SIZ , req->wLength);
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
  * @brief  USBD_PRESS_HID_SendReport
  *         Send PRESS_HID Report
  * @param  pdev: device instance
  * @param  buff: pointer to report
  * @retval status
  */
uint8_t USBD_PRESS_HID_SendReport     (USBD_HandleTypeDef  *pdev,
                                 uint8_t *report,
                                 uint16_t len)
{
  USBD_PRESS_HID_HandleTypeDef     *hhid = (USBD_PRESS_HID_HandleTypeDef*)pdev->pClassDataPRESS;

  if (pdev->dev_state == USBD_STATE_CONFIGURED )
  {
    if(hhid->state == PRESS_HID_IDLE)
    {
      hhid->state = PRESS_HID_BUSY;
      USBD_LL_Transmit (pdev,
                        PRESS_HID_EPIN,
                        report,
                        len);
    }
  }
  return USBD_OK;
}

// CUSTOMISED
// this gets called first to reduce code clutter from the while loop
uint8_t SendReport_PressEndpoint(USBD_HandleTypeDef  *pdev, uint8_t *report, uint16_t len)
{
    uint8_t status;

    status = USBD_PRESS_HID_SendReport(pdev, report, len);

    return status;
}

// CUSTOMISED
USB_HID_StateTypeDef USBD_PRESS_HID_GetState     (USBD_HandleTypeDef  *pdev)
{
  USBD_PRESS_HID_HandleTypeDef     *hhid = (USBD_PRESS_HID_HandleTypeDef*)pdev->pClassDataPRESS;

  return hhid->state;
}

/**
  * @brief  USBD_PRESS_HID_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t  USBD_PRESS_HID_DataIn (USBD_HandleTypeDef *pdev,
                              uint8_t epnum)
{

  /* Ensure that the FIFO is empty before a new transfer, this condition could
  be caused by  a new transfer before the end of the previous transfer */
  ((USBD_PRESS_HID_HandleTypeDef *)pdev->pClassDataPRESS)->state = PRESS_HID_IDLE;

  return USBD_OK;
}

/**
  * @brief  USBD_PRESS_HID_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t  USBD_PRESS_HID_DataOut (USBD_HandleTypeDef *pdev,
                              uint8_t epnum)
{

  USBD_PRESS_HID_HandleTypeDef     *hhid = (USBD_PRESS_HID_HandleTypeDef*)pdev->pClassDataPRESS;

  ((USBD_PRESS_HID_ItfTypeDef *)pdev->pClassSpecificInterfacePRESS)->OutEvent(hhid->Report_buf);

  USBD_LL_PrepareReceive(pdev, PRESS_HID_EPOUT , hhid->Report_buf,
                         USBD_PRESS_HID_OUTREPORT_BUF_SIZE);

  return USBD_OK;
}

/**
  * @brief  USBD_PRESS_HID_EP0_RxReady
  *         Handles control request data.
  * @param  pdev: device instance
  * @retval status
  */
uint8_t USBD_PRESS_HID_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_PRESS_HID_HandleTypeDef     *hhid = (USBD_PRESS_HID_HandleTypeDef*)pdev->pClassDataPRESS;

  if (hhid->IsReportAvailable == 1)
  {
    ((USBD_PRESS_HID_ItfTypeDef *)pdev->pClassSpecificInterfacePRESS)->OutEvent(hhid->Report_buf);
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
uint8_t  *USBD_PRESS_HID_GetDeviceQualifierDesc (uint16_t *length)
{
  *length = sizeof (USBD_PRESS_HID_DeviceQualifierDesc);
  return USBD_PRESS_HID_DeviceQualifierDesc;
}

/**
* @brief  USBD_PRESS_HID_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: PRESSHID Interface callback
  * @retval status
  */
uint8_t  USBD_PRESS_HID_RegisterInterface  (USBD_HandleTypeDef   *pdev,
                                             USBD_PRESS_HID_ItfTypeDef *fops)
{
  uint8_t  ret = USBD_FAIL;

  if(fops != NULL)
  {
    pdev->pClassSpecificInterfacePRESS = fops;
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

