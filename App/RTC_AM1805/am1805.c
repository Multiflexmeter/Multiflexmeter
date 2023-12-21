//*****************************************************************************
//
//! @file am1805.c
//!
//! @brief Driver to interface with the AM1805 RTC.
//!
//! These functions implement the AM1805 support routines adjusted for use on
//! STM32 by Dekimo.
//
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
//
//*****************************************************************************

#include <stdbool.h>
#include "am1805.h"


//*****************************************************************************
//
// Converts a Binary Coded Decimal (BCD) byte to its Decimal form.
//
//*****************************************************************************
static uint8_t bcd_to_dec(uint8_t ui8BCDByte)
{
  return (((ui8BCDByte & 0xF0) >> 4) * 10) + (ui8BCDByte & 0x0F);
}

//*****************************************************************************
//
// Converts a Decimal byte to its Binary Coded Decimal (BCD) form.
//
//*****************************************************************************
static uint8_t dec_to_bcd(uint8_t ui8DecimalByte)
{
  return (((ui8DecimalByte / 10) << 4) | (ui8DecimalByte % 10));
}

/**
 * @brief Writes an internal register in the AM1805.
 *
 * @param ui8Register is the register to write to.
 * @param ui8Value is the value to write to the register.
 *
 * This function performs a write to an AM1805 register over the I2C bus.
 *
 * @return None
 */
static void am1805_reg_write(const uint8_t ui8Register, const uint8_t ui8Value)
{
  uint8_t ui8Data = ui8Value;

  HAL_I2C_Mem_Write(&I2CHANDLE, AM1805_ADDRESS, ui8Register, 1, &ui8Data, 1, 100);
}

/**
 * @brief Reads an internal register in the AM1805.
 *
 * @param ui8Register is the address of the register to read.
 *
 * This function performs a read to an AM1805 register over the I2C bus.
 *
 * @return Data from the register
 */
static uint8_t am1805_reg_read(const uint8_t ui8Register)
{
  uint8_t data;

  // Read a single i2c register
  HAL_I2C_Mem_Read(&I2CHANDLE, AM1805_ADDRESS, ui8Register, 1, &data, 1, 100);

  return data;
}

/**
 * @brief Set one or more bits.
 *
 * @param ui8Address - RTC address.
 * @param ui8Mask - Bits to set.
 *
 * This function sets one or more bits in the selected register, selected by
 * 1's in the mask.
 *
 * @return None
 */
static void am1805_reg_set(uint8_t ui8Address, uint8_t ui8Mask)
{
  uint8_t ui8Temp;

  ui8Temp = am1805_reg_read(ui8Address);
  ui8Temp |= ui8Mask;
  am1805_reg_write(ui8Address, ui8Temp);
}


/**
 * @brief Clear one or more bits.
 *
 * @param ui8Address - RTC address.
 * @param ui8Mask - Bits to clear.
 *
 * This function clears one or more bits in the selected register, selected by
 * 1's in the mask.
 *
 * @return None
 */
static void am1805_reg_clear(uint8_t ui8Address, uint8_t ui8Mask)
{
  uint8_t ui8Temp;

  ui8Temp = am1805_reg_read(ui8Address);
  ui8Temp &= ~ui8Mask;
  am1805_reg_write(ui8Address, ui8Temp);
}

/**
 * @brief Reads a block of internal registers in the AM1805.
 *
 * @param ui8StartRegister is the address of the first register to read.
 * @param pui8Values is the byte-packed array where the read data will go.
 * @param ui8NumBytes is the total number of registers to read.
 *
 * This function performs a read to a block of AM1805 registers over the
 * I2C bus.
 *
 * @return None
 */
static void am1805_reg_block_read(const uint8_t ui8StartRegister, uint8_t *pui8Values, const uint8_t ui8NumBytes)
{
  uint8_t ui8Offset = ui8StartRegister;

  HAL_I2C_Mem_Read(&I2CHANDLE, AM1805_ADDRESS, ui8Offset, 1, pui8Values, ui8NumBytes, 100);

}

/**
 * @brief Writes a block of internal registers in the AM1805.
 *
 * @param ui8StartRegister is the address of the first register to write.
 * @param pui8Values is the byte-packed array of data to write.
 * @param ui8NumBytes is the total number of registers to write.
 *
 * This function performs a write to a block of AM1805 registers over the
 * I2C bus.
 *
 * @return None
 */
static void am1805_reg_block_write(const uint8_t ui8StartRegister, uint8_t *pui8Values, const uint8_t ui8NumBytes)
{
  uint8_t ui8Offset = ui8StartRegister;

  HAL_I2C_Mem_Write(&I2CHANDLE, AM1805_ADDRESS, ui8Offset, 1, pui8Values, ui8NumBytes, 100);
}

/**
 * @brief Reset the AM1805.
 *
 * This function performs a reset to AM1805.
 *
 * return None
 */
void am1805_reset(void)
{
  // Software reset
  am1805_reg_write(AM1805_CONFIG_KEY, 0x3C);
}

/**
 * @brief Get the time.
 *
 * @param time is a pointer to store the time to.
 *
 * This function loads the am1805_time_t structure with the time from the
 * AM1805.
 *
 * @return None.
 */
void am1805_time_get(am1805_time_t *time)
{
  uint8_t psTempBuff[8];

  // Read the counters.
  am1805_reg_block_read(AM1805_HUNDREDTHS, psTempBuff, 8);

  time->ui8Hundredth = bcd_to_dec(psTempBuff[0]);
  time->ui8Second = bcd_to_dec(psTempBuff[1]);
  time->ui8Minute = bcd_to_dec(psTempBuff[2]);
  time->ui8Hour = psTempBuff[3];
  time->ui8Date = bcd_to_dec(psTempBuff[4]);
  time->ui8Month = bcd_to_dec(psTempBuff[5]);
  time->ui8Year = bcd_to_dec(psTempBuff[6]);
  time->ui8Weekday = bcd_to_dec(psTempBuff[7]);

  // Get the current hours format mode 12:24.
  psTempBuff[0] = am1805_reg_read(AM1805_CONTROL_1);
  if ((psTempBuff[0] & 0x40) == 0)
  {
    // 24-hour mode.
    time->ui8Mode = 2;
    time->ui8Hour = time->ui8Hour & 0x3F;
  }
  else
  {
    // 12-hour mode.  Get PM:AM.
    time->ui8Mode = (time->ui8Hour & 0x20) ? 1 : 0;
    time->ui8Hour &= 0x1F;
  }

  time->ui8Hour = bcd_to_dec(time->ui8Hour);

  // Get the century bit.
  psTempBuff[0] = am1805_reg_read(AM1805_STATUS);
  time->ui8Century = (psTempBuff[0] & 0x80) ? 1 : 0;
}

