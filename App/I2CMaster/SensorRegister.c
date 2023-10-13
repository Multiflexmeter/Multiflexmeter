
#include "SensorRegister.h"

#include <stdint.h>
#include <string.h>

/* Register Declaration */

const SensorReg registers[] =
{
    {REG_FIRMWARE_VERSION,  UINT8_T,  10, READ},
    {REG_PROTOCOL_VERSION,  UINT8_T,  1,  READ},
    {REG_SENSOR_TYPE,       UINT16_T, 1,  READ},
    {REG_MEAS_START,        UINT8_T,  1,  READWRITE},
    {REG_MEAS_STATUS,       UINT8_T,  1,  READ},
    {REG_MEAS_TIME,         UINT16_T, 1,  READWRITE},
    {REG_MEAS_DATA,         SENSORDATA,  2,  READ},
    {REG_SENSOR_AMOUNT,     UINT8_T,  1,  READ},
    {REG_SENSOR_SELECTED,   UINT8_T,  1,  READWRITE},
    {REG_MEAS_TYPE,         UINT8_T,  1,  READWRITE},
    {REG_MEAS_SAMPLES,      UINT8_T,  1,  READWRITE},
    {REG_SENSOR_DATA,       UINT16_T, 1,  READ},
    {REG_ERROR_COUNT,       UINT16_T, 3,  READ},
    {REG_ERROR_STATUS,      UINT8_T,  1,  READ}
};

/**
 * @brief Find the index of the register in the constant register array.
 * @param regAddress The register address for which the index must be determined
 * @return The index of the provided register
 */
int8_t findRegIndex(uint8_t regAddress)
{
  uint8_t index = 0;
  uint8_t size = sizeof(registers) / sizeof(registers[0]);

  while((index < size) && (registers[index].adres != regAddress)) ++index;

  return (index == size ? -1 : index);
}
