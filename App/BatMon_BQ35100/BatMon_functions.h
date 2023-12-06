/**
  ******************************************************************************
  * @file           : BatMon_functions.h
  * @brief          : Header for BatMon_functions.c file.
  * @author         : P.Kwekkeboom
  * @date           : Nov 1, 2023
  ******************************************************************************
  */

#ifndef BATMON_BQ35100_BATMON_FUNCTIONS_H_
#define BATMON_BQ35100_BATMON_FUNCTIONS_H_

typedef struct {
    int16_t voltage;
    int16_t current;
    int16_t temperature;
    uint16_t stateOfHealth;
    uint16_t ScaledR;
    uint16_t MeasuredZ;

}structBatMonData;

const void batmon_enable(void);
const void batmon_disable(void);
const void batmon_enable_gauge(void);
const void batmon_disable_gauge(void);
const bool batmon_isInitComplet(void);
const bool batmon_isGaugeActive(void);
const bool batmon_isReady(void);
const void initBatMon(void);
const void deinitBatMon(void);
const void batmon_measure(void);
const structBatMonData batmon_getMeasure(void);

#endif /* BATMON_BQ35100_BATMON_FUNCTIONS_H_ */
