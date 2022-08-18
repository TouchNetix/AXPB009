/**
  ******************************************************************************
  * @file    usbd_composite.c
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief   This file provides the COMPOSITE_HID core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                COMPOSITE_HID Class  Description
  *          ===================================================================
  *           This module manages the COMPOSITE_HID class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (COMPOSITE_HID) Version 1.11 Jun 27, 2001".
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
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "usb_device.h"
#include "usbd_composite.h"

#include "usbd_mouse.h"
#include "usbd_generic.h"
#include "usbd_press.h"
#include "usbd_mouse_if.h"
#include "usbd_generic_if.h"
#include "usbd_press_if.h"
#include "Mode_Control.h"
#include "Digitizer.h"
#include "Timers_and_LEDs.h"

// CUSTOMISED
/*============ Defines ============*/

/*============ Exported Variables ============*/
volatile uint8_t target_interface = 0;
uint8_t NumInterfaces = 0;

/*============ Local Functions Prototypes============*/
static uint8_t  USBD_COMPOSITE_HID_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx);

static uint8_t  USBD_COMPOSITE_HID_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx);

static uint8_t  USBD_COMPOSITE_HID_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req);

static uint8_t  *USBD_COMPOSITE_HID_GetCfgDesc (uint16_t *length);

static uint8_t  *USBD_COMPOSITE_HID_GetDeviceQualifierDesc (uint16_t *length);

static uint8_t  USBD_COMPOSITE_HID_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t  USBD_COMPOSITE_HID_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_COMPOSITE_HID_EP0_RxReady (USBD_HandleTypeDef  *pdev);
/**
  * @}
  */

/** @defgroup USBD_COMPOSITE_HID_Private_Variables
  * @{
  */


USBD_ClassTypeDef  USBD_COMPOSITE_HID =
{
  USBD_COMPOSITE_HID_Init,
  USBD_COMPOSITE_HID_DeInit,
  USBD_COMPOSITE_HID_Setup,
  NULL, /*EP0_TxSent*/
  USBD_COMPOSITE_HID_EP0_RxReady, /*EP0_RxReady*/ /* STATUS STAGE IN */
  USBD_COMPOSITE_HID_DataIn, /*DataIn*/
  USBD_COMPOSITE_HID_DataOut,
  NULL, /*SOF */
  NULL,
  NULL,
  USBD_COMPOSITE_HID_GetCfgDesc,
  USBD_COMPOSITE_HID_GetCfgDesc,
  USBD_COMPOSITE_HID_GetCfgDesc,
  USBD_COMPOSITE_HID_GetDeviceQualifierDesc,
};

