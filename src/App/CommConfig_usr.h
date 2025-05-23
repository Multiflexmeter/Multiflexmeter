/**
  ******************************************************************************
  * @file           : CommConfig_usr
  * @brief          : Header for CommConfig_usr file.
  * @author         : P.Kwekkeboom
  * @date           : May 22, 2023
  * @copyright      : 2023 Dekimo Goes
  ******************************************************************************
  */
#ifndef COMMCONFIG_USR_H_
#define COMMCONFIG_USR_H_

const uint16_t getVccSupply(void);
const uint16_t getVbusSupply(void);
const int8_t getTemperature(void);


#endif /* COMMCONFIG_USR_H_ */
