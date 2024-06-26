//*****************************************************************************
//
//! @file am1805.h
//!
//! @brief Driver to interface with the AM1805 RTC.
//!
//! These functions implement the AM1805 support routines adjusted for use on
//! STM32 by Dekimo.
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
#define AM1805_SECONDS                    0x01
#define AM1805_MINUTES                    0x02
#define AM1805_HOURS                      0x03
#define AM1805_DATE                       0x04
#define AM1805_MONTH                      0x05
#define AM1805_YEARS                      0x06
#define AM1805_WEEKDAY                    0x07
#define AM1805_ALARM_HUNDRS               0x08
#define AM1805_ALARM_SECONDS              0x09
#define AM1805_ALARM_MINUTES              0x0A
#define AM1805_ALARM_HOURS                0x0B
#define AM1805_ALARM_DATE                 0x0C
#define AM1805_ALARM_MONTH                0x0D
#define AM1805_ALARM_WEEKDAY              0x0E
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

// Century
#define AM1805_CENTURY_19xx               0x00
#define AM1805_CENTURY_20xx               0x01
#define AM1805_CENTURY_21xx               0x00


//Configuration register : status (0x0F)
#define AM1805_mask_EX1                   0x01
#define AM1805_mask_EX2                   0x02
#define AM1805_mask_ALM                   0x04
#define AM1805_mask_TIM                   0x08
#define AM1805_mask_BL                    0x10
#define AM1805_mask_WDT                   0x20
#define AM1805_mask_BAT                   0x40
#define AM1805_mask_CB                    0x80

//Countdown
#define CNTDWN_RANGE_USEC                   0x00
#define CNTDWN_RANGE_SEC                    0x01

#define CNTDWN_IRQ_SINGLE_LEVEL             0x00 //0 => generate a single level interrupt
#define CNTDWN_IRQ_REP_PULSED_XT_1_4096S    0x01 //1 => generate a repeated pulsed interrupt, 1/4096 s (XT mode), 1/128 s(RC mode) (ui8Range must be 0)
#define CNTDWN_IRQ_REP_PULSED_RC_1_128S     0x01 //1 => generate a repeated pulsed interrupt, 1/4096 s (XT mode), 1/128 s(RC mode) (ui8Range must be 0)
#define CNTDWN_IRQ_SINGLE_PULSED_XT_1_4096S 0x02 //2 => generate a single pulsed interrupt, 1/4096 s (XT mode), 1/128 s (RC mode) (ui8Range must be 0)
#define CNTDWN_IRQ_SINGLE_PULSED_RC_1_128S  0x02 //2 => generate a single pulsed interrupt, 1/4096 s (XT mode), 1/128 s (RC mode) (ui8Range must be 0)
#define CNTDWN_IRQ_REP_PULSED_1_128S        0x03 //3 => generate a repeated pulsed interrupt, 1/128 s (ui8Range must be 0)
#define CNTDWN_IRQ_SINGLE_PULSED_1_128S     0x04 //4 => generate a single pulsed interrupt, 1/128 s (ui8Range must be 0)
#define CNTDWN_IRQ_REP_PULSED_1_64s         0x05 //5 => generate a repeated pulsed interrupt, 1/64 s (ui8Range must be 1)
#define CNTDWN_IRQ_SINGLE_PULSED_1_64S      0x06 //6 => generate a single pulsed interrupt, 1/64 s (ui8Range must be 1)

#define CNTDOWN_DISABLE_CNT_DOWN_TMR            0x00 //0 => disable the countdown timer
#define CNTDOWN_PIN_nTIRQ_LOW                   0x01 //1 => generate an interrupt on nTIRQ only, asserted low
#define CNTDOWN_PIN_FOUT_AND_nTIRQ_LOW          0x02 //2 => generate an interrupt on FOUT/nIRQ and nTIRQ, both asserted low
#define CNTDOWN_PIN_PSW_AND_nTIRQ_LOW           0x03 //3 => generate an interrupt on PSW/nIRQ2 and nTIRQ, both asserted low
#define CNTDOWN_PIN_CLKOUT_AND_nTIRQ_LOW        0x04 //4 => generate an interrupt on CLKOUT/nIRQ3 and nTIRQ, both asserted low
#define CNTDOWN_PIN_CLKOUT_HIGH_AND_nTIRQ_LOW   0x05 //5 => generate an interrupt on CLKOUT/nIRQ3 (asserted high) and nTIRQ (asserted low)

#define SLEEP_MODE_nRST_LOW                 0x00//0 => nRST is pulled low in sleep mode
#define SLEEP_MODE_PSW_LOW                  0x01//1 => PSW/nIRQ2 is pulled high on a sleep
#define SLEEP_MODE_nRST_LOW_AND_PSW_HIGH    0x02//2 => nRST pulled low and PSW/nIRQ2 pulled high on sleep