/* USB COMPOSITE_HID device Configuration Descriptor */ //NOTE: THE BYTE NUMBERS LISTED START AT 1, NOT ZERO
__ALIGN_BEGIN uint8_t USBD_COMPOSITE_HID_CfgDesc[USB_COMPOSITE_HID_CONFIG_DESC_SIZE] __ALIGN_END = //static
{
        0x09,                                       /* bLength: Conguration Descriptor size */
        USB_DESC_TYPE_CONFIGURATION,                /* bDescriptorType: Configuration */
        0x00, 0x00,                                 /* wTotalLength: Bytes returned */  /* byte 2 = low byte, byte 3 = high byte */
        USBD_MAX_NUM_INTERFACES,                    /* bNumInterfaces: 3 */
        0x01,                                       /* bConfigurationValue: Identifies Configuration */
        0x02,                                       /* iConfiguration: Index of string descriptor describing the configurator */
        RESERVED | BUS_POWERED | REMOTE_WAKEUP,     /* bmAttributes: Specifies if device is bus/self powered and if it supports remote wakeup (bus powered, no remote wake-up) */
        0xC8,                                       /* bMaxPower 400mA: Specifies how much bus current device requires (0x.. * 2) */
        /* 09 bytes */

//============================ GENERIC

        /********************  Generic HID interface ********************/
        0x09,                           /* bLength: Interface descriptor size */
        0x04,                           /* bDescriptorType: Interface */
        GENERIC_INTERFACE_NUM,          /* bInterfaceNumber: no. interface */
        0x00,                           /* bAlternateSetting */
        0x02,                           /* bNumEndPoints */
        0x03,                           /* bInterfaceClass: HID */
        0x00,                           /* bInterfaceSubClass */
        0x00,                           /* bInterfaceProtocol */
        USBD_IDX_INTERFACE_GENERIC_STR, /* iInterface: Index to string that describes the interface */
        /* 18 bytes */

        /********************  Generic HID Descriptor ********************/
        0x09,                               /* bLength: COMPOSITE_HID Descriptor size*/
        COMPOSITE_HID_DESCRIPTOR_TYPE,      /* bDescriptorType: COMPOSITE_HID*/
        0x11, 0x01,                         /* bcdUSB: USB Spec release number (BCD)*/
        0x00,                               /* bCountryCode: Hardware target country*/
        0x01,                               /* bNumDescriptors: Number of COMPOSITE_HID class descriptors to follow*/
        0x22,                               /* bDescriptorType*/
        USBD_GENERIC_HID_REPORT_DESC_SIZE,  /* wItemLength: Total length of Report descriptor*/
        0x00,
        /* 27 bytes */

        /********************  Generic HID endpoints ********************/
        0x07,                                                           /* bLength: Endpoint descriptor size */
        0x05,                                                           /* bDescriptorType: Endpoint */
        GENERIC_HID_EPIN,                                               /* bEndpointAddress: Endpoint number and direction (address 1, IN) */
        0x03,                                                           /* bmAttributes: Interrupt */
        LOBYTE(USB_FS_MAX_PACKET_SIZE), HIBYTE(USB_FS_MAX_PACKET_SIZE), /* wMaxPacketSize: Max. no. data bytes the endpoint can transfer in a transaction */
        0x01,                                                           /* bInterval: Polling interval in ms */
        /* 34 bytes */

        0x07,                                                           /* bLength: Endpoint descriptor size */
        0x05,                                                           /* bDescriptorType: Endpoint */
        GENERIC_HID_EPOUT,                                              /* bEndpointAddress: Endpoint number and direction (address 1, OUT) */
        0x03,                                                           /* bmAttributes: Interrupt */
        LOBYTE(USB_FS_MAX_PACKET_SIZE), HIBYTE(USB_FS_MAX_PACKET_SIZE), /* wMaxPacketSize: Max. no. data bytes the endpoint can transfer in a transaction */
        0x05,                                                           /* bInterval: Polling interval in ms */
        /* 41 bytes */

//============================ PRESS

        /*********************  Press HID interface *********************/
        0x09,                           /* bLength: Interface descriptor size */
        0x04,                           /* bDescriptorType: Interface */
        PRESS_INTERFACE_NUM,            /* bInterfaceNumber: no. interface */
        0x00,                           /* bAlternateSetting */
        0x02,                           /* bNumEndPoints */
        0x03,                           /* bInterfaceClass: HID */
        0x00,                           /* bInterfaceSubClass */
        0x00,                           /* bInterfaceProtocol */
        USBD_IDX_INTERFACE_PRESS_STR,   /* iInterface: Index to string that describes the interface */
        /* 50 bytes */

        /********************  Press HID Descriptor ********************/
        0x09,                                   /* bLength: PRESS_HID Descriptor size*/
        COMPOSITE_HID_DESCRIPTOR_TYPE,          /* bDescriptorType: PRESS_HID*/
        0x11, 0x01,                             /* bcdUSB: USB Spec release number*/
        0x00,                                   /* bCountryCode: Hardware target country*/
        0x01,                                   /* bNumDescriptors: Number of PRESS_HID class descriptors to follow*/
        0x22,                                   /* bDescriptorType*/
        USBD_PRESS_HID_REPORT_DESC_SIZE, 0x00,  /* wItemLength: Total length of Report descriptor*/
        /* 59 bytes */

        /*********************  Press HID endpoints *********************/
        0x07,                                                                   /* bLength: Endpoint descriptor size */
        0x05,                                                                   /* bDescriptorType: Endpoint */
        PRESS_HID_EPIN,                                                         /* bEndpointAddress: Endpoint number and direction (address 1, IN) */
        0x03,                                                                   /* bmAttributes: Interrupt */
        LOBYTE(USB_FS_MAX_PACKET_SIZE), HIBYTE(USB_FS_MAX_PACKET_SIZE),         /* wMaxPacketSize: Max. no. data bytes the endpoint can transfer in a transaction */
        0x01,                                                                   /* bInterval: Polling interval in ms */
        /* 66 bytes */

        0x07,                                                                   /* bLength: Endpoint descriptor size */
        0x05,                                                                   /* bDescriptorType: Endpoint */
        PRESS_HID_EPOUT,                                                        /* bEndpointAddress: Endpoint number and direction (address 1, OUT) */
        0x03,                                                                   /* bmAttributes: Interrupt */
        LOBYTE(USB_PRESS_OUT_PACKET_SIZE), HIBYTE(USB_PRESS_OUT_PACKET_SIZE),   /* wMaxPacketSize: Max. no. data bytes the endpoint can transfer in a transaction */
        0x05,                                                                   /* bInterval: Polling interval in ms */
        /* 73 bytes */

//============================ MOUSE

        /*********************  Mouse HID interface *********************/
        0x09,                           /* bLength: Interface descriptor size */
        0x04,                           /* bDescriptorType: Interface */
        MOUSE_INTERFACE_NUM,            /* bInterfaceNumber: no. interface */
        0x00,                           /* bAlternateSetting */
        0x01,                           /* bNumEndPoints */
        0x03,                           /* bInterfaceClass: HID */
        0x00,                           /* bInterfaceSubClass */
        0x00,                           /* bInterfaceProtocol */
        USBD_IDX_INTERFACE_MOUSE_STR,   /* iInterface: Index to string that describes the interface */
        /* 82 bytes */

        /********************  Mouse HID Descriptor ********************/
        0x09,                               /* bLength: MOUSE_HID Descriptor size*/
        COMPOSITE_HID_DESCRIPTOR_TYPE,      /* bDescriptorType: MOUSE_HID*/
        0x11, 0x01,                         /* bcdUSB: USB Spec release number*/
        0x00,                               /* bCountryCode: Hardware target country*/
        0x01,                               /* bNumDescriptors: Number of MOUSE_HID class descriptors to follow*/
        0x22,                               /* bDescriptorType*/
        0x00, 0x00,                         /* wItemLength: Total length of Report descriptor */        /* 90(low), 91(high)  - Mouse report descriptor size set during init */
        /* 91 bytes */

        /*********************  Mouse HID endpoints *********************/
        0x07,                     /* bLength: Endpoint descriptor size */
        0x05,                     /* bDescriptorType: Endpoint */
        MOUSE_HID_EPIN,           /* bEndpointAddress: Endpoint number and direction (address 1, IN) */
        0x03,                     /* bmAttributes: Interrupt */
        0x00, 0x00,               /* wMaxPacketSize: Max. no. data bytes the endpoint can transfer in a transaction */ /* 96(low), 97(high), Mouse report length, set during init */
        0x01,                     /* bInterval: Polling interval in ms */
        /* 98 bytes */
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_COMPOSITE_HID_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
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

/** @defgroup USBD_COMPOSITE_HID_Private_Functions
  * @{
  */

/**
  * @brief  USBD_COMPOSITE_HID_Init
  *         Initialize the HID+HID+HID interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_COMPOSITE_HID_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx)
{
  uint8_t ret = 0;
  /* GENERIC initialization */
  ret = USBD_GENERIC_HID_Init(pdev, cfgidx);
  if(ret != USBD_OK)
      return ret;

  /* PRESS initialization */
  ret = USBD_PRESS_HID_Init(pdev, cfgidx);
  if(ret != USBD_OK)
      return ret;

  /* MOUSE initialization */
  ret = USBD_MOUSE_HID_Init(pdev, cfgidx);
  if(ret != USBD_OK)
      return ret;

  return USBD_OK;
}

