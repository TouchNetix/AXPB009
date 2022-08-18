/*******************************************************************************
* @file           : Command_Processor.c
* @author         : James Cameron (TouchNetix)
* @date           : 24 Sep 2020
*******************************************************************************/

/*
******************************************************************************
* Copyright (c) 2022 TouchNetix
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
******************************************************************************
*/

/*============ Includes ============*/
#include <stdbool.h>
#include "Init.h"
#include "Command_Processor.h"
#include "Comms.h"
#include "SPI_comms.h"
#include "I2C_Comms.h"
#include "usbd_generic_if.h"
#include "usbd_mouse_if.h"
#include "usbd_press_if.h"
#include "Flash_Control.h"
#include "Proxy_driver.h"
#include "Digitizer.h"
#include "Press_driver.h"
#include "Usage_Builder.h"
#include "Mode_Control.h"
#include "usb_device.h"
#include "Timers_and_LEDs.h"

/*============ Defines ============*/
#define READ                            (0x80)
#define WRITE                           (0x00)
#define PROXY_SETTINGS_OK               (0x00u)
#define INVALID_SETTINGS                (0x01u)
#define INVALID_COMMAND                 (0x99u)
#define MODE_SWITCH_OK                  (0xE7u)
#define SPI_MODE_ADDRESS                (0x01u)
#define I2C_ERROR                       (0x81u)
#define UNKNOWN_DEVICE                  (0xFFu)
#define ID_F042                         (0x0Au)
#define ID_F070                         (0x0Bu)
#define ID_F072                         (0x0Cu)

/*------------TBP COMMANDS------------*/
#define CMD_ZERO                        (0x00u)     /* TH2 will call this as it exits - stops proxy mode, starts counter to re-enable proxy mode once it has closed */
#define CMD_AXIOM_COMMS                 (0x51u)     /* used by TH2 to read/write from the bridge */
#define CMD_MULTIPAGE_READ              (0x71u)     /* NOTE: this is NOT the same as proxy mode, TH2 will request this command each time it wants a block */
#define CMD_SET_CONFIG                  (0x80u)     /* TH2 COMPATIBILITY - sets parameters for bridge to use */
#define CMD_NULL                        (0x86u)     /* doesn't do anything other than cancels proxy if sent to control endpoint */
#define CMD_START_PROXY                 (0x88u)     /* bridge continually reads reports from aXiom and chucks it up the USB to the host */
#define CMD_GET_CONFIG                  (0x8Bu)     /* TH2 COMPATIBILITY - TH2 reads some operating parameters from the bridge */
#define CMD_RESET_AXIOM                 (0x99u)     /* allows user to reset aXiom at will via a command */
#define CMD_WRITE_USAGE                 (0xA2u)     /* used when in digitizer or mouse mode - i.e. when used in anything that isn't TH2 */
#define CMD_READ_USAGE                  (0xA3u)     /* used when in digitizer or mouse mode - i.e. when used in anything that isn't TH2 */
#define CMD_FIND_I2C_ADDRESS            (0xE0u)     /* returns the i2c address of aXiom, or reports as in SPI mode */

//------------Mode switch Commands
#define CMD_BLOCK_DIGITIZER_REPORTS     (0x87u)     /* enables/disables mouse reports */
#define CMD_BLOCK_PRESS_REPORTS         (0xB1u)     /* enables/disables press reports */
#define CMD_RESET_BRIDGE                (0xEFu)
#define CMD_GET_PART_ID                 (0xF0u)     /* returns an id used by TH2 to load the correct dfu file */
#define CMD_ENTER_BOOTLOADER            (0xF5u)
#define CMD_GET_BRIDGE_MODE             (0xF9u)
#define CMD_SWITCH_MODE_TBP_BASIC       (0xFAu)
#define CMD_SWITCH_MODE_TBP_DIGITIZER   (0xFEu)
#define CMD_SWITCH_MODE_TBP_ABS_MOUSE   (0xFFu)