#define SLEEP_RETURN_ACCEPTED               0x00 //0 => sleep request accepted, sleep mode will be initiated in ui8Timeout seconds
#define SLEEP_RETURN_ACCEPTED_AFTER_RETRY   0xAA //170 => sleep request accepted after retry, sleep mode will be initiated in ui8Timeout seconds
#define SLEEP_RETURN_ILLEGAL_INPUT          0x01 //1 => illegal input values
#define SLEEP_RETURN_DECLINED_ACTIVE_IRQ    0x02 //2 => sleep request declined, interrupt is currently pending
#define SLEEP_RETURN_DECLINED_NO_SLEEP_IRQ  0x03 //3 => sleep request declined, no sleep trigger interrupt enabled

//alarm
#define ALARM_INTERVAL_DISABLE        0x00  //0 => disable alarm
#define ALARM_INTERVAL_YEAR           0x01  //1 => once per year
#define ALARM_INTERVAL_MONTH          0x02  //2 => once per month
#define ALARM_INTERVAL_WEEK           0x03  //3 => once per week
#define ALARM_INTERVAL_DAY            0x04  //4 => once per day
#define ALARM_INTERVAL_HOUR           0x05  //5 => once per hour
#define ALARM_INTERVAL_MINUTE         0x06  //6 => once per minute
#define ALARM_INTERVAL_SECOND         0x07  //7 => once per second
#define ALARM_INTERVAL_10TH_SECOND    0x08  //8 => once per 10th of a second
#define ALARM_INTERVAL_100TH_SECOND   0x09  //9 => once per 100th of a second

#define ALARM_IRQ_LEVEL               0x00  //0 => level interrupt
#define ALARM_IRQ_PULSE_XT_1_8192S    0x01  //1 => pulse of 1/8192s (XT)
#define ALARM_IRQ_PULSE_RC_1_128S     0x01  //1 => pulse of 1/128 s (RC)
#define ALARM_IRQ_PULSE_1_64S         0x02  //2 => pulse of 1/64 s
#define ALARM_IRQ_PULSE_1_4S          0x03  //3 => pulse of 1/4 s

#define ALARM_PIN_INTERNAL_FLAG       0x00 //0 => internal flag only
#define ALARM_PIN_nIRQ                0x01 //1 => FOUT/nIRQ
#define ALARM_PIN_PSW                 0x02 //2 => PSW/nIRQ2


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

// Define the i2c handle to use for the AM1805
#define I2CHANDLE                         hi2c1
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
  uint8_t ui8Month;
  uint8_t ui8Year;
  uint8_t ui8Century; // A 0 assumes the century is 19xx or 21xx, and a 1 assumes it is 20xx for leap year calculations.
  uint8_t ui8Weekday;
  uint8_t ui8Mode;
} am1805_time_t;

//*****************************************************************************
//
// External function definitions
//
//*****************************************************************************

void am1805_reset(void);
void am1805_time_get(am1805_time_t *time);
void am1805_time_set(am1805_time_t time,uint8_t ui8Protect);
void am1805_cal_set(uint8_t ui8Mode, int32_t iAdjust);
void am1805_alarm_set(am1805_time_t alarm, uint8_t ui8Repeat, uint8_t ui8IntMode, uint8_t ui8Pin);
void am1805_countdown_set(uint8_t ui8Range, int32_t iPeriod, uint8_t ui8Repeat, uint8_t ui8Pin);
uint32_t am1805_osc_sel(uint8_t ui8OSC);
void am1805_sqw_set(uint8_t ui8SQFS, uint8_t ui8Pin);
uint32_t am1805_sleep_set(uint8_t ui8Timeout, uint8_t ui8Mode);
void am1805_watchdog_set(uint32_t ui8Period, uint8_t ui8Pin);
void am1805_autocal_set(uint8_t ui8Period);
uint8_t am1805_ext_address_get(uint8_t ui8Address);
uint8_t am1805_ram_read(uint8_t ui8Address);
void am1805_ram_write(uint8_t ui8Address, uint8_t ui8Data);
void am1805_enable_wdi_ex1_interrupt(void);
void am1805_disable_wdi_ex1_interrupt(void);
void am1805_enable_wdi_ex2_interrupt(void);
void am1805_disable_wdi_ex2_interrupt(void);
void am1805_ex2p_rising_edge_interrupt(void);
void am1805_ex2p_faling_edge_interrupt(void);
void am1805_enable_pwgt(void);
void am1805_disable_pwgt(void);
uint8_t am1805_get_status(uint8_t clear);
bool am1805_get_wdin_status(void);
bool am1805_get_exin_status(void);
uint8_t am1805_get_interrupt_mask(void);
uint8_t am1805_get_sleep_control(void);
uint8_t am1805_get_osc_control(void);
uint8_t am1805_get_output_control(void);
uint8_t am1805_get_control1(void);
uint8_t am1805_get_control2(void);
void am1805_set_output_control(void);
void am1805_clear_output_control(void);
am1805_time_t am1805_read_current_alarm(void);

#ifdef __cplusplus
}
#endif

#endif // AM1805_H
