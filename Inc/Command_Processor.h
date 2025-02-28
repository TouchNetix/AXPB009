/*******************************************************************************
* @file           : COmmand_Processor.h
* @author         : James Cameron (TouchNetix)
* @date           : 24 Sep 2020
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

#ifndef COMMAND_PROCESSOR_H_
#define COMMAND_PROCESSOR_H_

/*============ Includes ============*/
#include "stm32f0xx.h"
#include <stdbool.h>

/*============ Exported Variables ============*/
extern  bool    boGenericTBPResponseWaiting;
extern  bool    boPressTBPResponseWaiting;
extern  uint8_t *pTBPCommandReport;

/*============ Exported Functions ============*/
void ProcessTBPCommand();

#endif /* COMMAND_PROCESSOR_H_ */
