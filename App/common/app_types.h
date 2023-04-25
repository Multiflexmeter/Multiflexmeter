/**
  ******************************************************************************
  * @addtogroup     : common
  * @{
  * @file           : app_types.h
  * @brief          : Header app specific type definitions.  *
  * @author         : P.Kwekkeboom
  * @date           : Apr 24, 2023
  * @copyright      : 2023 Dekimo Goes
  * @}
  ******************************************************************************
  */

#ifndef COMMON_APP_TYPES_H_
#define COMMON_APP_TYPES_H_

#ifndef TRUE
#define TRUE      ( 1==1 )
#endif

#ifndef FALSE
#define FALSE     ( !TRUE )
#endif

/**
 * @brief enumeration for boolean type
 */
typedef enum { false = 0, true } bool;


#endif /* COMMON_APP_TYPES_H_ */
