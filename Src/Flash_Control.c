/*******************************************************************************
* @file           : Flash_Control.c
* @author         : James Cameron (TouchNetix)
* @date           : 6 Oct 2020
*******************************************************************************/

/*
******************************************************************************
* Copyright (c) 2025 TouchNetix
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
#include "stm32f0xx.h"
#include "stm32f0xx_hal_flash_ex.h"
#include "_AXPB009_Main.h"
#include "Flash_Control.h"
#include "Mode_Control.h"
#include "Init.h"

#include "usbd_core.h"
#include "usb_device.h"

/*============ Defines ============*/
#define BOOT_PIN                (8)

#define BYTE0                   (0)
#define BYTE1                   (1)
#define CONFIG_BYTE_ADDR        (0x1FFFF802U)   // byte containing the config bytes used by the bridge at startup
#define OPTBYTE0_ADDR           (0x1FFFF804U)   // first byte in option register
#define OPTBYTE0_COMP_ADDR       (0x1FFFF805U)   // complement of first byte in option register
#define OPTBYTE1_ADDR           (0x1FFFF806U)   // second byte in option register
#define OPTBYTE1_COMP_ADDR       (0x1FFFF807U)   // complement of second byte in option register
#define READ_CONFIG_BYTE()      (*(volatile uint8_t *)CONFIG_BYTE_ADDR)
#define READ_OPTBYTE0()         (*(volatile uint8_t *)OPTBYTE0_ADDR)
#define READ_OPTBYTE0_COMP()    (*(volatile uint8_t *)OPTBYTE0_COMP_ADDR)
#define READ_OPTBYTE1()         (*(volatile uint8_t *)OPTBYTE1_ADDR)
#define READ_OPTBYTE1_COMP()    (*(volatile uint8_t *)OPTBYTE1_COMP_ADDR)
#define WRITE_CONFIG_BYTE(val)  ((*(volatile uint8_t *)CONFIG_BYTE_ADDR) = (val))
#define WRITE_OPTBYTE0(val)     ((*(volatile uint8_t *)OPTBYTE0_ADDR) = (val))
#define WRITE_OPTBYTE1(val)     ((*(volatile uint8_t *)OPTBYTE1_ADDR) = (val))

/*============ Local Variables ============*/

/*============ Exported Variables ============*/

/*============ Local Function Prototypes ============*/
void    unlock_memory(void);
void    lock_memory(void);
void    erase_option_byte(uint8_t Erasebyte);
void    write_to_option_byte(uint8_t Data, uint8_t OptionByte);
uint8_t check_boot_bits(void);

/*============ Local Functions ============*/
/* Allows user to access  */
void unlock_memory(void)
{
    while(READ_BIT(FLASH->SR, FLASH_SR_BSY != RESET));

    //need to unlock both the flash memory and option bytes (in that order)
    if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET) // check if flash access if already unlocked
        HAL_FLASH_Unlock();
    if(READ_BIT(FLASH->CR, FLASH_CR_OPTWRE) == RESET) // check if option byte access is already unlocked
        HAL_FLASH_OB_Unlock();
}

/*-----------------------------------------------------------*/

void lock_memory(void)
{
    // lock option byte, then flash access
    CLEAR_BIT(FLASH->CR, FLASH_CR_OPTWRE);
    SET_BIT(FLASH->CR, FLASH_CR_LOCK);
}

/*-----------------------------------------------------------*/
/*
 * @param Erasebyte - the byte that we want to clear
 */
void erase_option_byte(uint8_t Erasebyte)
{
    uint8_t TempOB = 0x00;

    while(READ_BIT(FLASH->SR, FLASH_SR_BSY != RESET));  // wait for any flash operations to stop

    /* don't want to erase both at the same time!
     * save the OB we want to keep, wipe both, write the OB back again
     */
    switch(Erasebyte)
    {
        case BYTE0:
        {
            TempOB = READ_OPTBYTE1(); // store the other byte
            HAL_FLASHEx_OBErase();  // wipe both bytes
            while(READ_BIT(FLASH->SR, FLASH_SR_BSY != RESET)) // wait for any flash operations to stop
                ;
            write_to_option_byte(TempOB, BYTE1); // write back to the OB we want to keep
            break;
        }
        case BYTE1:
        {
            TempOB = READ_OPTBYTE0();
            HAL_FLASHEx_OBErase();
            while(READ_BIT(FLASH->SR, FLASH_SR_BSY != RESET)) // wait for any flash operations to stop
                ;
            write_to_option_byte(TempOB, BYTE0); // write back to the OB we want to keep
            break;
        }
    }
}