/**
 * @brief Set the time in the counters.
 *
 * @param time is the time to sync the RTC with.
 * @param ui8Protect:  0 => leave counters writable
 *                     1 => leave counters unwritable
 *
 * This function loads the AM1805 counter registers with the provided
 * am1805_time_t structure values.
 *
 * @return None
 */
void am1805_time_set(am1805_time_t time, uint8_t ui8Protect)
{
  am1805_time_t setTime;
  uint8_t psTempBuff[8];

  // Convert decimal to binary-coded decimal.
  setTime.ui8Hundredth = dec_to_bcd(time.ui8Hundredth);
  setTime.ui8Second = dec_to_bcd(time.ui8Second);
  setTime.ui8Minute = dec_to_bcd(time.ui8Minute);
  setTime.ui8Hour = dec_to_bcd(time.ui8Hour);
  setTime.ui8Date = dec_to_bcd(time.ui8Date);
  setTime.ui8Weekday = dec_to_bcd(time.ui8Weekday);
  setTime.ui8Month = dec_to_bcd(time.ui8Month);
  setTime.ui8Year = dec_to_bcd(time.ui8Year);

  // Determine whether 12 or 24-hour timekeeping mode is being used and set
  // the 1224 bit appropriately.
  // 24-hour day.
  if (time.ui8Mode == AM1805_24HR_MODE)
  {
    am1805_reg_clear(AM1805_CONTROL_1, 0x40);
  }

  // 12-hour day PM.
  else if (time.ui8Mode == AM1805_12HR_MODE)
  {
    // Set AM/PM.
    time.ui8Hour |= 0x20;
    am1805_reg_set(AM1805_CONTROL_1, 0x40);
  }

  // 12-hour day AM.
  else
  {
    am1805_reg_set(AM1805_CONTROL_1, 0x40);
  }

  // Set the WRTC bit to enable counter writes.
  am1805_reg_set(AM1805_CONTROL_1, 0x01);

  // Set the correct century.
  if (time.ui8Century == 0)
  {
    am1805_reg_clear(AM1805_STATUS, AM1805_mask_CB);
  }
  else
  {
    am1805_reg_set(AM1805_STATUS, AM1805_mask_CB);
  }

  // Write all of the time counters.
  psTempBuff[0] = setTime.ui8Hundredth;
  psTempBuff[1] = setTime.ui8Second;
  psTempBuff[2] = setTime.ui8Minute;
  psTempBuff[3] = setTime.ui8Hour;
  psTempBuff[4] = setTime.ui8Date;
  psTempBuff[5] = setTime.ui8Month;
  psTempBuff[6] = setTime.ui8Year;
  psTempBuff[7] = setTime.ui8Weekday;

  // Write the values to the AM1805.
  am1805_reg_block_write(AM1805_HUNDREDTHS, psTempBuff, 8);

  // Load the final value of the WRTC bit based on the value of ui8Protect.
  // Clear the WRTC bit and the STOP bit.
  // Invert the protect bit and update WRTC.
  psTempBuff[0] = am1805_reg_read(AM1805_CONTROL_1);
  psTempBuff[0] &= 0x7E;
  psTempBuff[0] |= (0x01 & (~ui8Protect));
  am1805_reg_write(AM1805_CONTROL_1, psTempBuff[0]);
}

/**
 * @brief Set the calibration.
 *
 * @param  ui8Mode:
 *        0 => calibrate the XT oscillator
 *        1 => calibrate the RC oscillator
 * @param   iAdjust: Adjustment in ppm.
 *
 *  Adjustment limits are:
 *          ui8Mode = 0 => (-610 to +242)
 *          ui8Mode = 1 => (-65536 to +65520)
 *          An iAdjust value of zero resets the selected oscillator calibration
 *          value to 0.
 *
 * This function calibrates the selected oscillator by the provided adjust parameter.
 *
 * @return None
 */
