/*------------------------------------------------------------------------*/
/* STM32F100: MMCv3/SDv1/SDv2 (SPI mode) control module                   */
/*------------------------------------------------------------------------*/
/*
/  Copyright (C) 2018, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/

/* modified by Dekimo Goes 28-09-2023
 * - use HAL SPI library
 * -
 */




/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/

#include "main.h"

#include "spi.h"
#include "diskio.h"
#include "fatfs_mmc.h"

#define MMC_WP    0 /* Write protected (yes:true, no:false, default:false) */
#define MMC_CD    1 /* Card detect (yes:true, no:false, default:true) */


static volatile DSTATUS Stat = STA_NOINIT;	/* Physical drive status */
static volatile uint16_t Timer1, Timer2;		/* 1kHz decrement timer stopped at zero (disk_timerproc()) */

static uint8_t CardType;	/* Card type flags */
static uint8_t PowerFlag = 0;       /* Power flag */

#define CS_HIGH() HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET)
#define CS_LOW() HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET)

#define FCLK_SLOW() MX_SPI1_lowSpeed()
#define FCLK_FAST() MX_SPI1_highSpeed()

/*-----------------------------------------------------------------------*/
/* SPI controls (Platform dependent)                                     */
/*-----------------------------------------------------------------------*/


/* */
/**
 * @fn uint8_t xchg_spi(uint8_t)
 * @brief Exchange a byte
 *
 * @param dat : Data to send
 * @return readed data byte
 */
static uint8_t xchg_spi ( uint8_t dat	)
{
  uint8_t rxData;
  while (HAL_SPI_GetState(HSPI_SDCARD) != HAL_SPI_STATE_READY);
  HAL_SPI_TransmitReceive(HSPI_SDCARD, &dat, &rxData, 1, SPI_TIMEOUT);
  return rxData;
}


/**
 * @fn uint8_t SPI_RxByte(void)
 * @brief SPI receive a byte
 *
 * @return readed byte
 */
static uint8_t SPI_RxByte(void)
{
  uint8_t dummy, data;
  dummy = 0xFF;

  while (HAL_SPI_GetState(HSPI_SDCARD) != HAL_SPI_STATE_READY);
  HAL_SPI_TransmitReceive(HSPI_SDCARD, &dummy, &data, 1, SPI_TIMEOUT);

  return data;
}


/**
 * @fn void rcvr_spi_multi(uint8_t*, uint16_t)
 * @brief Receive multiple byte
 *
 * @param buff Pointer to data buffer
 * @param btr Number of bytes to receive (even number)
 */
static void rcvr_spi_multi(uint8_t *buff, uint16_t btr )
{
  while (btr--)
  {
    *buff++ = SPI_RxByte();
  }
}


#if FF_FS_READONLY == 0
/**
 * @fn void xmit_spi_multi(const uint8_t*, uint16_t)
 * @brief Send multiple byte
 *
 * @param buff : Pointer to the data
 * @param btx : Number of bytes to send (even number)
 */
static void xmit_spi_multi(const uint8_t *buff, uint16_t btx)
{
	do
  {
	  xchg_spi(*buff++);
  }
  while(--btx);

}
#endif


/**
 * @fn int wait_ready(uint16_t)
 * @brief Wait for card ready
 *
 * @param wt : Timeout [ms]
 * @return 1:Ready, 0:Timeout
 */
static int wait_ready(uint16_t wt)
{
  uint8_t d;


	Timer2 = wt;
	do {
		d = xchg_spi(0xFF);
		/* This loop takes a time. Insert rot_rdq() here for multitask envilonment. */
	} while (d != 0xFF && Timer2);	/* Wait for card goes ready or timeout */

	return (d == 0xFF) ? 1 : 0;
}


/**
 * @fn void deselect(void)
 * @brief Deselect card and release SPI
 *
 */
static void deselect (void)
{
	CS_HIGH();		/* Set CS# high */
	xchg_spi(0xFF);	/* Dummy clock (force DO hi-z for multiple slave SPI) */
}


/**
 * @fn int select(void)
 * @brief Select card and wait for ready
 *
 * @return
 */
