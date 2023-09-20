/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   app_fatfs.c
  * @brief  Code for fatfs applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_fatfs.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  APPLICATION_IDLE = 0,
  APPLICATION_INIT,
  APPLICATION_RUNNING,
  APPLICATION_SD_UNPLUGGED,
}FS_FileOperationsTypeDef;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFFER_SIZE 32
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
FATFS USERFatFs;    /* File system object for USER logical drive */
FIL USERFile;       /* File  object for USER */
char USERPath[4];   /* USER logical drive path */
/* USER CODE BEGIN PV */
FS_FileOperationsTypeDef Appli_state = APPLICATION_IDLE;
char buffer[32];  // to store strings..
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/**
  * @brief  FatFs initialization
  * @param  None
  * @retval Initialization result
  */
int32_t MX_FATFS_Init(void)
{
  /*## FatFS: Link the disk I/O driver(s)  ###########################*/
  if (FATFS_LinkDriver(&USER_Driver, USERPath) != 0)
  /* USER CODE BEGIN FATFS_Init */
  {
    return APP_ERROR;
  }
  else
  {
    Appli_state = APPLICATION_INIT;
    return APP_OK;
  }
  /* USER CODE END FATFS_Init */
}

/**
  * @brief  FatFs application main process
  * @param  None
  * @retval Process result
  */
int32_t MX_FATFS_Process(void)
{
  /* USER CODE BEGIN FATFS_Process */
  int32_t process_res = APP_OK;

  return process_res;
  /* USER CODE END FATFS_Process */
}

/**
  * @brief  Gets Time from RTC (generated when FS_NORTC==0; see ff.c)
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
  return 0;
  /* USER CODE END get_fattime */
}

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN Application */
void clear_buffer (void)
{
  for (int i=0; i<BUFFER_SIZE; i++) buffer[i] = '\0';
}

int8_t SD_TEST(void)
{
  FATFS fs;
  FIL file;
  FRESULT fres; //Result after operations
  char teststring[BUFFER_SIZE] = "Test string";
  char buffer[BUFFER_SIZE];  // to store strings..

  // Mount the filesystem
  fres = f_mount(&fs, "/", 1);
  if (fres != FR_OK)
  {
    return -1;
  }

  // Write the test string to the file
  fres= f_open(&file, "write.txt", FA_WRITE | FA_CREATE_ALWAYS);
  if (fres != FR_OK)
  {
    return -1;
  }

  f_puts(teststring, &file);
  f_close(&file);

  // read the test string from the file
  fres= f_open(&file, "write.txt", FA_READ);
  if (fres != FR_OK)
  {
    return -1;
  }

  clear_buffer();
  f_read(&file, buffer, f_size(&file), NULL);

  // compare the content of the file to the test string
  uint8_t result = strcmp(buffer, teststring);

  // close file and remove it
  f_close(&file);
  f_unlink("/write.txt");
  f_mount(NULL, "/", 0);

  return result;
}
/* USER CODE END Application */