void am1805_cal_set(uint8_t ui8Mode, int32_t iAdjust)
{
    int32_t iAdjint;
    uint8_t ui8Adjreg;
    uint8_t ui8Adjregu;
    uint8_t ui8XTcal;

    // Calculate current calibration value:
    //    adjval = (double)iAdjust;
    //    iAdjint = (int16_t)round(adjval*1000/1907)
    if (iAdjust < 0 )
    {
        iAdjint = ((iAdjust)*1000 - 953);
    }
    else
    {
        iAdjint = ((iAdjust)*1000 + 953);
    }
    iAdjint = iAdjint / 1907;

    if (ui8Mode == 0)
    {
        // XT Adjust
        if (iAdjint > 63 )
        {
            // 64 to 127.
            // CMDX = 1.
            ui8XTcal = 0;
            ui8Adjreg = ((iAdjint >> 1) & 0x3F) | 0x80;
        }
        else if (iAdjint > -65)
        {

            // -64 to 63.
            // CMDX = 0.
            ui8XTcal = 0;
            ui8Adjreg = (iAdjint & 0x7F);
        }
        else if (iAdjint > -129)
        {
            // -128 to -65.
            // CMDX = 0.
            ui8XTcal = 1;
            ui8Adjreg = ((iAdjint + 64) & 0x7F);
        }
        else if (iAdjint > -193)
        {
            // -192 to -129.
            // CMDX = 0.
            ui8XTcal = 2;
            ui8Adjreg = ((iAdjint + 128) & 0x7F);
        }
        else if (iAdjint > -257)
        {
            // -256 to -193.
            // CMDX = 0.
            ui8XTcal = 3;
            ui8Adjreg = ((iAdjint + 192) & 0x7F);
        }
        else
        {
            // -320 to -257.
            // CMDX = 1.
            ui8XTcal = 3;
            ui8Adjreg = ((iAdjint + 192) >> 1) & 0xFF;
        }

        // Load the CALX register.
        am1805_reg_write(AM1805_CAL_XT, ui8Adjreg);

        // Mask ui8XTcal.
        ui8Adjreg = am1805_reg_read(AM1805_OSC_STATUS) & 0x3F;

        // Add ui8XTcal field.
        ui8Adjreg = ui8Adjreg | (ui8XTcal << 6);

        // Write it back.
        am1805_reg_write(AM1805_OSC_STATUS, ui8Adjreg);
    }
    else
    {
        // RC Adjust.
        if (iAdjint > 32767 )
        {
            // 32768 to 65535.
            // Lower 8 bits.
            // CMDR = 3.
            ui8Adjreg = ((iAdjint >> 3) & 0xFF);
            ui8Adjregu = ((iAdjint >> 11) | 0xC0);
        }
        else if (iAdjint > 16383 )
        {
            // 16384 to 32767.
            // Lower 8 bits.
            // CMDR = 2.
            ui8Adjreg = ((iAdjint >> 2) & 0xFF);
            ui8Adjregu = ((iAdjint >> 10) | 0x80);
        }
        else if (iAdjint > 8191 )
        {
            // 8192 to 16383.
            // Lower 8 bits.
            // CMDR = 2.
            ui8Adjreg = ((iAdjint >> 1) & 0xFF);
            ui8Adjregu = ((iAdjint >> 9) | 0x40);
        }
        else if (iAdjint >= 0 )
        {
            // 0 to 1023.
            // Lower 8 bits.
            // CMDR = 0.
            ui8Adjreg = ((iAdjint) & 0xFF);
            ui8Adjregu = (iAdjint >> 8);
        }
        else if (iAdjint > -8193 )
        {
            // -8192 to -1.
            // Lower 8 bits.
            // CMDR = 0.
            ui8Adjreg = ((iAdjint) & 0xFF);
            ui8Adjregu = (iAdjint >> 8) & 0x3F;
        }
        else if (iAdjint > -16385 )
        {
            // -16384 to -8193.
            // Lower 8 bits.
            // CMDR = 1.
            ui8Adjreg = ((iAdjint >> 1) & 0xFF);
            ui8Adjregu = (iAdjint >> 9) & 0x7F;
        }
        else if (iAdjint > -32769 )
        {
            // -32768 to -16385.
            // Lower 8 bits.
            // CMDR = 2.
            ui8Adjreg = ((iAdjint >> 2) & 0xFF);
            ui8Adjregu = (iAdjint >> 10) & 0xBF;
        }
        else
        {
            // -65536 to -32769.
            // Lower 8 bits.
            // CMDR = 3.
            ui8Adjreg = ((iAdjint >> 3) & 0xFF);
            ui8Adjregu = (iAdjint >> 11) & 0xFF;
        }

        // Load the CALRU register.
        am1805_reg_write(AM1805_CAL_RC_HI, ui8Adjregu);

        // Load the CALRL register.
        am1805_reg_write(AM1805_CAL_RC_LOW, ui8Adjreg);
    }
}

/**
 * @brief Set the alarm value.
 *
 * @param alarm is the time to set the alarm to.
 * @param ui8Repeat - the alarm repeat interval
 *         0 => disable alarm
 *         1 => once per year
 *         2 => once per month
 *         3 => once per week
 *         4 => once per day
 *         5 => once per hour
 *         6 => once per minute
 *         7 => once per second
 *         8 => once per 10th of a second
 *         9 => once per 100th of a second
 *         NOTE: year and century are not used
 *         OTE: mode must match current 12/24 selection
 * @param ui8IntMode - define the interrupt mode
 *         0 => level interrupt
 *         1 => pulse of 1/8192s (XT) or 1/128 s (RC)
 *         2 => pulse of 1/64 s
 *         3 => pulse of 1/4 s
 * @param ui8Pin - pin on which to generate the interrupt
 *         0 => internal flag only
 *         1 => FOUT/nIRQ
 *         2 => PSW/nIRQ2
 *
 * This function sets the alarm value and configures the correct pin (if necessary).
 *
 * @return None
 */