/**
  * @brief  USBD_COMPOSITE_HID_DeInit
  *         DeInitialize the HID+HID layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_COMPOSITE_HID_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx)
{
    /* GENERIC initialization */
    USBD_GENERIC_HID_DeInit(pdev, cfgidx);

    /* MOUSE initialization */
    USBD_MOUSE_HID_DeInit(pdev, cfgidx);

    /* PRESS initialization */
    USBD_PRESS_HID_DeInit(pdev, cfgidx);

    return USBD_OK;
}

/**
  * @brief  USBD_COMPOSITE_HID_Setup
  *         Handle the COMPOSITE_HID specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_COMPOSITE_HID_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req)
{
    /* identify if host is talking to generic, press or mouse */
    if (req->wIndex == GENERIC_INTERFACE_NUM)
        return USBD_GENERIC_HID_Setup(pdev, req);

    else if(req->wIndex == MOUSE_INTERFACE_NUM)
        return USBD_MOUSE_HID_Setup(pdev, req);

    else if(req->wIndex == PRESS_INTERFACE_NUM)
        return USBD_PRESS_HID_Setup(pdev, req);

    else // should never be in here
        return !USBD_OK;
}


/**
  * @brief  USBD_COMPOSITE_HID_GetCfgDesc
  *         return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer   2  3  high low
  */