static int select (void)	/* 1:OK, 0:Timeout */
{
	CS_LOW();		/* Set CS# low */
	xchg_spi(0xFF);	/* Dummy clock (force DO enabled) */
	if (wait_ready(500)) return 1;	/* Wait for card ready */

	deselect();
	return 0;	/* Timeout */
}


/***************************************
 * SD functions
 **************************************/

/**
 * @fn void SD_PowerOn(void)
 * @brief Power on
 *
 */
static void SD_PowerOn(void)
{
  uint8_t args[6];
  uint32_t cnt = 0x1FFF;

  /* transmit bytes to wake up */
  select();
  for(int i = 0; i < 10; i++)
  {
    xchg_spi(0xFF);
  }

  /* slave select */
  select();

  /* make idle state */
  args[0] = (CMD0 | 0x40);   /* CMD0:GO_IDLE_STATE */
  args[1] = 0;
  args[2] = 0;
  args[3] = 0;
  args[4] = 0;
  args[5] = 0x95;   /* CRC */

  xmit_spi_multi(args, sizeof(args));

  /* wait response */
  while ((SPI_RxByte() != 0x01) && cnt)
  {
    cnt--;
  }

  deselect();
  xchg_spi(0XFF);

  PowerFlag = 1;
}

/**
 * @fn void SD_PowerOff(void)
 * @brief power off
 *
 */
static void SD_PowerOff(void)
{
  PowerFlag = 0;
}

/**
 * @fn uint8_t SD_CheckPower(void)
 * @brief check power flag
 *
 * @return
 */
static uint8_t SD_CheckPower(void)
{
  return PowerFlag;
}


/*-----------------------------------------------------------------------*/
/*                                     */
/*-----------------------------------------------------------------------*/
/**
 * @fn int rcvr_datablock(uint8_t*, uint16_t)
 * @brief Receive a data packet from the MMC
 *
 * @param buff : Data buffer
 * @param btr : Data block length (byte)
 * @return 1:OK, 0:Error
 */
static int rcvr_datablock(uint8_t *buff, uint16_t btr)
{
  uint8_t token;


	Timer1 = 200;
	do {							/* Wait for DataStart token in timeout of 200ms */
		token = xchg_spi(0xFF);
		/* This loop will take a time. Insert rot_rdq() here for multitask envilonment. */
	} while ((token == 0xFF) && Timer1);
	if(token != 0xFE) return 0;		/* Function fails if invalid DataStart token or timeout */

	rcvr_spi_multi(buff, btr);		/* Store trailing data to the buffer */
	xchg_spi(0xFF); xchg_spi(0xFF);			/* Discard CRC */

	return 1;						/* Function succeeded */
}


#if FF_FS_READONLY == 0
/**
 * @fn int xmit_datablock(const uint8_t*, uint8_t)
 * @brief Send a data packet to the MMC
 *
 * @param buff : Ponter to 512 byte data to be sent
 * @param token : Token
 * @return 1:OK, 0:Failed
 */
static int xmit_datablock(const uint8_t *buff, uint8_t token)
{
  uint8_t resp;


	if (!wait_ready(500)) return 0;		/* Wait for card ready */

	xchg_spi(token);					/* Send token */
	if (token != 0xFD) {				/* Send data if token is other than StopTran */
		xmit_spi_multi(buff, 512);		/* Data */
		xchg_spi(0xFF); xchg_spi(0xFF);	/* Dummy CRC */

		resp = xchg_spi(0xFF);				/* Receive data resp */
		if ((resp & 0x1F) != 0x05) return 0;	/* Function fails if the data packet was not accepted */
	}
	return 1;
}
#endif


/*-----------------------------------------------------------------------*/
/* Send a command packet to the MMC                                      */
/*-----------------------------------------------------------------------*/
/**
 * @fn uint8_t send_cmd(uint8_t, uint32_t)
 * @brief Send a command packet to the MMC
 *
 * @param cmd : Command index
 * @param arg : Argument
 * @return Return value: R1 resp (bit7==1:Failed to send)
 */
