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
  CHECK_LORA_JOIN,
  SWITCH_ON_VSYS,
  SWITCH_SENSOR_SLOT,
  CHECK_SENSOR_INIT_AVAILABLE,
  START_SENSOR_INIT,
  WAIT_SENSOR_INIT_READY,
  START_SENSOR_MEASURE,
  WAIT_FOR_SENSOR_DATA,
  WAIT_BATTERY_GAUGE_IS_ALIVE,
  READ_SENSOR_DATA,
  WAIT_GAUGE_IS_ACTIVE,
  WAIT_BATMON_DATA,
  SAVE_DATA,
  SEND_LORA_DATA,
  NEXT_SENSOR_MODULE,
  WAIT_LORA_TRANSMIT_READY,
  GAUGE_DISABLE,
  SWITCH_OFF_VSYS,
  WAIT_LORA_RECEIVE_READY,
  WAIT_BATTERY_MONITOR_READY,
  CHECK_USB_CONNECTED,
  WAIT_USB_DISCONNECT,
  WAIT_FOR_SLEEP,

} ENUM_STATE_MAINTASK;

const void init_mainTask(void);
const void stop_mainTask(bool resume);
const void pause_mainTask(void);
const void resume_mainTask(void);

const void enable_vAlwaysOn(void);
const void disable_vAlwaysOn(void);

const void setDelayReJoin(int periodMs);

#endif /* MAINTASK_H_ */
