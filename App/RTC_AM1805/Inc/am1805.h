//*****************************************************************************
//
//! @file am1805.h
//!
//! @brief Driver to interface with the AM1805 RTC.
//!
//! These functions implement the AM1805 support routines for use on STM32.
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2017, Ambiq Micro
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision 1.2.9 of the AmbiqSuite Development Package.
//
//*****************************************************************************

#ifndef AM1805_H
#define AM1805_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "i2c.h"

//*****************************************************************************
//
// Register Defines.
//
//*****************************************************************************
#define AM1805_HUNDREDTHS                 0x00
#define AM1805_YEARS                      0x06
#define AM1805_ALARM_HUNDRS               0x08
#define AM1805_STATUS                     0x0F
#define AM1805_CONTROL_1                  0x10
#define AM1805_CONTROL_2                  0x11
#define AM1805_INT_MASK                   0x12
#define AM1805_SQW                        0x13
#define AM1805_CAL_XT                     0x14
#define AM1805_CAL_RC_HI                  0x15
#define AM1805_CAL_RC_LOW                 0x16
#define AM1805_SLEEP_CTRL                 0x17
#define AM1805_TIMER_CTRL                 0x18
#define AM1805_TIMER                      0x19
#define AM1805_TIMER_INITIAL              0x1A
#define AM1805_WDT                        0x1B
#define AM1805_OSC_CONTROL                0x1C
#define AM1805_OSC_STATUS                 0x1D
#define AM1805_CONFIG_KEY                 0x1F
#define AM1805_ACAL_FLT                   0x26
  
#define AM1805_OUTPUT_CTRL                0x30
  
#define AM1805_EXTENDED_ADDR              0x3F
#define AM1805_RAM_START                  0x40

// Keys.
#define AM1805_CONFIG_KEY_VAL             0xA1
#define AM1805_CONFIG_KEY_3C              0x3C
#define AM1805_CONFIG_KEY_9D              0x9D

// Modes
#define AM1805_12HR_MODE                  0x01
#define AM1805_24HR_MODE                  0x02

//*****************************************************************************
//
// Device structure used for communication.
//
//*****************************************************************************

// defines slave address for am1805
#define AM1805_ADDRESS                    0xD2

// defines to be used in am1805_t structure.
#define AM1805_MODE_SPI                   true
#define AM1805_MODE_I2C                   false


//*****************************************************************************
//
// Structure used for time keeping.
//
//*****************************************************************************
typedef struct
{
    uint8_t ui8Hundredth;
    uint8_t ui8Second;
    uint8_t ui8Minute;
    uint8_t ui8Hour;
    uint8_t ui8Date;
    uint8_t ui8Weekday;
    uint8_t ui8Month;
    uint8_t ui8Year;
    uint8_t ui8Century;
    uint8_t ui8Mode;
} am1805_time_t;

extern am1805_time_t g_psTimeRegs;
extern I2C_HandleTypeDef hi2c1;

//*****************************************************************************
//
// External function definitions
//
//*****************************************************************************

void am1805_reset(void);
void am1805_time_get(void);
void am1805_time_set(uint8_t ui8Protect);
//
//extern void am1805_cal_set(am1805_t *psDevice,
//                                      uint8_t ui8Mode, int32_t iAdjust);
//
//extern void am1805_alarm_set(am1805_t *psDevice,
//                                        uint8_t ui8Repeat, uint8_t ui8IntMode,
//                                        uint8_t ui8Pin);
//
//extern void am1805_countdown_set(am1805_t *psDevice,
//                                            uint8_t ui8Range, int32_t iPeriod,
//                                            uint8_t ui8Repeat, uint8_t ui8Pin);
//
//extern uint32_t am1805_osc_sel(am1805_t *psDevice,
//                                          uint8_t ui8OSC);
//
//extern void am1805_sqw_set(am1805_t *psDevice,
//                                      uint8_t ui8SQFS, uint8_t ui8Pin);
//
//extern uint32_t am1805_sleep_set(am1805_t *psDevice,
//                                           uint8_t ui8Timeout, uint8_t ui8Mode);
//
//extern void am1805_watchdog_set(am1805_t *psDevice,
//                                           uint32_t ui8Period, uint8_t ui8Pin);
//
//extern void am1805_autocal_set(am1805_t *psDevice,
//                                          uint8_t ui8Period);
//
//extern uint8_t am1805_ext_address_get(am1805_t *psDevice,
//                                                 uint8_t ui8Address);
//
//extern uint8_t am1805_ram_read(am1805_t *psDevice,
//                                          uint8_t ui8Address);
//
//extern void am1805_ram_write(am1805_t *psDevice,
//                                        uint8_t ui8Address, uint8_t ui8Data);

#ifdef __cplusplus
}
#endif

#endif // AM1805_H