static uint8_t send_cmd(uint8_t cmd, uint32_t arg)
{
	uint8_t n, res;


	if (cmd & 0x80) {	/* Send a CMD55 prior to ACMD<n> */
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1) return res;
	}

	/* Select the card and wait for ready except to stop multiple block read */
	if (cmd != CMD12) {
		deselect();
		if (!select()) return 0xFF;
	}

	/* Send command packet */
	xchg_spi(0x40 | cmd);				/* Start + command index */
	xchg_spi((uint8_t)(arg >> 24));		/* Argument[31..24] */
	xchg_spi((uint8_t)(arg >> 16));		/* Argument[23..16] */
	xchg_spi((uint8_t)(arg >> 8));			/* Argument[15..8] */
	xchg_spi((uint8_t)arg);				/* Argument[7..0] */
	n = 0x01;							/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	xchg_spi(n);

	/* Receive command resp */
	if (cmd == CMD12) xchg_spi(0xFF);	/* Diacard following one byte when CMD12 */
	n = 10;								/* Wait for response (10 bytes max) */
	do {
		res = xchg_spi(0xFF);
	} while ((res & 0x80) && --n);

	return res;							/* Return received response */
}



/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/

/**
 * @fn DSTATUS SD_disk_initialize(uint8_t)
 * @brief Initialize disk drive
 *
 * @param drv Physical drive number (0)
 * @return
 */
DSTATUS SD_disk_initialize(uint8_t drv)
{
	uint8_t n, cmd, ty, ocr[4];


	if (drv) return STA_NOINIT;			/* Supports only drive 0 */


	if (Stat & STA_NODISK) return Stat;	/* Is card existing in the soket? */


	FCLK_SLOW();

 /* power on */
  SD_PowerOn();

  /* slave select */
  select();

  /* check disk type */
  CardType = 0;


	for (n = 10; n; n--) xchg_spi(0xFF);	/* Send 80 dummy clocks */

	ty = 0;
	if (send_cmd(CMD0, 0) == 1) {			/* Put the card SPI/Idle state */
		Timer1 = 1000;						/* Initialization timeout = 1 sec */
		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2? */
			for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);	/* Get 32 bit return value of R7 resp */
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {				/* Is the card supports vcc of 2.7-3.6V? */
				while (Timer1 && send_cmd(ACMD41, 1UL << 30)) ;	/* Wait for end of initialization with ACMD41(HCS) */
				if (Timer1 && send_cmd(CMD58, 0) == 0) {		/* Check CCS bit in the OCR */
					for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);
					ty = (ocr[0] & 0x40) ? CT_SDC2 | CT_BLOCK : CT_SDC2;	/* Card id SDv2 */
				}
			}
		} else {	/* Not SDv2 card */
			if (send_cmd(ACMD41, 0) <= 1) 	{	/* SDv1 or MMC? */
				ty = CT_SDC1; cmd = ACMD41;	/* SDv1 (ACMD41(0)) */
			} else {
				ty = CT_MMC3; cmd = CMD1;	/* MMCv3 (CMD1(0)) */
			}
			while (Timer1 && send_cmd(cmd, 0)) ;		/* Wait for end of initialization */
			if (!Timer1 || send_cmd(CMD16, 512) != 0)	/* Set block length: 512 */
				ty = 0;
		}
	}
	CardType = ty;	/* Card type */
	deselect();

	if (ty) {			/* OK */
		FCLK_FAST();			/* Set fast clock */
		Stat &= ~STA_NOINIT;	/* Clear STA_NOINIT flag */
	} else {			/* Failed */
		Stat = STA_NOINIT;
		SD_PowerOff();
	}

	return Stat;
}



/**
 * @fn DSTATUS SD_disk_status(uint8_t)
 * @brief Get disk status
 *
 * @param drv : Physical drive number (0)
 * @return
 */
DSTATUS SD_disk_status(uint8_t drv)
{
	if (drv) return STA_NOINIT;		/* Supports only drive 0 */

	return Stat;	/* Return disk status */
}



/**
 * @fn DRESULT SD_disk_read(uint8_t, uint8_t*, LBA_t, uint16_t)
 * @brief Read sector(s)
 *
 * @param drv : Physical drive number (0)
 * @param buff : Pointer to the data buffer to store read data
 * @param sector : Start sector number (LBA)
 * @param count : Number of sectors to read (1..128)
 * @return
 */
