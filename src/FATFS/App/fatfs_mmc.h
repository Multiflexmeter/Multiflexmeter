#ifndef __FATFS_MMC_H
#define __FATFS_MCC_H

/* MMC/SD command */
#define CMD0  (0)     /* GO_IDLE_STATE */
#define CMD1  (1)     /* SEND_OP_COND (MMC) */
#define ACMD41  (0x80+41) /* SEND_OP_COND (SDC) */
#define CMD8  (8)     /* SEND_IF_COND */
#define CMD9  (9)     /* SEND_CSD */
#define CMD10 (10)    /* SEND_CID */
#define CMD12 (12)    /* STOP_TRANSMISSION */
#define ACMD13  (0x80+13) /* SD_STATUS (SDC) */
#define CMD16 (16)    /* SET_BLOCKLEN */
#define CMD17 (17)    /* READ_SINGLE_BLOCK */
#define CMD18 (18)    /* READ_MULTIPLE_BLOCK */
#define CMD23 (23)    /* SET_BLOCK_COUNT (MMC) */
#define ACMD23  (0x80+23) /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24 (24)    /* WRITE_BLOCK */
#define CMD25 (25)    /* WRITE_MULTIPLE_BLOCK */
#define CMD32 (32)    /* ERASE_ER_BLK_START */
#define CMD33 (33)    /* ERASE_ER_BLK_END */
#define CMD38 (38)    /* ERASE */
#define CMD55 (55)    /* APP_CMD */
#define CMD58 (58)    /* READ_OCR */

/* MMC card type flags (MMC_GET_TYPE) */
#define CT_MMC3   0x01    /* MMC ver 3 */
#define CT_MMC4   0x02    /* MMC ver 4+ */
#define CT_MMC    0x03    /* MMC */
#define CT_SDC1   0x02    /* SDC ver 1 */
#define CT_SDC2   0x04    /* SDC ver 2+ */
#define CT_SDC    0x0C    /* SDC */
#define CT_BLOCK  0x10    /* Block addressing */

#if FF_FS_EXFAT
#if FF_INTDEF != 2
#error exFAT feature wants C99 or later
#endif
typedef QWORD FSIZE_t;
#if FF_LBA64
typedef uint64_t LBA_t;
#else
typedef uint64_t LBA_t;
#endif
#else
#if FF_LBA64
#error exFAT needs to be enabled when enable 64-bit LBA
#endif
typedef uint32_t FSIZE_t;
typedef uint32_t LBA_t;
#endif

/* Functions */
DSTATUS SD_disk_initialize (uint8_t pdrv);
DSTATUS SD_disk_status (uint8_t pdrv);
DRESULT SD_disk_read (uint8_t pdrv, uint8_t* buff, LBA_t sector, uint16_t count);
DRESULT SD_disk_write (uint8_t pdrv, const BYTE* buff, LBA_t sector, uint16_t count);
DRESULT SD_disk_ioctl (uint8_t pdrv, uint8_t cmd, void* buff);

//#define SPI_TIMEOUT 100
#define SPI_TIMEOUT 1000

extern SPI_HandleTypeDef  hspi1;
#define HSPI_SDCARD     &hspi1
#define SD_CS_PORT      GPIOB
#define SD_CS_PIN     GPIO_PIN_4


#endif