/*-----------------------------------------------------------*/

void write_to_option_byte(uint8_t Data, uint8_t OptionByte)
{
    FLASH_OBProgramInitTypeDef OBInit;

    unlock_memory();

    // read off what is currently stored in the user area
    HAL_FLASHEx_OBGetConfig(&OBInit);

    /* select which byte we want to write to */
    switch(OptionByte)
    {
        case BYTE0:
        {
            OBInit.DATAAddress = OB_DATA_ADDRESS_DATA0;
            OBInit.DATAData = Data;
            break;
        }
        case BYTE1:
        {
            OBInit.DATAAddress = OB_DATA_ADDRESS_DATA1;
            OBInit.DATAData = Data;
            break;
        }
    }

    /* erase the entire page of flash - always need to do this before writing */
    HAL_FLASHEx_OBErase();

    /*  write to option data */
    OBInit.OptionType = OPTIONBYTE_DATA;
    HAL_FLASHEx_OBProgram(&OBInit);

    /* write to user configuration */
    OBInit.OptionType = OPTIONBYTE_USER;
    HAL_FLASHEx_OBProgram(&OBInit);

    /* lock memory again to prevent any accidental writes! */
    lock_memory();
}

/*-----------------------------------------------------------*/

uint8_t check_boot_bits(void)
{
    uint8_t status = 0;
#if defined(STM32F042x6)
    FLASH_OBProgramInitTypeDef TempOBConfig;
    uint8_t tempOBData = 0;

    /* only want to change some bits so read option byte configs off, set bits and write back */
    HAL_FLASHEx_OBGetConfig(&TempOBConfig);

    // only wanting to do a write if we have a bit to set (shouldn't have to do this unless somethings gone wrong!)
    if(((TempOBConfig.USERConfig & OB_BOOT0_SET) == 0x08) || ((TempOBConfig.USERConfig & OB_BOOT1_SET) != 0x10))
    {
        // store the bridge mode stored so we don't wipe it!
        tempOBData = HAL_FLASHEx_OBGetUserData(OPTBYTE0_ADDR);
        TempOBConfig.DATAData = tempOBData;
        TempOBConfig.DATAAddress = OPTBYTE0_ADDR;

        // check state of nBOOT0
        if((TempOBConfig.USERConfig & OB_BOOT0_SET) == 0x08)
        {
            /* nBOOT0 = 0 */
            TempOBConfig.USERConfig &= ~OB_BOOT0_SET;
        }

        // check state of nBOOT1
        if((TempOBConfig.USERConfig & OB_BOOT1_SET) != 0x10)
        {
            /* nBOOT1 = 1 */
            TempOBConfig.USERConfig |= OB_BOOT1_SET;
        }

        unlock_memory();    /* allow user to write to flash memory */

        /* erase the page of flash */
        HAL_FLASHEx_OBErase();

        /* write back the option byte configs */
        TempOBConfig.OptionType = OPTIONBYTE_USER;
        HAL_FLASHEx_OBProgram(&TempOBConfig);

        /* write back the option data */
        TempOBConfig.OptionType = OPTIONBYTE_DATA;
        HAL_FLASHEx_OBProgram(&TempOBConfig);

        lock_memory();  /* lock memory again to prevent accidental writes */

        status = 1;
    }
#endif
    return status;
}

/*============ Exported Functions ============*/
void Store_BridgeMode_To_Flash(uint8_t BridgeMode_to_store)
{
    write_to_option_byte(BridgeMode_to_store, BYTE0);

    // need to reset the chip using the following command otherwise the data is only saved whilst the power is connected --> power cycle will wipe it!
    HAL_FLASH_OB_Launch();
}

/*-----------------------------------------------------------*/

/* This reads the first option byte stored in flash user area */
uint8_t GetDeviceModeFromFlash(void)
{
    uint8_t BridgeMode_temp    = 0;
    uint8_t BridgeMode_Complement = 0;

    BridgeMode_temp = READ_OPTBYTE0();
    BridgeMode_Complement = READ_OPTBYTE0_COMP();

    uint8_t BridgeMode_temp_comp = ~BridgeMode_temp;

    if(BridgeMode_temp_comp == BridgeMode_Complement)
    {
        // no flash errors
        // make sure the bridge is in a known mode, if not then put it into basic mode
        if((BridgeMode_temp != MODE_TBP_BASIC) && (BridgeMode_temp != MODE_ABSOLUTE_MOUSE) && (BridgeMode_temp != MODE_PARALLEL_DIGITIZER))
        {
            HAL_DeInit();
            Store_BridgeMode_To_Flash(MODE_PARALLEL_DIGITIZER);
        }
    }
    else
    {
        // discrepancy detected, write the default mode back to flash
        HAL_DeInit();
        Store_BridgeMode_To_Flash(MODE_PARALLEL_DIGITIZER);
    }

    return BridgeMode_temp;
}