DRESULT SD_disk_read(uint8_t drv, uint8_t *buff, LBA_t sector, uint16_t count)
{
	uint32_t sect = (uint32_t)sector;


	if (drv || !count) return RES_PARERR;		/* Check parameter */
	if (Stat & STA_NOINIT) return RES_NOTRDY;	/* Check if drive is ready */

	if (!(CardType & CT_BLOCK)) sect *= 512;	/* LBA ot BA conversion (byte addressing cards) */

	if (count == 1) {	/* Single sector read */
		if ((send_cmd(CMD17, sect) == 0)	/* READ_SINGLE_BLOCK */
			&& rcvr_datablock(buff, 512)) {
			count = 0;
		}
	}
	else {				/* Multiple sector read */
		if (send_cmd(CMD18, sect) == 0) {	/* READ_MULTIPLE_BLOCK */
			do {
				if (!rcvr_datablock(buff, 512)) break;
				buff += 512;
			} while (--count);
			send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
		}
	}
	deselect();

	return count ? RES_ERROR : RES_OK;	/* Return result */
}


#if FF_FS_READONLY == 0
/**
 * @fn DRESULT SD_disk_write(uint8_t, const uint8_t*, LBA_t, uint16_t)
 * @brief Write sector(s)
 *
 * @param drv : Physical drive number (0)
 * @param buff : Ponter to the data to write
 * @param sector : Start sector number (LBA)
 * @param count : Number of sectors to write (1..128)
 * @return
 */
DRESULT SD_disk_write(uint8_t drv, const uint8_t *buff, LBA_t sector, uint16_t count)
{
	uint32_t sect = (uint32_t)sector;


	if (drv || !count) return RES_PARERR;		/* Check parameter */
	if (Stat & STA_NOINIT) return RES_NOTRDY;	/* Check drive status */
	if (Stat & STA_PROTECT) return RES_WRPRT;	/* Check write protect */

	if (!(CardType & CT_BLOCK)) sect *= 512;	/* LBA ==> BA conversion (byte addressing cards) */

	if (count == 1) {	/* Single sector write */
		if ((send_cmd(CMD24, sect) == 0)	/* WRITE_BLOCK */
			&& xmit_datablock(buff, 0xFE)) {
			count = 0;
		}
	}
	else {				/* Multiple sector write */
		if (CardType & CT_SDC) send_cmd(ACMD23, count);	/* Predefine number of sectors */
		if (send_cmd(CMD25, sect) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!xmit_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(0, 0xFD)) count = 1;	/* STOP_TRAN token */
		}
	}
	deselect();

	return count ? RES_ERROR : RES_OK;	/* Return result */
}
#endif


/*-----------------------------------------------------------------------*/
/*              */
/*-----------------------------------------------------------------------*/
/**
 * @fn DRESULT SD_disk_ioctl(uint8_t, uint8_t, void*)
 * @brief Miscellaneous drive controls other than data read/write
 *
 * @param drv Physical drive number (0)
 * @param cmd : Control command code
 * @param buff : Pointer to the conrtol data
 * @return
 */