void am1805_alarm_set(am1805_time_t alarm, uint8_t ui8Repeat, uint8_t ui8IntMode, uint8_t ui8Pin)
{
    volatile uint8_t ui8Temp;
    am1805_time_t setAlarm;
    uint8_t psTempBuff[8];

    // Convert decimal to binary-coded decimal.
    setAlarm.ui8Hundredth = dec_to_bcd(alarm.ui8Hundredth);
    setAlarm.ui8Second = dec_to_bcd(alarm.ui8Second);
    setAlarm.ui8Minute = dec_to_bcd(alarm.ui8Minute);
    setAlarm.ui8Hour = dec_to_bcd(alarm.ui8Hour);
    setAlarm.ui8Date = dec_to_bcd(alarm.ui8Date);
    setAlarm.ui8Weekday = dec_to_bcd(alarm.ui8Weekday);
    setAlarm.ui8Month = dec_to_bcd(alarm.ui8Month);

    // Determine whether a 12-hour or a 24-hour time keeping mode is being
    // used.
    if (alarm.ui8Mode == 1)
    {
        // A 12-hour day PM.
        // Set AM/PM.
        alarm.ui8Hour = alarm.ui8Hour | 0x20;
    }

    // Write all of the time counters.
    psTempBuff[0] = setAlarm.ui8Hundredth;
    psTempBuff[1] = setAlarm.ui8Second;
    psTempBuff[2] = setAlarm.ui8Minute;
    psTempBuff[3] = setAlarm.ui8Hour;
    psTempBuff[4] = setAlarm.ui8Date;
    psTempBuff[5] = setAlarm.ui8Month;
    psTempBuff[6] = setAlarm.ui8Weekday;

    // Clear the RPT field.
    // Clear the AIE bit IM field.
    // Clear the ALM flag.
    am1805_reg_clear(AM1805_TIMER_CTRL, 0x1C);
    am1805_reg_clear(AM1805_INT_MASK, 0x64);
    am1805_reg_clear(AM1805_STATUS, AM1805_mask_ALM);

    if (ui8Pin == 1)
    {
        // Interrupt on FOUT/nIRQ.
        // Get the Control2 Register.
        ui8Temp = am1805_reg_read(AM1805_CONTROL_2);

        // Extract the OUT1S field.
        ui8Temp = (ui8Temp & 0x03);

        // Not already selecting nIRQ.
        if (!ui8Temp)
        {
            // Set OUT1S to 3.
            am1805_reg_set(AM1805_CONTROL_2, 0x03);
        }
    }
    if (ui8Pin == 2)
    {
        // Interrupt on PSW/nIRQ2.
        // Get the Control2 Register.
        ui8Temp = am1805_reg_read(AM1805_CONTROL_2);

        // Extract the OUT2S field.
        ui8Temp &= 0x1C;

        // Not already selecting nIRQ.
        if (ui8Temp != 0)
        {
            // Clear OUT2S & Set OUT2S to 3.
            am1805_reg_clear(AM1805_CONTROL_2, 0x1C);
            am1805_reg_set(AM1805_CONTROL_2, 0x0C);
        }
    }

    if (ui8Repeat == 8)
    {
        // 10ths interrupt.
        // Select correct RPT value.
        psTempBuff[0] |= 0xF0;
        ui8Repeat = 7;
    }
    if (ui8Repeat == 9)
    {
        // 100ths interrupt.
        // Select correct RPT value.
        psTempBuff[0] = 0xFF;
        ui8Repeat = 7;
    }

    // Don't initiate if ui8Repeat = 0.
    if (ui8Repeat != 0)
    {
        // Set the RPT field to the value of ui8Repeat.
        ui8Temp = (ui8Repeat << 2);

        // Was previously cleared.
        // Set the alarm interrupt mode.
        // Execute the burst write.
        // Set the AIE bit.
        am1805_reg_set(AM1805_TIMER_CTRL, ui8Temp);
        am1805_reg_set(AM1805_INT_MASK, (ui8IntMode << 5));
        am1805_reg_block_write(AM1805_ALARM_HUNDRS, psTempBuff, 7);
        am1805_reg_set(AM1805_INT_MASK, 0x04);
    }
    else
    {
        // Set IM field to 0x3 (reset value) to minimize current draw.
        am1805_reg_set(AM1805_INT_MASK, 0x60);
    }
}

/**
 * @brief Configure and set the countdown.
 *
 * @param ui8Range:    0 => iPeriod in us
 *                     1 => iPeriod in seconds
 * @param iPeriod - the iPeriod of the countdown timer.
 * @param ui8Repeat - Configure the interrupt output type:
 *         0 => generate a single level interrupt
 *         1 => generate a repeated pulsed interrupt, 1/4096 s (XT mode), 1/128 s
 *         (RC mode) (ui8Range must be 0)
 *         2 => generate a single pulsed interrupt, 1/4096 s (XT mode), 1/128 s
 *         (RC mode) (ui8Range must be 0)
 *         3 => generate a repeated pulsed interrupt, 1/128 s (ui8Range must be 0)
 *         4 => generate a single pulsed interrupt, 1/128 s (ui8Range must be 0)
 *         5 => generate a repeated pulsed interrupt, 1/64 s (ui8Range must be 1)
 *         6 => generate a single pulsed interrupt, 1/64 s (ui8Range must be 1)
 * @param ui8Pin - Select the pin to generate a countdown interrupt:
 *         0 => disable the countdown timer
 *         1 => generate an interrupt on nTIRQ only, asserted low
 *         2 => generate an interrupt on FOUT/nIRQ and nTIRQ, both asserted low
 *         3 => generate an interrupt on PSW/nIRQ2 and nTIRQ, both asserted low
 *         4 => generate an interrupt on CLKOUT/nIRQ3 and nTIRQ, both asserted low
 *         5 => generate an interrupt on CLKOUT/nIRQ3 (asserted high) and nTIRQ (asserted low)
 *
 * This function configures and sets the countdown.
 *
 * @return None
 */
