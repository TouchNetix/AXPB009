/*******************************************************************************
* @file           : Flash_Controler.h
* @author         : James Cameron (TouchNetix)
* @date           : 6 Oct 2020
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

#ifndef FLASH_CONTROL_H_
#define FLASH_CONTROL_H_

/*============ Includes ============*/
#include <stdio.h>

/*============ Exported Variables ============*/

/*============ Exported Defines ============*/
#define MODE_TBP_BASIC              (0x00u)
#define MODE_ABSOLUTE_MOUSE         (0x01u)
#define MODE_SERIAL_DIGITIZER       (0x02u) /* redundant */
#define MODE_PARALLEL_DIGITIZER     (0x03u)
#define RESTART                     (1)
#define NO_RESTART                  (0)

/*============ Exported Function Prototypes ============*/
void    Store_BridgeMode_To_Flash(uint8_t BridgeMode_to_store, uint8_t restart_required);
uint8_t GetDeviceModeFromFlash(void);
uint8_t check_boot_sel(void);
void    check_boot_config(void);
void    write_boot_sel(uint8_t boot_bit);
void    StartBootloader(void);

#endif /* FLASH_CONTROL_H_ */