DRESULT SD_disk_ioctl(uint8_t drv, uint8_t cmd, void *buff)
{
	DRESULT res;
	uint8_t n, csd[16];
	uint8_t * ptr = buff;
	uint32_t st, ed, csize;
	LBA_t *dp;

	if (drv) return RES_PARERR;					/* Check parameter */

	if (cmd == CTRL_POWER)
  {
    switch (*ptr)
    {
      case 0:
        SD_PowerOff(); /* Power Off */
        res = RES_OK;
        break;
      case 1:
        SD_PowerOn(); /* Power On */
        res = RES_OK;
        break;
      case 2:
        *(ptr + 1) = SD_CheckPower();
        res = RES_OK; /* Power Check */
        break;
      default:
        res = RES_PARERR;
    }
    return res;
  }

	if (Stat & STA_NOINIT) return RES_NOTRDY;	/* Check if drive is ready */

	res = RES_ERROR;

	switch (cmd) {
	case CTRL_SYNC :		/* Wait for end of internal write process of the drive */
		if (select()) res = RES_OK;
		break;

	case GET_SECTOR_COUNT :	/* Get drive capacity in unit of sector (uint32_t) */
		if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
			if ((csd[0] >> 6) == 1) {	/* SDC CSD ver 2 */
				csize = csd[9] + ((WORD)csd[8] << 8) + ((uint32_t)(csd[7] & 63) << 16) + 1;
				*(LBA_t*)buff = csize << 10;
			} else {					/* SDC CSD ver 1 or MMC */
				n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
				csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
				*(LBA_t*)buff = csize << (n - 9);
			}
			res = RES_OK;
		}
		break;

	case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (uint32_t) */
		if (CardType & CT_SDC2) {	/* SDC ver 2+ */
			if (send_cmd(ACMD13, 0) == 0) {	/* Read SD status */
				xchg_spi(0xFF);
				if (rcvr_datablock(csd, 16)) {				/* Read partial block */
					for (n = 64 - 16; n; n--) xchg_spi(0xFF);	/* Purge trailing data */
					*(uint32_t*)buff = 16UL << (csd[10] >> 4);
					res = RES_OK;
				}
			}
		} else {					/* SDC ver 1 or MMC */
			if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {	/* Read CSD */
				if (CardType & CT_SDC1) {	/* SDC ver 1.XX */
					*(uint32_t*)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
				} else {					/* MMC */
					*(uint32_t*)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
				}
				res = RES_OK;
			}
		}
		break;

	case CTRL_TRIM :	/* Erase a block of sectors (used when _USE_ERASE == 1) */
		if (!(CardType & CT_SDC)) break;				/* Check if the card is SDC */
		if (disk_ioctl(drv, MMC_GET_CSD, csd)) break;	/* Get CSD */
		if (!(csd[10] & 0x40)) break;					/* Check if ERASE_BLK_EN = 1 */
		dp = buff; st = (uint32_t)dp[0]; ed = (uint32_t)dp[1];	/* Load sector block */
		if (!(CardType & CT_BLOCK)) {
			st *= 512; ed *= 512;
		}
		if (send_cmd(CMD32, st) == 0 && send_cmd(CMD33, ed) == 0 && send_cmd(CMD38, 0) == 0 && wait_ready(30000)) {	/* Erase sector block */
			res = RES_OK;	/* FatFs does not check result of this command */
		}
		break;

	/* Following commands are never used by FatFs module */

	case MMC_GET_TYPE:		/* Get MMC/SDC type (uint8_t) */
		*(uint8_t*)buff = CardType;
		res = RES_OK;
		break;

	case MMC_GET_CSD:		/* Read CSD (16 bytes) */
		if (send_cmd(CMD9, 0) == 0 && rcvr_datablock((uint8_t*)buff, 16)) {	/* READ_CSD */
			res = RES_OK;
		}
		break;

	case MMC_GET_CID:		/* Read CID (16 bytes) */
		if (send_cmd(CMD10, 0) == 0 && rcvr_datablock((uint8_t*)buff, 16)) {	/* READ_CID */
			res = RES_OK;
		}
		break;

	case MMC_GET_OCR:		/* Read OCR (4 bytes) */
		if (send_cmd(CMD58, 0) == 0) {	/* READ_OCR */
			for (n = 0; n < 4; n++) *(((uint8_t*)buff) + n) = xchg_spi(0xFF);
			res = RES_OK;
		}
		break;

	case MMC_GET_SDSTAT:	/* Read SD status (64 bytes) */
		if (send_cmd(ACMD13, 0) == 0) {	/* SD_STATUS */
			xchg_spi(0xFF);
			if (rcvr_datablock((uint8_t*)buff, 64)) res = RES_OK;
		}
		break;

	default:
		res = RES_PARERR;
	}

	deselect();

	return res;
}


/**
 * @fn void disk_timerproc(void)
 * @brief Device timer function
 * @note This function must be called from timer interrupt routine in period of 1 ms to generate card control timing.
 */
void disk_timerproc (void)
{
	WORD n;
	uint8_t s;


	n = Timer1;						/* 1kHz decrement timer stopped at 0 */
	if (n) Timer1 = --n;
	n = Timer2;
	if (n) Timer2 = --n;

	s = Stat;
	if (MMC_WP) {	/* Write protected */
		s |= STA_PROTECT;
	} else {		/* Write enabled */
		s &= ~STA_PROTECT;
	}
	if (MMC_CD) {	/* Card is in socket */
		s &= ~STA_NODISK;
	} else {		/* Socket empty */
		s |= (STA_NODISK | STA_NOINIT);
	}
	Stat = s;
}
