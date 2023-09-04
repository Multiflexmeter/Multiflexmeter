/**
  ******************************************************************************
  * @file           : common.h
  * @brief          : Header for common.c file.
  * @author         : P.Kwekkeboom
  * @date           : Aug 31, 2023
  * @copyright      : 2023 Dekimo Goes
  ******************************************************************************
  */

#ifndef COMMON_COMMON_H_
#define COMMON_COMMON_H_

typedef enum {
  BACKUP_REGISTER_SECONDS,
  BACKUP_REGISTER_SUBSECONDS,
  BACKUP_REGISTER_MSBTICKS,
  BACKUP_REGISTER_OLDEST_LOG,
  BACKUP_REGISTER_LATEST_LOG,

} ENUM_backupRegister;


uint32_t getResetSource(void);
bool powerOnReset(void);
void writeBackupRegister(ENUM_backupRegister backupRegisterId, uint32_t value);
uint32_t readBackupRegister(ENUM_backupRegister backupRegisterId);

#endif /* COMMON_COMMON_H_ */
