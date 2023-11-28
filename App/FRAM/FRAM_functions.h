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

typedef enum
{
  FRAM_SETTING_MODEMID = 1,
}ENUM_FRAM_SETTING;

const void saveLoraSettings( const void *pSource, size_t length );
const void restoreLoraSettings( const void *pSource, size_t length);

const void saveFramSettings( const void *pSource, size_t length );
const void restoreFramSettings( const void *pSource, size_t length);

const int8_t setFramSetting( ENUM_FRAM_SETTING setting, void * value, bool forceWrite );
const int8_t getFramSetting( ENUM_FRAM_SETTING setting, void * value, bool forceRead );


#endif /* FRAM_FRAM_FUNCTIONS_H_ */
