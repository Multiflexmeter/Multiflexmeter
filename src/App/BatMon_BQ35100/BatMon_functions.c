/**
  ******************************************************************************
  * @addtogroup     : App
  * @{
  * @file           : BatMon_functions.c
  * @brief          : Battery monitor functions
  * @author         : P.Kwekkeboom
  * @date           : Nov 1, 2023
  * @}
  ******************************************************************************
  */

#include "main.h"
#include "sys_app.h"

#include "../IO/board_io.h"
#include "bq35100.h"

#include "BatMon_functions.h"


structBatMonData measure;

/**
 * @fn const void batmon_enable(void)
 * @brief enable battery monitor
 *
 */
const void batmon_enable(void)
{
  writeOutput_board_io(EXT_IO_GE_EN, GPIO_PIN_SET); //enable GE_EN, to operate battery monitor
}

/**
 * @fn const void batmon_disable(void)
 * @brief disable battery monitor
 *
 */
const void batmon_disable(void)
{
  writeOutput_board_io(EXT_IO_GE_EN, GPIO_PIN_RESET); //enable GE_EN, to operate battery monitor
}

/**
 * @fn const void batmon_enable_gauge(void)
 * @brief enable gauge battery monitor
 *
 */
const void batmon_enable_gauge(void)
{
  bq35100_enableGauge();
}

/**
 * @fn const void batmon_disable_gauge(void)
 * @brief disable gauge battery monitor
 *
 */
const void batmon_disable_gauge(void)
{
  bq35100_disableGauge(true);
}

/**
 * @fn const int batmon_isInitComplet(void)
 * @brief
 *
 * @return true is complete, false not complete, negative is peripheral timeout
 */
const int batmon_isInitComplet(void)
{
  uint8_t controlStatus[2] = {0};

  if( bq35100_getControlStatus(controlStatus, sizeof(controlStatus)))

    return (controlStatus[0] & CONTROL_STATUS0_INITCOMP) ? true : false;

  return -1;
}

/**
 * @fn const bool batmon_isGaugeActive(void)
 * @brief
 *
 * @return true is active, false not active
 */
const bool batmon_isGaugeActive(void)
{
  return bq35100_isGaugeEnabled();
}

/**
 * @fn const bool batmon_isReady(void)
 * @brief
 *
 * @return true is ready, false is busy
 */
const bool batmon_isReady(void)
{
  uint8_t controlStatus[2] = {0};

  bool result = bq35100_getControlStatus(controlStatus, sizeof(controlStatus));

  return (controlStatus[0] & CONTROL_STATUS0_G_DONE || result == false) ? true : false;
}

/**
 * @fn const void initBatMon(void)
 * @brief function to initialize battery monitor
 * - initialize the I2C handle
 */
const void initBatMon(void)
{
  bq35100_init(&hi2c1);
}

/**
 * @fn const void initBatMon(void)
 * @brief function to deinitialize battery monitor
 * - stop the gague
 * - disable the GE pin
 */
const void deinitBatMon(void)
{
  batmon_disable_gauge();
  batmon_disable();
}

/**
 * @fn const void batmon_measure(void)
 * @brief function to execute a battery monitor measure
 *
 */
const void batmon_measure(void)
{
  measure.voltage = bq35100_getVoltage();
  measure.current = bq35100_getCurrent();
  measure.temperature = bq35100_getInternalTemp();
  measure.stateOfHealth = bq35100_getStateOfHealth();
  measure.ScaledR = bq35100_getScaledR();
  measure.MeasuredZ = bq35100_getMeasuredZ();
}

/**
 * @fn const structBatMonData batmon_getMeasure(void)
 * @brief get battery monitor data
 *
 * @return
 */
const structBatMonData batmon_getMeasure(void)
{
  return measure;
}

/**
 * @fn const int8_t convertTemperatureGaugeToByte(int16_t)
 * @brief helper function to convert degrees in tenth of degrees to byte in whole degrees
 *
 * @param temperature
 * @return
 */
const int8_t convertTemperatureGaugeToByte(int16_t temperature)
{
  //check sign for round off
  if( temperature > 0 )
  {
    temperature+=5;
  }
  else
  { //negative
    temperature-=5;
  }

  return (int8_t)(temperature/10); //convert from tenth to whole degrees.
}

/**
 * @fn const void testBatMon(int, int32_t*)
 * @brief test function battery monitor
 *
 * @param mode
 * @param value
 */
const void testBatMon( int mode, int32_t * value )
{
  if( mode == 0 ) //enable gauge
  {
    *value = (int32_t)bq35100_enableGauge();
    APP_LOG(TS_OFF, VLEVEL_H, "cmd: Enable Gauge\r\n");
  }

  if( mode == 1) //read supply
  {
    *value = (int32_t)bq35100_getVoltage();
    APP_LOG(TS_OFF, VLEVEL_H, "cmd: getVoltage\r\n");
  }
  else if( mode == 2) //read current
  {
    *value = (int32_t)bq35100_getCurrent();
    APP_LOG(TS_OFF, VLEVEL_H, "cmd: getCurrent\r\n");
  }

  else if( mode == 3 ) //read temperature
  {
    *value = (int32_t)bq35100_getInternalTemp();
    APP_LOG(TS_OFF, VLEVEL_H, "cmd: getTemperature\r\n");
  }

  else if( mode == 4 ) //read state of health
  {
    *value = (int32_t)bq35100_getStateOfHealth();
    APP_LOG(TS_OFF, VLEVEL_H, "cmd: getSOH\r\n");
  }

  else if( mode == 5 ) //read design capacity
  {
    *value = (int32_t)bq35100_getDesignCapacity();
    APP_LOG(TS_OFF, VLEVEL_H, "cmd: getDesignCapacity\r\n");
  }

  else if( mode == 6 ) //read scaled R
  {
    *value = (int32_t)bq35100_getScaledR();
    APP_LOG(TS_OFF, VLEVEL_H, "cmd: getMeasured-R\r\n");
  }

  else if( mode == 7 ) //read measured Z
  {
    *value = (int32_t) bq35100_getMeasuredZ();
    APP_LOG(TS_OFF, VLEVEL_H, "cmd: getMeasured-Z\r\n");
  }

  else if( mode == 8 ) //disable gauge
  {
    *value = (int32_t)bq35100_disableGauge(true);
    APP_LOG(TS_OFF, VLEVEL_H, "cmd: Disable Gauge\r\n");
  }

  else if( mode == 9 ) //enable batmonitor
  {
    writeOutput_board_io(EXT_IOVSYS_EN, GPIO_PIN_SET);
    writeOutput_board_io(EXT_IO_GE_EN, GPIO_PIN_SET); //enable GE_EN, to operate battery monitor
    APP_LOG(TS_OFF, VLEVEL_H, "cmd: Enable Batmon\r\n");
    *value = 1;
  }

  else if( mode == 10 ) //disable batmonitor
  {
    writeOutput_board_io(EXT_IO_GE_EN, GPIO_PIN_RESET); //disable GE_EN, to disable battery monitor
    APP_LOG(TS_OFF, VLEVEL_H, "cmd: Disable Batmon\r\n");
    *value = 1;
  }

  else if( mode == 11 ) //instructs the fuel gauge with a new cell.
  {
    *value = bq35100_NewBattery();
    APP_LOG(TS_OFF, VLEVEL_H, "cmd: New Bat\r\n");
  }

}
