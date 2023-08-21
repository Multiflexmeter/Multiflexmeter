/**
 * @file bq35100.h
 *
 * @brief Driver to interface with the TI bq35100 battery gauge.
 *
 */

#ifndef BATTERY_GAUGE_BQ35100_H_
#define BATTERY_GAUGE_BQ35100_H_

#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"

/* Registers */
#define REG_CONTROL                             0x00
#define REG_ACCUMULATED_CAPACITY                0x02
#define REG_TEMPERATURE                         0x06
#define REG_VOLTAGE                             0x08
#define REG_BATTERY_STATUS                      0x0A
#define REG_BATTERY_ALERT                       0x0B
#define REG_CURRENT                             0x0C
#define REG_SCALED_R                            0x16
#define REG_MEASURED_Z                          0x22
#define REG_INTERNAL_TEMP                       0x28
#define REG_STATE_OF_HEALTH                     0x2E
#define REG_DESIGN_CAPACITY                     0x3C
#define REG_CAL_COUNT                           0x79
#define REG_CAL_CURRENT                         0x7A
#define REG_CAL_VOLTAGE                         0x7C
#define REG_CAL_TEMP                            0x7E

/* Commands */
#define CMD_DATA_CLASS                          0x3E
#define CMD_BLOCK_DATA                          0x40
#define CMD_CHECK_SUM                           0x60
#define CMD_FLAGS                               0x06

/* Subcommands */
#define SUB_CMD_CONTROL_STATUS                  0x0000
#define SUB_CMD_DEVICE_TYPE                     0x0001
#define SUB_CMD_FW_VERSION                      0x0002
#define SUB_CMD_HW_VERSION                      0x0003
#define SUB_CMD_STATIC_CHEM_DF_CHKSUM           0x0005
#define SUB_CMD_CHEM_ID                         0x0006
#define SUB_CMD_PREV_MACWRITE                   0x0007
#define SUB_CMD_BOARD_OFFSET                    0x0009
#define SUB_CMD_CC_OFFSET                       0x000A
#define SUB_CMD_CC_OFFSET_SAVE                  0x000B
#define SUB_CMD_GAUGE_START                     0x0011
#define SUB_CMD_GAUGE_STOP                      0x0012
#define SUB_CMD_SEALED                          0x0020
#define SUB_CMD_CAL_ENABLE                      0x002D
#define SUB_CMD_LT_ENABLE                       0x002E
#define SUB_CMD_RESET                           0x0041
#define SUB_CMD_NEW_BATTERY                     0xA613


/* Device I2C address. */
#define BQ35100_ADDRESS 0x55 << 1

/* Settling time after gaugeEnable is set high */
#define GAUGE_ENABLE_SETTLING_TIME_MS 10

/* The default seal codes (step 1 in the higher word, step 2 the lower word), NOT byte reversed. */
#define SEAL_CODES_DEFAULT 0x04143672

/* The default full access codes (step 1 in the higher word, step 2 the lower word). */
#define FULL_ACCESS_CODES_DEFAULT 0xFFFFFFFF

/* The security mode of the BQ35100 chip. */
typedef enum
{
  SECURITY_MODE_UNKNOWN = 0x00,
  SECURITY_MODE_FULL_ACCESS = 0x20, // Allows writes to all of memory.
  SECURITY_MODE_UNSEALED = 0x40,    // Allows writes to all of memory apart from the security codes area.
  SECURITY_MODE_SEALED = 0x60       // Normal operating mode, prevents accidental writes.
} SecurityMode;

/* Functions */
void bq35100_init(I2C_HandleTypeDef *i2cHandle);
int16_t bq35100_getVoltage(void);
int16_t bq35100_getCurrent(void);
float bq35100_getTemp(void);
uint16_t bq35100_getDesignCapacity(void);
uint32_t bq35100_getRemainingCapacity(void);
uint32_t bq35100_getUsedCapacity(void);
SecurityMode bq35100_getSecurityMode(void);
bool bq35100_isGaugeEnabled(void);
bool bq35100_enableGauge(void);
bool bq35100_disableGauge(bool ignoreCheck);

#endif /* BATTERY_GAUGE_BQ35100_H_ */
