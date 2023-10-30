/**
  ******************************************************************************
  * @file           : FRAM_functions.h
  * @brief          : Header for FRAM_functions.c file.
  * @author         : P.Kwekkeboom
  * @date           : Oct 12, 2023
  ******************************************************************************
  */
#ifndef FRAM_FRAM_FUNCTIONS_H_
#define FRAM_FRAM_FUNCTIONS_H_

const void saveLoraSettings( const void *pSource, size_t length );
const void restoreLoraSettings( const void *pSource, size_t length);

#endif /* FRAM_FRAM_FUNCTIONS_H_ */