static uint8_t  *USBD_COMPOSITE_HID_GetCfgDesc (uint16_t *length)
{
    uint8_t *USBD_COMPOSITE_HID_NO_DIGITIZER_CfgDesc = 0;

    // alter config descriptor depending on number of interfaces
    if(NumInterfaces == 2)
    {
        /* set reported length */
        USBD_COMPOSITE_HID_CfgDesc[2] = LOBYTE(USB_COMPOSITE_HID_CONFIG_NO_DIGITIZER_DESC_SIZE);  // low byte
        USBD_COMPOSITE_HID_CfgDesc[3] = HIBYTE(USB_COMPOSITE_HID_CONFIG_NO_DIGITIZER_DESC_SIZE);  // high byte

        /* set number of reported interfaces */
        USBD_COMPOSITE_HID_CfgDesc[4] = USBD_GENERIC_AND_PRESS_INTERFACES_ONLY;

        /* set pointer to start of config descriptor */
        USBD_COMPOSITE_HID_NO_DIGITIZER_CfgDesc = USBD_COMPOSITE_HID_CfgDesc;

        *length = USB_COMPOSITE_HID_CONFIG_NO_DIGITIZER_DESC_SIZE;
        return USBD_COMPOSITE_HID_NO_DIGITIZER_CfgDesc;
    }

    else
    {
        /* change reported length */
        USBD_COMPOSITE_HID_CfgDesc[2] = LOBYTE(USB_COMPOSITE_HID_CONFIG_DESC_SIZE);  // low byte
        USBD_COMPOSITE_HID_CfgDesc[3] = HIBYTE(USB_COMPOSITE_HID_CONFIG_DESC_SIZE);  // high byte

        /* set number of reported interfaces */
        USBD_COMPOSITE_HID_CfgDesc[4] = USBD_MAX_NUM_INTERFACES;

        *length = sizeof (USBD_COMPOSITE_HID_CfgDesc);
        return USBD_COMPOSITE_HID_CfgDesc;
    }
}

/**
  * @brief  USBD_COMPOSITE_HID_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_COMPOSITE_HID_DataIn (USBD_HandleTypeDef *pdev,
                              uint8_t epnum)
{
    /* identify which interface host is talking to */
    if(epnum == GENERIC_EPIN_IDX)
        return USBD_GENERIC_HID_DataIn(pdev, epnum);

    else if(epnum == MOUSE_EPIN_IDX)
        return USBD_MOUSE_HID_DataIn(pdev, epnum);

    else if(epnum == PRESS_EPIN_IDX)
        return USBD_PRESS_HID_DataIn(pdev, epnum);

    else
        return !USBD_OK;
}

/**
  * @brief  USBD_COMPOSITE_HID_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_COMPOSITE_HID_DataOut (USBD_HandleTypeDef *pdev,
                              uint8_t epnum)
{
    /* identify which interface host is talking to */
    if(epnum == GENERIC_EPOUT_IDX)
        return USBD_GENERIC_HID_DataOut(pdev, epnum);

    else if(epnum == PRESS_EPOUT_IDX)
        return USBD_PRESS_HID_DataOut(pdev, epnum);

    else
        return !USBD_OK;
}

/**
  * @brief  USBD_COMPOSITE_HID_EP0_RxReady
  *         Handles control request data.
  * @param  pdev: device instance
  * @retval status
  */
uint8_t USBD_COMPOSITE_HID_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
    return USBD_GENERIC_HID_EP0_RxReady(pdev);
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
static uint8_t  *USBD_COMPOSITE_HID_GetDeviceQualifierDesc (uint16_t *length)
{
  *length = sizeof (USBD_COMPOSITE_HID_DeviceQualifierDesc);
  return USBD_COMPOSITE_HID_DeviceQualifierDesc;
}

//CUSTOMISED
// allows USB transfers to be called from a single function - changing function parameters is cleaner!
/* @param interface - USB interface/endpoint that we want to send data to the host through
 * @param pdev - pointer to usb device handle
 * @param report - pointer to data buffer to send
 * @param len - length of data to send
 * @return status - HAL status check
 */
uint8_t Send_USB_Report(uint8_t interface,USBD_HandleTypeDef  *pdev, uint8_t *report, uint16_t len)
{
    uint8_t status = 0;

    if(boBlockReports == 0) // prevents proxy reports 'gumming up' the host usb buffer when a response is expected
    {
        // tell the main loop that there is comms happening
        boFlashUSBLED = 1;
        boUSBActivity = 1;

        switch(interface)
        {
            case GENERIC:
            {
                status = SendReport_ControlEndpoint(pdev, report, len);
                break;
            }
            case MOUSE:
            {
                status = SendReport_MouseEndpoint(pdev, report, len);
                break;
            }
            case PRESS:
            {
                status = SendReport_PressEndpoint(pdev, report, len);
                break;
            }
        }
    }

    return status;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