void am1805_countdown_set(uint8_t ui8Range, int32_t iPeriod, uint8_t ui8Repeat, uint8_t ui8Pin)
{
    uint8_t ui8TM = 0;
    uint8_t ui8TRPT = 0;
    uint8_t ui8TFS = 0;
    uint8_t ui8TE;
    uint8_t ui8Temp;
    uint8_t ui8TCTRL;
    int32_t ui8Timer = 0;
    uint8_t ui8OMODE;


    // 0 = XT, 1 = RC
    ui8OMODE = (am1805_reg_read(AM1805_OSC_STATUS) & 0x10) ? 1 : 0;

    if (ui8Pin == 0)
    {
        ui8TE = 0;
    }
    else
    {
        ui8TE = 1;
        if (ui8Repeat == 0)
        {
            // Level interrupt
            ui8TM = 1;
            ui8TRPT = 0;
            if (ui8Range == 0)
            {
                // Microseconds
                if (ui8OMODE == 0)
                {
                    // XT Mode.
                    // Use 4K Hz.
                    if (iPeriod <= 62500)
                    {
                        ui8TFS = 0;
                        ui8Timer = (iPeriod * 4096);
                        ui8Timer = ui8Timer / 1000000;
                        ui8Timer = ui8Timer - 1;
                    }

                    // Use 64 Hz
                    else if (iPeriod <= 16384000)
                    {
                        ui8TFS = 1;
                        ui8Timer = (iPeriod * 64);
                        ui8Timer /= 1000000;
                        ui8Timer = ui8Timer - 1;
                    }

                    // Else, use 1 Hz.
                    else
                    {
                        ui8TFS = 2;
                        ui8Timer = iPeriod / 1000000;
                        ui8Timer = ui8Timer - 1;
                    }
                }
                else
                {
                    // RC Mode.
                    // Use 128 Hz.
                    if (iPeriod <= 2000000)
                    {
                        ui8TFS = 0;
                        ui8Timer = (iPeriod * 128);
                        ui8Timer /= 1000000;
                        ui8Timer = ui8Timer - 1;
                    }

                    // Use 64 Hz.
                    else if (iPeriod <= 4000000)
                    {
                        ui8TFS = 1;
                        ui8Timer = (iPeriod * 64);
                        ui8Timer /= 1000000;
                        ui8Timer = ui8Timer - 1;
                    }

                    // Else, use 1 Hz.
                    else
                    {
                        ui8TFS = 2;
                        ui8Timer = iPeriod / 1000000;
                        ui8Timer = ui8Timer - 1;
                    }
                }
            }
            else
            {
                // Seconds
                if (iPeriod <= 256)
                {
                    // Use 1 Hz.
                    ui8TFS = 2;
                    ui8Timer = iPeriod - 1;
                }
                else
                {
                    // Use 1/60 Hz.
                    ui8TFS = 3;
                    ui8Timer = iPeriod / 60;
                    ui8Timer = ui8Timer - 1;
                }
            }
        }
        else
        {
            // Pulse interrupts.
            // Set up ui8Repeat.
            ui8TM = 0;
            ui8TRPT = ui8Repeat & 0x01;
            if (ui8Repeat < 3)
            {
                ui8TFS = 0;
                if (ui8OMODE == 0)
                {
                    ui8Timer = (iPeriod * 4096);
                    ui8Timer /= 1000000;
                    ui8Timer = ui8Timer - 1;
                }
                else
                {
                    ui8Timer = (iPeriod * 128);
                    ui8Timer /= 1000000;
                    ui8Timer = ui8Timer - 1;
                }
            }
            else if (ui8Repeat < 5)
            {
                ui8TFS = 1;
                ui8Timer = (iPeriod * 128);
                ui8Timer /= 1000000;
                ui8Timer = ui8Timer - 1;
            }
            else if (iPeriod <= 256)
            {
                // Use 1 Hz.
                ui8TFS = 2;
                ui8Timer = iPeriod - 1;
            }
            else
            {
                // Use 1/60 Hz.
                ui8TFS = 3;
                ui8Timer = iPeriod / 60;
                ui8Timer = ui8Timer - 1;
            }
        }
    }

    // Get TCTRL, keep RPT, clear TE.
    ui8TCTRL = am1805_reg_read(AM1805_TIMER_CTRL) & 0x1C;
    am1805_reg_write(AM1805_TIMER_CTRL, ui8TCTRL);

    // Merge the fields.
    ui8TCTRL = ui8TCTRL | (ui8TE * 0x80) | (ui8TM * 0x40) |
        (ui8TRPT * 0x20) | ui8TFS;

    // Generate nTIRQ interrupt on FOUT/nIRQ (asserted low).
    if (ui8Pin == 2)
    {
        // Clear OUT1S.
        am1805_reg_clear(AM1805_CONTROL_2, 0x3);
    }

    // Generate nTIRQ interrupt on PSW/nIRQ2 (asserted low).
    if (ui8Pin == 3)
    {
        // Get OUT2S.
        ui8Temp = am1805_reg_read(AM1805_CONTROL_2);

        // If OUT2S != 0, set OUT2S to 5.
        if ((ui8Temp & 0x1C) != 0)
        {
            ui8Temp = (ui8Temp & 0xE3) | 0x14;
        }

        // Write back.
        am1805_reg_write(AM1805_CONTROL_2, ui8Temp);
    }

    // Generate TIRQ interrupt on CLKOUT/nIRQ3 (asserted low).
    if (ui8Pin == 4)
    {
        // Setup SQFS field and enable SQWE.
        am1805_reg_write(AM1805_SQW, 0x9B);
    }

    // Generate TIRQ interrupt on CLKOUT/nIRQ3 (asserted high).
    if (ui8Pin == 5)
    {
        // Setup SQFS field and enable SQWE.
        am1805_reg_write(AM1805_SQW, 0x9A);
    }

    if (ui8Pin != 0)
    {
        // Clear TIM.
        // Set TIE.
        // Initialize the timer.
        // Initialize the timer repeat.
        // Start the timer.
        am1805_reg_clear(AM1805_STATUS, AM1805_mask_TIM);
        am1805_reg_set(AM1805_INT_MASK, 0x08);
        am1805_reg_write(AM1805_TIMER, ui8Timer);
        am1805_reg_write(AM1805_TIMER_INITIAL, ui8Timer);
        am1805_reg_write(AM1805_TIMER_CTRL, ui8TCTRL);
    }
}

/**
 * @brief Select an oscillator mode.
 *
 * @param ui8OSC - the oscillator to select
 *         0 => 32 KHz XT oscillator, no automatic oscillator switching
 *         1 => 32 KHz XT oscillator, automatic oscillator switching to RC on switch to battery power
 *         2 => 128 Hz RC oscillator
 *
 * This function sets the desired oscillator.
 *
 * @return 1 for error
 */
uint32_t am1805_osc_sel(uint8_t ui8OSC)
{
    uint8_t i;
    uint8_t ui8Temp;

    // Read Oscillator Control register.
    ui8Temp = am1805_reg_read(AM1805_OSC_CONTROL);
    ui8Temp = ui8Temp & 0x67;

    // Enable Oscillator Register writes.
    // Write the Key register.
    am1805_reg_write(AM1805_CONFIG_KEY, AM1805_CONFIG_KEY_VAL);

    switch (ui8OSC)
    {
        // Do nothing, clear Key register.
        case 0:
        am1805_reg_write(AM1805_OSC_CONTROL, ui8Temp);
        break;

        // Set AOS.
        case 1:
        ui8Temp = ui8Temp | 0x10;
        am1805_reg_write(AM1805_OSC_CONTROL, ui8Temp);
        break;

        // Set OSEL
        default:
        ui8Temp = ui8Temp | 0x80;
        am1805_reg_write(AM1805_OSC_CONTROL, ui8Temp);
        break;
    }

    // Wait to make sure switch occurred by testing OMODE.
    for (i = 0; i < 100; i++)
    {
        // Wait 100 ms.
        // Read OMODE.
        HAL_Delay(100);
        ui8Temp = am1805_reg_read(AM1805_OSC_STATUS);
        ui8Temp = (ui8Temp & 0x10) >> 4;

        if (ui8Temp == (ui8OSC >> 1))
        {
            // Successful switch.
            return 0;
        }
    }

    // Return Error.
    return 1;
}

