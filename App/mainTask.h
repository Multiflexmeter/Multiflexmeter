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

const void init_mainTask(void);
const void stop_mainTask(bool resume);
const void pause_mainTask(void);
const void resume_mainTask(void);

const void enable_vAlwaysOn(void);
const void disable_vAlwaysOn(void);

#endif /* MAINTASK_H_ */