/*-----------------------------------------------------------*/

/* Reads and returns the value of the USER boot config */
void check_boot_config(void)
{
    uint8_t restart_required = 0;

    // check if bits are correct --> shouldn't change at all!
    restart_required += check_boot_bits();

    // check if we've just come out of bootloader mode
    restart_required += check_boot_sel();

    // we've changed something in flash --> do a reboot
    if(restart_required >= 1)
    {
        HAL_FLASH_OB_Launch();
    }
}

/*-----------------------------------------------------------*/

/* Reads and returns the value of BOOT_SEL config */
uint8_t check_boot_sel(void)
{
    uint8_t status = 0;
#if defined(STM32F042x6)
    FLASH_OBProgramInitTypeDef TempOBConfig;

    HAL_FLASHEx_OBGetConfig(&TempOBConfig);

    if((TempOBConfig.USERConfig & OB_BOOT_SEL_SET) == 0x80)
    {
        /* BOOT_SEL = 1 --> carry on as normal */
        status = 0;
    }
    else
    {
        /* BOOT_SEL = 0 --> need to write to BOOT_SEL and do a chip reset */
        write_boot_sel(1);
        status = 1;
    }
#endif

    return status;
}

/*-----------------------------------------------------------*/

/* Writes a value to the BOOT_SEL bit of the user data area */
void write_boot_sel(uint8_t boot_bit)
{
#if defined(STM32F042x6)
    FLASH_OBProgramInitTypeDef TempOBConfig;
    uint8_t tempOBData = 0;

    /* only want to change BOOT_SEL so read option byte configs off, set bit and write back */
    HAL_FLASHEx_OBGetConfig(&TempOBConfig);
    tempOBData = HAL_FLASHEx_OBGetUserData((uint32_t)OPTBYTE0_ADDR);

    // store a temporary copy of the bridge mode so we don't wipe it!
    TempOBConfig.DATAData = tempOBData;
    TempOBConfig.DATAAddress = (uint32_t)OPTBYTE0_ADDR;

    /* BOOT_SEL = boot_bit */
    if(boot_bit == 0)    /* BOOT_SEL = 0 */
    {
        TempOBConfig.USERConfig &= ~OB_BOOT_SEL_SET;
    }
    else if(boot_bit == 1)   /* BOOT_SEL = 1 */
    {
        TempOBConfig.USERConfig |= OB_BOOT_SEL_SET;
    }

    unlock_memory();    /* allow user to write to flash memory */

    /* erase the page of flash */
    HAL_FLASHEx_OBErase();

    /* write back the option byte configs with changes */
    TempOBConfig.OptionType = OPTIONBYTE_USER;
    HAL_FLASHEx_OBProgram(&TempOBConfig);

    /* write back the bridge mode */
    TempOBConfig.OptionType = OPTIONBYTE_DATA;
    HAL_FLASHEx_OBProgram(&TempOBConfig);

    lock_memory();  /* lock memory again to prevent accidental writes */
#endif
}

/*-----------------------------------------------------------*/

void StartBootloader(void)
{
    Device_DeInit();

    write_boot_sel(0);

#if defined(STM32F042x6)
    /* The STM32F042 has:
     * 6k SRAM in address range: 0x2000 0000 - 0x2000 17FF
     */
    *((unsigned long *)0x200017F0) = 0xDEADBEEF;

    // Reset the processor --> this call makes sure the option bytes are reloaded correctly at reset
    HAL_FLASH_OB_Launch();
#endif

#if defined(STM32F070xB) || defined(STM32F072xB)
    /* The STM32F070xB and STM32F072xB have:
     * 16k SRAM in address range: 0x2000 0000 - 0x2000 3FFF
     */
    *((unsigned long *)0x20003FF0) = 0xDEADBEEF;

    NVIC_SystemReset();
#endif

    // We then do a check in SystemInit() at startup to see if this 'magic number' has been set
}

/*-----------------------------------------------------------*/
