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


#define MFM_SOFTWARE_VERSION  "0.0.00"

const char softwareVersionMFM[] = MFM_SOFTWARE_VERSION;

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