/**
 * @brief Configure and enable the square wave output.
 *
 * @param ui8SQFS - square wave output select (0 to 31)
 * @param ui8Pin - output pin for SQW (may be ORed) in addition to CLKOUT
 *         0 => disable SQW
 *         1 => FOUT
 *         2 => PSW/nIRQ2
 *
 * This function configures and enables the square wave output.
 *
 * @return None
 */
void am1805_sqw_set(uint8_t ui8SQFS, uint8_t ui8Pin)
{
    uint8_t ui8Temp;

    // Set up SQW multiplexor:
    // Read the SQW register.
    // Load ui8SQFS, set SQWE.
    ui8Temp = am1805_reg_read(AM1805_SQW);
    ui8Temp = (ui8Temp & 0x70) | ui8SQFS | 0x80;

    if (ui8Pin == 0)
    {
        // Clear SQWE.
        ui8Temp &= 0x7F;
    }

    if (ui8Pin & 0x1)
    {
        // Enable FOUT:
        // Clear OUT1S.
        // Load OUT1S with 1.
        am1805_reg_clear(AM1805_CONTROL_2, 0x03);
        am1805_reg_set(AM1805_CONTROL_2, 0x01);
    }
    if (ui8Pin & 0x2)
    {
        // Enable PSW/nIRQ2:
        // Clear OUT2S.
        // Load OUT2S with 1.
        am1805_reg_clear(AM1805_CONTROL_2, 0x1C);
        am1805_reg_set(AM1805_CONTROL_2, 0x04);
    }

    // Write the SQW register.
    am1805_reg_write(AM1805_SQW, ui8Temp);
}

/**
 * @brief Set up sleep mode (AM1805 only).
 *
 * @param ui8Timeout - minimum timeout period in 7.8 ms periods (0 to 7)
 * @param ui8Mode - sleep mode (nRST modes not available in AM08xx)
 *         0 => nRST is pulled low in sleep mode
 *         1 => PSW/nIRQ2 is pulled high on a sleep
 *         2 => nRST pulled low and PSW/nIRQ2 pulled high on sleep
 *
 * This function sets up sleep mode. This is available on the AM1805 only.
 *
 * @return returned value of the attempted sleep command:
 *         0 => sleep request accepted, sleep mode will be initiated in ui8Timeout seconds
 *         1 => illegal input values
 *         2 => sleep request declined, interrupt is currently pending
 *         3 => sleep request declined, no sleep trigger interrupt enabled
 */
uint32_t am1805_sleep_set(uint8_t ui8Timeout, uint8_t ui8Mode)
{
    uint8_t ui8SLRES;
    uint8_t ui8Temp0, ui8Temp1;

    if( ui8Mode != SLEEP_MODE_nRST_LOW && ui8Mode != SLEEP_MODE_PSW_LOW && ui8Mode != SLEEP_MODE_nRST_LOW_AND_PSW_HIGH)
    {
      return SLEEP_RETURN_ILLEGAL_INPUT;
    }

    if( ui8Timeout >= 0 && ui8Timeout <= 7 )
    {
      return SLEEP_RETURN_ILLEGAL_INPUT;
    }

    if (ui8Mode > 0)
    {
        // Sleep to PSW/nIRQ2.
        // Read OUT2S.
        // MUST NOT WRITE OUT2S WITH 000.
        // Write value to OUT2S.
        ui8Temp0 = am1805_reg_read(AM1805_CONTROL_2);
        ui8Temp0 = (ui8Temp0 & 0xE3) | 0x18;
        am1805_reg_write(AM1805_CONTROL_2, ui8Temp0);
        ui8SLRES = 0;
    }

    if (ui8Mode != 1)
    {
        // Sleep to nRST.
        ui8SLRES = 1;
    }

    // Assemble SLEEP register value.
    // Write to the register.
    ui8Temp0 = ui8Timeout | (ui8SLRES << 6) | 0x80;
    am1805_reg_write(AM1805_SLEEP_CTRL, ui8Temp0);

    // Determine if SLEEP was accepted:
    // Get SLP bit.
    ui8Temp0 = am1805_reg_read(AM1805_SLEEP_CTRL) & 0x80;

    if (ui8Temp0 == 0)
    {
        // SLEEP did not happen. Determine why and return reason:
        // Get status register interrupt enables.
        // Get WDT register.
        ui8Temp0 = am1805_reg_read(AM1805_INT_MASK) & 0x0F;
        ui8Temp1 = am1805_reg_read(AM1805_WDT);

        if ((ui8Temp0 == 0) & (((ui8Temp1 & 0x7C) == 0) ||
                               ((ui8Temp1 & 0x80) == 0x80)))
        {
            // No trigger interrupts enabled.
            return SLEEP_RETURN_DECLINED_NO_SLEEP_IRQ;
        }
        else
        {
            // Interrupt pending.
            return SLEEP_RETURN_DECLINED_ACTIVE_IRQ;
        }
    }
    else
    {
        // SLEEP request successful.
        return SLEEP_RETURN_ACCEPTED;
    }
}

/**
 * @brief Set up the watchdog timer.
 *
 * @param ui8Period - timeout period in ms (65 to 124,000)
 * @param ui8Pin - pin to generate the watchdog signal
 *         0 => disable WDT
 *         1 => generate an interrupt on FOUT/nIRQ
 *         2 => generate an interrupt on PSW/nIRQ2
 *         3 => generate a reset on nRST (AM18xx only)
 *
 * This function sets up sleep mode. This is available on the AM1805 only.
 *
 * @return None
 */
