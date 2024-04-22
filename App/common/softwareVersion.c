/**
  ******************************************************************************
  * @addtogroup     : common
  * @{
  * @file           : uart.c
  * @brief          : uart functions
  * @author         : P.Kwekkeboom
  * @date           : Sep 13, 2023
  * @}
  ******************************************************************************
  */

#include "softwareVersion.h"


#define MFM_SOFTWARE_VERSION  "0.1.23"
#define MFM_CONFIG_PROTOCOL_VERSION  "0"
#define MFM_SENSOR_PROTOCOL_VERSION  "0"

const char softwareVersionMFM[] = MFM_SOFTWARE_VERSION;
const char configProtocolVersionMFM[1] = MFM_CONFIG_PROTOCOL_VERSION;
const char sensorProtocolVersionMFM[1] = MFM_SENSOR_PROTOCOL_VERSION;

/**
 * @fn const char getSoftwareVersionMFM*(void)
 * @brief function to return software version
 *
 * @return pointer to char array
 */
const char * getSoftwareVersionMFM(void)
{
  return softwareVersionMFM;
}

/**
 * @fn const char getProtocolVersionConfig*(void)
 * @brief function to return protocol version of MFM
 *
 * @return pointer to char array
 */
const char * getProtocolVersionConfig(void)
{
  return configProtocolVersionMFM;
}

/**
 * @fn const char getProtocolVersionSensor*(void)
 * @brief furnction to return protocol version of sensor
 *
 * @return pointer to char array
 */
const char * getProtocolVersionSensor(void)
{
  return sensorProtocolVersionMFM;
}