//------------Reserved Commands - Used by the PB005/7, and as such should not be used here
#define CMD_IIC_DATA_2                      (0x52u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_3                      (0x53u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_4                      (0x54u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_5                      (0x55u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_256_EXTRA              (0x60u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_1_256                  (0x61u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_2_256                  (0x62u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_3_256                  (0x63u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_4_256                  (0x64u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_5_256                  (0x65u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_x256                   (0x68u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_1_x256                 (0x69u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_2_x256                 (0x6Au) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_3_x256                 (0x6Bu) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_4_x256                 (0x6Cu) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_5_x256                 (0x6Du) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_2_MP                   (0x72u) /* RESERVED - Used by the PB005/7 */
#define CMD_CONFIG_READ_PINS                (0x81u) /* RESERVED - Used by the PB005/7 */
#define CMD_GET_VOLTAGE                     (0xA0u) /* RESERVED - Used by the PB005/7 */
#define CMD_CALL_SELFTEST                   (0xA1u) /* RESERVED - Used by the PB005/7 */
#define CMD_CLOCK_PRESS                     (0xB0u) /* RESERVED - Used by the PB005/7 */
#define CMD_RnR_PEAK_PRESS                  (0xC0u) /* RESERVED - Used by the PB005/7 */
#define CMD_FLIP_MOUSE_AXES                 (0xD0u) /* RESERVED - Used by the PB005/7 */
#define CMD_SET_IO_A                        (0xE4u) /* RESERVED - Used by the PB005/7 */
#define CMD_GET_IO_A                        (0xE5u) /* RESERVED - Used by the PB005/7 */
#define CMD_SET_IO_B                        (0xE6u) /* RESERVED - Used by the PB005/7 */
#define CMD_GET_IO_B                        (0xE7u) /* RESERVED - Used by the PB005/7 */
#define CMD_SET_IO_C                        (0xE8u) /* RESERVED - Used by the PB005/7 */
#define CMD_GET_IO_C                        (0xE9u) /* RESERVED - Used by the PB005/7 */
#define CMD_SAVE_CONFIGS_EEPROM             (0xEAu) /* RESERVED - Used by the PB005/7 */
#define CMD_RESTORE_DEFAULT_CONFIGS         (0xEBu) /* RESERVED - Used by the PB005/7 */
#define CMD_SWITCH_USB                      (0xF6u) /* RESERVED - Used by the PB005/7 */
#define CMD_CHECK_UUT_USB_ACTIVITY          (0xF7u) /* RESERVED - Used by the PB005/7 */
#define CMD_IIC_DATA_MASK                   (0xF8u) /* RESERVED - Used by the PB005/7 */
#define CMD_SWITCH_MODE_DEBUG               (0xFCu) /* RESERVED - Used by the PB005/7 */
#define CMD_SWITCH_MODE_SERIAL_DIGITIZER    (0xFDu) /* RESERVED - Used by the PB005/7 */

/*============ Local Variables ============*/

/*============ Exported Variables ============*/
bool    boGenericTBPResponseWaiting = 0;
bool    boPressTBPResponseWaiting = 0;
bool    boRespondNow = 1;   // indicates the host is waiting for some sort of reply (normally status) - Unless specified, all commands send a response
                            // (Done this way as it saves on flash)
uint8_t *pTBPCommandReport = 0;

static bool UsageReadWrite_ErrorChecks(int16_t usage_table_idx, uint16_t usage_length_in_bytes);

/*============ Functions ============*/
static bool UsageReadWrite_ErrorChecks(int16_t usage_table_idx, uint16_t usage_length_in_bytes)
{
    bool error_check_passed;

    if(usagetable[usage_table_idx].numpages == 0)
    {
        /* usage length zero --> usage is a report placeholder so can't be written to */
        pTBPCommandReport[1] = 0x95;
        pTBPCommandReport[2] = 0x80;    // error flag
        error_check_passed = false;
    }
    else if(pTBPCommandReport[2] > (usagetable[usage_table_idx].numpages-1))
    {
        /* start page too large */
        pTBPCommandReport[1] = 0x94;
        pTBPCommandReport[2] = 0x80;    // error flag
        error_check_passed = false;
    }
    else if(pTBPCommandReport[3] >= usage_length_in_bytes)
    {
        /* offset into page address beyond end of usage */
        pTBPCommandReport[1] = 0x93;
        pTBPCommandReport[2] = 0x80;    // error flag
        error_check_passed = false;
    }
    else if(pTBPCommandReport[4] > usage_length_in_bytes)
    {
        /* too many bytes requested */
        pTBPCommandReport[1] = 0x92;
        pTBPCommandReport[2] = 0x80;    // error flag
        pTBPCommandReport[3] = (uint8_t)(usage_length_in_bytes & 0xFF);
        pTBPCommandReport[4] = (uint8_t)(usage_length_in_bytes >> 8);
        error_check_passed = false;
    }
    else
    {
        // no errors found
        error_check_passed = true;
    }

    return error_check_passed;
}