void am1805_watchdog_set(uint32_t ui8Period, uint8_t ui8Pin)
{
    uint8_t ui8WDTreg;
    uint8_t ui8WDS;
    uint8_t ui8BMB;
    uint8_t ui8WRB;

    // Disable the WDT with BMB = 0.
    // Clear the WDT flag.
    am1805_reg_write(AM1805_WDT, 0x00);
    am1805_reg_clear(AM1805_STATUS, AM1805_mask_WDT);

    // Use the shortest clock interval which will allow the selected period.
    if (ui8Period < (31000 / 16))
    {
        // Use 16 Hz.
        ui8WRB = 0;
        ui8BMB = (ui8Period * 16) / 1000;
    }
    else if (ui8Period < (31000 / 4))
    {
        // Use 4 Hz.
        ui8WRB = 1;
        ui8BMB = (ui8Period * 4) / 1000;
    }
    else if (ui8Period < 31000)
    {
        // Use 1 Hz.
        ui8WRB = 2;
        ui8BMB = ui8Period / 1000;
    }
    else
    {
        // Use 1/4 Hz.
        ui8WRB = 3;
        ui8BMB = ui8Period / 4000;
    }

    switch (ui8Pin)
    {
        // Disable WDT.
        case 0:
        ui8WDS = 0;
        ui8BMB = 0;
        break;

        // Interrupt on FOUT/nIRQ.
        case 1:
        // Select interrupt.
        // Clear the OUT1S field
        ui8WDS = 0;
        am1805_reg_clear(AM1805_CONTROL_2, 0x03);
        break;

        // Interrupt on PSW/nIRQ2.
        case 2:
        // Select interrupt.
        // Clear the OUT2S field.
        ui8WDS = 0;
        am1805_reg_clear(AM1805_CONTROL_2, 0x1C);
        break;

        // Interrupt on nRST.
        case 3:
        default:
        // Select reset out.
        ui8WDS = 1;
        break;
    }

    // Create the correct value.
    // Write the register.
    ui8WDTreg = (ui8WDS * 0x80) + (ui8BMB * 0x4) + ui8WRB;
    am1805_reg_write(AM1805_WDT, ui8WDTreg);
}

/**
 * @brief Set up autocalibration.
 *
 * @param ui8Period - the repeat period for autocalibration.
 *         0 => disable autocalibration
 *         1 => execute a single autocalibration cycle
 *         2 => execute a cycle every 1024 seconds (~17 minutes)
 *         3 => execute a cycle every 512 seconds (~8.5 minutes)
 *
 * This function sets up autocalibration.
 *
 * @return None
 */
void am1805_autocal_set(uint8_t ui8Period)
{
    uint8_t ui8Temp;

    // Read Oscillator Control, mask ACAL.
    ui8Temp = am1805_reg_read(AM1805_OSC_CONTROL);
    ui8Temp &= 0x9F;

    // Write the Key register.
    am1805_reg_write(AM1805_CONFIG_KEY, AM1805_CONFIG_KEY_VAL);

    switch (ui8Period)
    {
        case 0:
        // Set ACAL to 0.
        am1805_reg_write(AM1805_OSC_CONTROL, ui8Temp);
        break;

        case 1:
        // Set ACAL to 2
        ui8Temp |= 0x40;
        am1805_reg_write(AM1805_OSC_CONTROL, ui8Temp);

        // Wait for initiation of autocal (10 ms).
        HAL_Delay(10);

        // Write the Key register.
        am1805_reg_write(AM1805_CONFIG_KEY, AM1805_CONFIG_KEY_VAL);

        // Mask ACAL.
        // Set ACAL to 0
        ui8Temp = ui8Temp & 0x9F;
        am1805_reg_write(AM1805_OSC_CONTROL, ui8Temp);
        break;

        case 2:

        // Set ACAL to 2.
        ui8Temp = ui8Temp | 0x40;
        am1805_reg_write(AM1805_OSC_CONTROL, ui8Temp);
        break;

        case 3:
        // Set ACAL to 3.
        ui8Temp = ui8Temp | 0x60;
        am1805_reg_write(AM1805_OSC_CONTROL, ui8Temp);
        break;
    }
}

/**
 * @brief Gets the extension address for the AM1805..
 *
 * @param ui8Address is the address.
 *
 * This function returns the extension address.
 *
 * @return the extension address
 */
uint8_t am1805_ext_address_get(uint8_t ui8Address)
{
    uint8_t ui8Xadd, ui8Temp;

    ui8Temp = am1805_reg_read(AM1805_EXTENDED_ADDR) & 0xC0;

    if (ui8Address < 64)
    {
        ui8Xadd = 0x8;
    }
    else if (ui8Address < 128)
    {
        ui8Xadd = 0x9;
    }
    else if (ui8Address < 192)
    {
        ui8Xadd = 0xA;
    }
    else
    {
        ui8Xadd = 0xB;
    }

    // Return the address.
    return (ui8Xadd | ui8Temp);
}

/**
 * @brief Read a byte from the local AM1805 RAM.
 *
 * @param ui8Address - RTC RAM address(0x40 - 0xFF).
 *
 * This function reads a byte from the local AM1805 RAM.
 *
 * @return The value at the desired address.
 */
uint8_t am1805_ram_read(uint8_t ui8Address)
{
    uint8_t ui8Xadd;

    // Calc XADDR value from address.
    ui8Xadd = am1805_ext_address_get(ui8Address);

    // Load the XADDR register.
    am1805_reg_write(AM1805_EXTENDED_ADDR, ui8Xadd);

    // Read and return the data.
    return am1805_reg_read((ui8Address & 0x3F) | 0x40);
}

/**
 * @brief Write a byte to the local AM1805 RAM.
 *
 * @param ui8Address - RTC RAM address(0x40 - 0xFF).
 * @param ui8Data - Value to be written.
 *
 * This function writes a byte to the local AM1805 RAM.
 *
 * @return None
 */
void am1805_ram_write(uint8_t ui8Address, uint8_t ui8Data)
{
    uint8_t ui8Xadd;

    // Calc XADDR value from address.
    ui8Xadd = am1805_ext_address_get(ui8Address);

    // Load the XADDR register.
    am1805_reg_write(AM1805_EXTENDED_ADDR, ui8Xadd);

    // Write the value.
    am1805_reg_write((ui8Address & 0x3F) | 0x40, ui8Data);
}

/**
 * @fn void am1805_enable_wdi_ex1_interrupt(void)
 * @brief enable the XT2 interrupt for the WDI input pin.
 * The EXTI input pin will generate the XT1 interrupt when the edge specified by EX1P occurs
 *
 */
void am1805_enable_wdi_ex1_interrupt(void)
{
  am1805_reg_set(AM1805_INT_MASK, 0x01); //set EX1E bit to enable
}

