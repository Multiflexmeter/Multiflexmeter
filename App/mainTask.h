/*
 * mainTask.h
 *
 *  Created on: 28 sep. 2023
 *      Author: p.kwekkeboom
 */
/**
  ******************************************************************************
  * @file           : mainTask.h
  * @brief          : Header for mainTask.c file.
  * @author         : P.Kwekkeboom
  * @date           : Sep 28, 2023
  ******************************************************************************
  */
#ifndef MAINTASK_H_
#define MAINTASK_H_

typedef enum
{

  INIT_POWERUP = 0,
  INIT_SLEEP,
  ENABLE_SLOTPOWER,
  START_SENSOR_MEASURE,
  WAIT_FOR_SENSOR_DATA,
  READ_SENSOR_DATA,
  WAIT_BATMON_DATA,
  SAVE_DATA,
  SEND_LORA_DATA,
  NEXT_SENSOR_MODULE,
  WAIT_LORA_TRANSMIT_READY,
  CHECK_LORA_JOINED,
  SWITCH_OFF_VSYS,
  CHECK_LORA_REJOIN,
  WAIT_LORA_RECEIVE_READY,
  WAIT_FOR_SLEEP,
  STOP_MAINTASK,

} ENUM_STATE_MAINTASK;

const void init_mainTask(void);
const void stop_mainTask(bool resume);
const void pause_mainTask(void);
const void resume_mainTask(void);

const void enable_vAlwaysOn(void);
const void disable_vAlwaysOn(void);

#endif /* MAINTASK_H_ */