void ProcessTBPCommand()
{
    bool boProxyMode_temp;
    bool boInternalProxy_temp;
    boInternalProxy_temp = boInternalProxy; // stores the proxy mode so it can be reinstated in some functions (e.g. toggling digitizer, want proxy to be in same state as before command was sent)
    boProxyMode_temp = boProxyEnabled;  // stores the proxy mode so it can be reinstated in some functions (e.g. toggling digitizer, want proxy to be in same state as before command was sent)

    // if proxy mode is active and command comes via control endpoint, normally means host wants bridge to stop proxy so clear bit and de-init the interrupt line
    if(target_interface == GENERIC_INTERFACE_NUM)
    {
        pTBPCommandReport = pTBPCommandReportGeneric;

        // clear all proxy flags --> makes sure the process doesn't start halfway through next time
        boInternalProxy = 0;
        boProxyReportAvailable = 0;
        boProxyReportToProcess = 0;
        boProxyEnabled = 0;
        boUSBTimeoutEnabled = false; // any command stops reports

        // de-init proxy gpio pin
        DeInitProxyInterruptMode();
    }
    else if(target_interface == PRESS_INTERFACE_NUM)
    {
        pTBPCommandReport = pTBPCommandReportPress;
    }

    switch (pTBPCommandReport[0])
    {
//------------------------------------------------------------TBP COMMANDS------------------------------------------------------------
        case CMD_FIND_I2C_ADDRESS:
        {
            if(comms_mode == SPI)
            {
                pTBPCommandReport[1] = SPI_MODE_ADDRESS;
            }
            else if(comms_mode == I2C)
            {
                pTBPCommandReport[1] = device_address;
            }
            else
            {
                // error
                pTBPCommandReport[1] = I2C_ERROR;
            }

            break;
        }
//-------
        case CMD_RESET_AXIOM: //0x99
        {
            HAL_GPIO_WritePin(nRESET_GPIO_Port, nRESET, RESET);
            HAL_Delay(1);
            HAL_GPIO_WritePin(nRESET_GPIO_Port, nRESET, SET);
            HAL_Delay(500); // gives aXiom time to boot up again before having anything requested of it
            boProxyEnabled = boProxyMode_temp;  // reinstate previous proxy mode
            boInternalProxy = boInternalProxy_temp; // restore the mode proxy was in before function was called
            break;
        }
//-------
        case CMD_ZERO:  //0x00  /* TH2 will call this as it exits - stops proxy mode, starts counter to re-enable proxy mode once it has closed */
        {
            boUSBTimeoutEnabled = true;
            // set both LEDs ON
            HAL_GPIO_WritePin(LED_AXIOM_GPIO_Port, LED_AXIOM_Pin, SET);
            HAL_GPIO_WritePin(LED_USB_GPIO_Port, LED_USB_Pin, SET);
            break;
        }
//-------
        case CMD_AXIOM_COMMS: //0x51 /* used by TH2 to learn about the bridge, and read/write usages */
        {
            /* copy the no. bytes to write/read from aXiom */
            aXiom_NumBytesTx = pTBPCommandReport[1];
            aXiom_NumBytesRx = pTBPCommandReport[2];

            /* Copy data to transmit to aXiom, starting from 4th byte in USB input buffer (from host) and copying for SPI_NumBytesTx */
            memcpy(aXiom_Tx_Buffer, &pTBPCommandReport[3], aXiom_NumBytesTx);

            // don't need to react to status as they are sent to host in response packet
            (void)Comms_Sequence();

            // want a response immediately, so set up to send at end of this function
            memcpy(pTBPCommandReport, aXiom_Rx_Buffer[CircularBufferHead], USBD_GENERIC_HID_REPORT_IN_SIZE);
            break;
        }
//-------
        case CMD_MULTIPAGE_READ: //0x71     /* NOTE: this is NOT the same as proxy mode, TH2 will request this command each time it wants a block */
        {
            byMultiPage_Semaphore_UsageNumber = pTBPCommandReport[7];
            byMultiPage_Semaphore_UsageOffset = pTBPCommandReport[8];
            byMultiPage_Semaphore_StartByte   = pTBPCommandReport[9];
            byMultiPage_Semaphore_StopByte    = pTBPCommandReport[10];
            write_multipage_semaphore(START);

            aXiom_NumBytesTx          = pTBPCommandReport[1];  // no. bytes to write --> page num., no. bytes to read, RnW byte
            ProxyMP_TotalNumBytesRx = ((uint16_t)pTBPCommandReport[3] << 8) | (uint16_t)pTBPCommandReport[2]; // TOTAL no. bytes to read (concatenated into a 16 bit word)
            byProxyMP_PageLength    = pTBPCommandReport[4];   // length of a page (in case page size changes in firmware update)

            wdProxyMP_AddrStart = (pTBPCommandReport[6] << 8) | pTBPCommandReport[5];   //multi-page start target address
            wdProxyMP_BytesRead = 0;    //variable keeps track of how many bytes in we are

            /* These are copied after sending the semaphore (above) so we can re-use the same buffer */
            aXiom_NumBytesRx = (ProxyMP_TotalNumBytesRx <= NUMBYTES_RX_MP) ? ProxyMP_TotalNumBytesRx : (64 - 3 - 2 - 1);  // set read size to 58 (or no. requested bytes if less than endpoint size for whatever reason)
                                                                                                            // -3 ('nonsense' bytes at end) -2 (header) -1(??)
            aXiom_Tx_Buffer[0] = (uint8_t)(wdProxyMP_AddrStart & 0xFF);    // page address lobyte
            aXiom_Tx_Buffer[1] = (wdProxyMP_AddrStart >> 8);   // page address hibyte

            boReadInProgress = 0;
            boRespondNow = 0;
            break;
        }
//-------
        case CMD_NULL: //0x86  /* doesn't do anything other than cancels proxy if sent to control endpoint */
        {
            // responds with an echo of the command
            break;
        }
//-------
        case CMD_WRITE_USAGE:   //0xA2
        case CMD_READ_USAGE:    //0xA3
        {
            int16_t  usage_table_idx;
            uint16_t usage_length_in_bytes;
            uint16_t start_address;

            /* WRITE USAGE
             * Command bytes
             * 1: usage number
             * 2: relative start page (0 means 1st page, 1 meand 2ns page etc.)
             * 3: byte offset into page
             * 4: no. bytes to write (min 0, max usage_len_bytes - 1)
             * 5+: data to write
             *
             * READ USAGE
             * 1: usage number
             * 2: start page
             * 3: byte offset into usage
             * 4: no. bytes to read
             *
             * RETURN
             * 1-4: echo command
             * 5+: data read
             */

            if(!(BridgeMode == PARALLEL_DIGITIZER || BridgeMode == ABSOLUTE_MOUSE || 1))   // we're *always* in press mode so this never triggers!
            {
                /* bridge in incorrect mode */
                pTBPCommandReport[1] = 0x98;    // error code
                pTBPCommandReport[2] = 0x80;    // error flag
            }
            else
            {
                usage_table_idx = find_usage_from_table(pTBPCommandReport[1]);

                if(usage_table_idx < 0) // above function returns -1 if usage not found
                {
                    /* usage number is not known to bridge */
                    pTBPCommandReport[1] = 0x96;    // error code
                    pTBPCommandReport[2] = 0x80;    // error flag
                }
                else
                {
                    //calculate how many bytes long the usage is?
                    usage_length_in_bytes = ((usagetable[usage_table_idx].maxoffset & 0x80) == 0x00) ?
                                            ((uint16_t)usagetable[usage_table_idx].numpages) * (((uint16_t)(usagetable[usage_table_idx].maxoffset & 0x7F) + 1) * 2) :
                                            (((uint16_t)usagetable[usage_table_idx].numpages - 1) << 7) + (((uint16_t)(usagetable[usage_table_idx].maxoffset & 0x7F) + 1) * 2);

                    // if reading the host can ask for 0 bytes --> indicates they want to read the usage table entry
                    if((pTBPCommandReport[0] == CMD_READ_USAGE) && (pTBPCommandReport[4] == 0))
                    {
                        memcpy(&pTBPCommandReport[5], (uint8_t *)&usagetable[usage_table_idx], sizeof(usagetable[usage_table_idx]));
                    }
                    else
                    {
                        if(UsageReadWrite_ErrorChecks(usage_table_idx, usage_length_in_bytes) == false)
                        {
                            /* false means an error has been triggered */
                        }
                        else // no errors so continue with write
                        {
                            memset(aXiom_Tx_Buffer, 0, sizeof(aXiom_Tx_Buffer));

                            start_address = ((usagetable[usage_table_idx].maxoffset & 0x80) == 0x00) ?
                                         ((uint16_t)usagetable[usage_table_idx].startpage << 8) + (((uint16_t)pTBPCommandReport[2] * ((uint16_t)(usagetable[usage_table_idx].maxoffset & 0x7F) + 1)) * 2) + pTBPCommandReport[3] :
                                         ((uint16_t)usagetable[usage_table_idx].startpage << 8) + (((uint16_t)pTBPCommandReport[2] * 2) + pTBPCommandReport[3]);

                            aXiom_Tx_Buffer[0] = (uint8_t)(start_address & 0xFF);
                            aXiom_Tx_Buffer[1] = (uint8_t)((start_address >> 8) & 0xFF);

                            if(pTBPCommandReport[0] == CMD_READ_USAGE)
                            {
                                aXiom_Tx_Buffer[2] = (uint8_t)pTBPCommandReport[4]; // no. bytes host wants to write to device
                                aXiom_Tx_Buffer[3] = (uint8_t)READ;
                                aXiom_NumBytesTx = 4;
                                aXiom_NumBytesRx = pTBPCommandReport[4];
                            }
                            else
                            {
                                aXiom_Tx_Buffer[2] = (uint8_t)(pTBPCommandReport[4]); // no. bytes host wants to write to device
                                aXiom_Tx_Buffer[3] = (uint8_t)(WRITE); // sets the write flag

                                aXiom_NumBytesTx = 4 + pTBPCommandReport[4];
                                aXiom_NumBytesRx = 0; // doing a write here so not reading any bytes

                                // copy data to write into comms buffer ready to send
                                memcpy(&aXiom_Tx_Buffer[4], &pTBPCommandReport[5], aXiom_NumBytesTx);
                            }

                            if(Comms_Sequence() == HAL_ERROR)
                            {
                                /* comms error (internal) */
                                pTBPCommandReport[1] = 0x97;    // error code
                                pTBPCommandReport[2] = 0x80;    // error flag
                            }
                            else
                            {
                                if(pTBPCommandReport[0] == CMD_READ_USAGE)
                                {
                                    memcpy(&pTBPCommandReport[5], &aXiom_Rx_Buffer[CircularBufferHead][2], aXiom_NumBytesRx);
                                }
                                else
                                {
                                    // doing a write so only need to echo back command data
                                }
                            }
                        }
                    }
                }
            }

            break;
        }
//-------
        case CMD_START_PROXY: //0x88  /* bridge continually reads reports from aXiom and chucks it up the USB to the host */
        {
            if((pTBPCommandReport[2] == NUMPROXYBYTES_TX) && ((pTBPCommandReport[7] & 0x80) == READ)) // check no. write bytes and read bit is correct
            {
                InitProxyInterruptMode();   // sets pin PA0 as EXTI interrupt

                // update the local copy of the u34 address
                u34_addr              = (pTBPCommandReport[5] << 8) | pTBPCommandReport[4];
                boProxyEnabled        = 1;
                pTBPCommandReport[1]  = PROXY_SETTINGS_OK;
            }
            else
            {
                boProxyEnabled        = 0;
                pTBPCommandReport[1]  = INVALID_SETTINGS;
            }

            break;
        }
//-------
        case CMD_BLOCK_DIGITIZER_REPORTS: //0x87   /* enables/disables mouse reports */
        {
            boMouseEnabled = (pTBPCommandReport[1] == 0);   // if command byte is non-zero then the digitizer is disabled
            boProxyEnabled = boProxyMode_temp;  // restore the mode proxy was in before function was called
            boInternalProxy = boInternalProxy_temp; // restore the mode proxy was in before function was called
            break;
        }
//-------
        case CMD_BLOCK_PRESS_REPORTS: //0xB1
        {
            boBlockPressReports = (pTBPCommandReport[1] != 0);
            boProxyEnabled = boProxyMode_temp;  // reinstate previous proxy mode
            boInternalProxy = boInternalProxy_temp; // restore the mode proxy was in before function was called
            break;
        }

//------------------------------------------------------------Mode switch Commands------------------------------------------------------------

        case CMD_RESET_BRIDGE: //0xEF
        {
            RestartBridge();
            break;
        }
//-------
        case CMD_ENTER_BOOTLOADER: //0xF5
        {
            if((pTBPCommandReport[1] == 0xAAu)&&(pTBPCommandReport[2] == 0x55u)&&(pTBPCommandReport[3] == 0xA5u)&&(pTBPCommandReport[4] == 0x5Au))
            {
                StartBootloader();
            }

            break;
        }
//-------
        case CMD_GET_PART_ID: //0xF0
        {
#if defined(STM32F042x6)
            pTBPCommandReport[1] = ID_F042;
#elif defined(STM32F070xB)
            pTBPCommandReport[1] = ID_F070;
#elif defined(STM32F072xB)
            pTBPCommandReport[1] = ID_F072;
#else
            pTBPCommandReport[1] = UNKNOWN_DEVICE;
#endif
            break;
        }
//-------
        case CMD_GET_BRIDGE_MODE: //0xF9
        {
            /* returns the current mode of the bridge (either basic, digitizer or absolute mouse) */
            pTBPCommandReport[1] = BridgeMode;
            break;
        }
//-------
        case CMD_SWITCH_MODE_TBP_BASIC: //0xFA
        {
            if(pTBPCommandReport[1] == MODE_SWITCH_OK)
            {
                /* Change value of BridgeMode and store in flash (saves desired state when rebooted) then reboot */
                Store_BridgeMode_To_Flash(MODE_TBP_BASIC, RESTART);
            }
            else
            {
                pTBPCommandReport[1] = INVALID_SETTINGS;
            }

            break;
        }
//-------
        case CMD_SWITCH_MODE_TBP_DIGITIZER: //0xFE
        {
            if(pTBPCommandReport[1] == MODE_SWITCH_OK)
            {
                /* Change value of BridgeMode and store in flash (saves desired state when rebooted) */
                Store_BridgeMode_To_Flash(MODE_PARALLEL_DIGITIZER, RESTART);
            }
            else
            {
                pTBPCommandReport[1] = INVALID_SETTINGS;
            }

            break;
        }
//-------
        case CMD_SWITCH_MODE_TBP_ABS_MOUSE: //0xFF
        {
            if(pTBPCommandReport[1] == MODE_SWITCH_OK)
            {
                /* Change value of BridgeMode and store in flash (saves desired state when rebooted) */
                Store_BridgeMode_To_Flash(MODE_ABSOLUTE_MOUSE, RESTART);
            }
            else
            {
                pTBPCommandReport[1] = INVALID_SETTINGS;
            }

            break;
        }

//------------------------------------------------------------TH2 Compatibility Commands------------------------------------------------------------
/* The following commands are requested by TH2 at startup - we need to respond in a certain way so we don't break compatibilty */

        case CMD_SET_CONFIG: //0x80     /* sets parameters for bridge to use */
        {
            // all parameters are 'set in stone' so there's nothing to change
            break;
        }
        //-------
        case CMD_GET_CONFIG: //0x8B     /* TH2 reads some operating parameters from the bridge */
        {
            pTBPCommandReport[1]  = 0; // I2C not used
            pTBPCommandReport[2]  = 0; // I2C not used
            pTBPCommandReport[4]  = 0; // I2C not used
            pTBPCommandReport[5]  = 0; // not used
            pTBPCommandReport[6]  = 0; // not used
            pTBPCommandReport[7]  = 0; // I2C not used
            pTBPCommandReport[8]  = 0; // not used
            pTBPCommandReport[9]  = 0; // not used
            pTBPCommandReport[10] = 0; // not used
            pTBPCommandReport[14] = SPI_Speed_PreScaler; // this is now fixed
            break;
        }

//------------------------------------------------------------Defualt------------------------------------------------------------

        default:
        {
            /* unknown command received - return with command value and an error status */
            pTBPCommandReport[1] = INVALID_COMMAND;
            break;
        }
    }

    /* re-enable all reports now! */
    boBlockReports = 0;

    // respond immediately (for commands like SET_CONFIG, GET_CONFIG etc. that set parameters in the bridge)
    if (boRespondNow == 1)
    {
        if(target_interface == GENERIC_INTERFACE_NUM)
        {
            boGenericTBPResponseWaiting = 1;
        }
        else if(target_interface == PRESS_INTERFACE_NUM)
        {
            boPressTBPResponseWaiting = 1;
        }

        boRespondNow = 0;
    }
    boCommandWaitingToDecode = 0;
}