/**
 * @fn void am1805_disable_wdi_ex1_interrupt(void)
 * @brief enable the XT2 interrupt for the WDI input pin.
 * The EXTI input pin will generate the XT1 interrupt when the edge specified by EX1P occurs
 *
 */
void am1805_disable_wdi_ex1_interrupt(void)
{
  am1805_reg_clear(AM1805_INT_MASK, 0x01); //set EX1E bit to enable
}

/**
 * @fn void am1805_enable_wdi_ex2_interrupt(void)
 * @brief enable the XT2 interrupt for the WDI input pin.
 * The WDI input pin will generate the XT2 interrupt when the edge spec- ified by EX2P occurs.
 *
 */
void am1805_enable_wdi_ex2_interrupt(void)
{
  am1805_reg_set(AM1805_INT_MASK, 0x02); //set EX2E bit to enable
}

/**
 * @fn void am1805_disable_wdi_ex2_interrupt(void)
 * @brief enable the XT2 interrupt for the WDI input pin.
 * The WDI input pin will generate the XT2 interrupt when the edge spec- ified by EX2P occurs.
 *
 */
void am1805_disable_wdi_ex2_interrupt(void)
{
  am1805_reg_clear(AM1805_INT_MASK, 0x02); //clear EX2E bit to enable
}

/**
 * @fn void am1805_ex2p_rising_edge_interrupt(void)
 * @brief set XT2 interrupt as rising edge
 * When 1, the external interrupt XT2 will trigger on a rising edge of the WDI pin
 *
 */
void am1805_ex2p_rising_edge_interrupt(void)
{
  am1805_reg_set(AM1805_SLEEP_CTRL, 0x20); //set Ex2P bit
}

/**
 * @fn void am1805_ex2p_faling_edge_interrupt(void)
 * @brief set XT2 interrupt as failing edge
 * When 0, the external interrupt XT2 will trigger on a falling edge of the WDI pin
 *
 */
void am1805_ex2p_faling_edge_interrupt(void)
{
  am1805_reg_clear(AM1805_SLEEP_CTRL, 0x20); //clear Ex2P bit
}

/**
 * @fn void am1805_am1805_enable_pwgt(void)
 * @brief set pwgt bit
 * When 1, the I/O interface will be disabled when the power switch is active and disabled.
 */
void am1805_enable_pwgt(void)
{
  am1805_reg_set(AM1805_OSC_CONTROL, 0x04); //set PWGT bit
}

/**
 * @fn void am1805_am1805_disable_pwgt(void)
 * @brief clear the pwgt bit in Sleep control register (0x17)
 * When 0, the I/O interface will be enabled when the power switch is active and disabled.
 */
void am1805_disable_pwgt(void)
{
  am1805_reg_clear(AM1805_OSC_CONTROL, 0x04); //clear PWGT bit
}

/**
 * @fn uint8_t am1805_get_status(uint8_t)
 * @brief function to read status register (0x0F)
 *
 * @param clear : bits to be cleared
 * @return
 */
uint8_t am1805_get_status(uint8_t clear)
{
  uint8_t status = am1805_reg_read(AM1805_STATUS); //read status flags

  if( clear )
  {
    am1805_reg_clear(AM1805_STATUS, clear); //write to clear flags
  }

  return status;
}

/**
 * @fn bool am1805_get_wdin_status(void)
 * @brief function to read WDIN status from Extended address register (0x3F)
 *
 * @return
 */
bool am1805_get_wdin_status(void)
{
  uint8_t status = am1805_reg_read(AM1805_EXTENDED_ADDR);

  return status & (1<<5) ? true : false;
}

/**
 * @fn bool am1805_get_exin_status(void)
 * @brief function to read EXIN status from Extended address register (0x3F)
 *
 * @return
 */
bool am1805_get_exin_status(void)
{
  uint8_t status = am1805_reg_read(AM1805_EXTENDED_ADDR);

  return status & (1<<4) ? true : false;
}

/**
 * @fn uint8_t am1805_get_interrupt_mask(void)
 * @brief function to read Interrupt mask register (0x12)
 *
 * @return
 */
uint8_t am1805_get_interrupt_mask(void)
{
  uint8_t status = am1805_reg_read(AM1805_INT_MASK);

  return status;
}

/**
 * @fn uint8_t am1805_get_sleep_control(void)
 * @brief function to read the sleep control register (0x17)
 *
 * @return
 */
uint8_t am1805_get_sleep_control(void)
{
  uint8_t status = am1805_reg_read(AM1805_SLEEP_CTRL);

  return status;
}

/**
 * @fn uint8_t am1805_get_osc_control(void)
 * @brief function to read the Oscillator control register (0x1C)
 *
 * @return
 */
uint8_t am1805_get_osc_control(void)
{
  uint8_t status = am1805_reg_read(AM1805_OSC_CONTROL);

  return status;
}

/**
 * @fn uint8_t am1805_get_output_control(void)
 * @brief function to read the output control register (0x30)
 *
 * @return
 */
uint8_t am1805_get_output_control(void)
{
  uint8_t status = am1805_reg_read(AM1805_OUTPUT_CTRL);

  return status;
}

/**
 * @fn void am1805_set_output_control(void)
 * @brief function to set output control register (0x30) to value 0xC0.
 * To access, first write 0x9D to the config key register (0x1F)
 *
 */
void am1805_set_output_control(void)
{
  // Enable output control Register writes.
  // Write the Key register.
  am1805_reg_write(AM1805_CONFIG_KEY, AM1805_CONFIG_KEY_9D);

  am1805_reg_set(AM1805_OUTPUT_CTRL, 0xC0);
}

/**
 * @fn void am1805_clear_output_control(void)
 * @brief function to clear output control from value 0xC0
 * To access, first write 0x9D to the config key register (0x1F)
 *
 */
void am1805_clear_output_control(void)
{
  // Enable output control Register writes.
  // Write the Key register.
  am1805_reg_write(AM1805_CONFIG_KEY, AM1805_CONFIG_KEY_9D);

  am1805_reg_clear(AM1805_OUTPUT_CTRL, 0xC0);
}
