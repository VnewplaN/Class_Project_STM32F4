/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.c
  * @brief   This file includes a diskio driver skeleton to be completed by the user.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/*
 * Warning: the user section 0 is no more in use (starting from CubeMx version 4.16.0)
 * To be suppressed in the future.
 * Kept to ensure backward compatibility with previous CubeMx versions when
 * migrating projects.
 * User code previously added there should be copied in the new user sections before
 * the section contents can be deleted.
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */
#include "main.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* SPI + CS pin used for SD */
#define SD_CS_GPIO_Port GPIOB
#define SD_CS_Pin GPIO_PIN_12
#define SD_SPI_TIMEOUT 100U

extern SPI_HandleTypeDef hspi2;

/* MMC/SD command set */
#define CMD0   (0x40U+0)   /* GO_IDLE_STATE */
#define CMD1   (0x40U+1)   /* SEND_OP_COND (MMC) */
#define ACMD41 (0xC0U+41)  /* SEND_OP_COND (SDC) */
#define CMD8   (0x40U+8)   /* SEND_IF_COND */
#define CMD9   (0x40U+9)   /* SEND_CSD */
#define CMD10  (0x40U+10)  /* SEND_CID */
#define CMD12  (0x40U+12)  /* STOP_TRANSMISSION */
#define CMD16  (0x40U+16)  /* SET_BLOCKLEN */
#define CMD17  (0x40U+17)  /* READ_SINGLE_BLOCK */
#define CMD24  (0x40U+24)  /* WRITE_BLOCK */
#define CMD55  (0x40U+55)  /* APP_CMD */
#define CMD58  (0x40U+58)  /* READ_OCR */

/* Card type flags (CardType) */
#define CT_MMC    0x01U
#define CT_SD1    0x02U
#define CT_SD2    0x04U
#define CT_SDC    (CT_SD1|CT_SD2)
#define CT_BLOCK  0x08U

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ff_gen_drv.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;
static BYTE CardType = 0;

/* USER CODE BEGIN Private */
static void SD_Select(void);
static void SD_Deselect(void);
static uint8_t SD_TxRx(uint8_t data);
static void SD_SendClock(void);
static int SD_WaitReady(uint32_t timeoutMs);
static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg);
static DSTATUS SD_PowerOn(void);
static DRESULT SD_ReadSectors(BYTE *buff, DWORD sector, UINT count);
static DRESULT SD_WriteSectors(const BYTE *buff, DWORD sector, UINT count);

static void SD_Select(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
}

static void SD_Deselect(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
  SD_TxRx(0xFF);
}

static uint8_t SD_TxRx(uint8_t data)
{
  uint8_t rx = 0xFF;
  HAL_SPI_TransmitReceive(&hspi2, &data, &rx, 1, SD_SPI_TIMEOUT);
  return rx;
}

static void SD_SendClock(void)
{
  SD_TxRx(0xFF);
}

static int SD_WaitReady(uint32_t timeoutMs)
{
  uint32_t start = HAL_GetTick();
  uint8_t resp;
  do {
    resp = SD_TxRx(0xFF);
    if (resp == 0xFF) {
      return 1;
    }
  } while ((HAL_GetTick() - start) < timeoutMs);
  return 0;
}

static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg)
{
  uint8_t res;

  if (cmd & 0x80) {
    cmd &= 0x7F;
    res = SD_SendCmd(CMD55, 0);
    if (res > 1) {
      return res;
    }
  }

  SD_Deselect();
  SD_Select();

  if (!SD_WaitReady(500)) {
    SD_Deselect();
    return 0xFF;
  }

  SD_TxRx(0xFF);

  SD_TxRx(cmd);
  SD_TxRx((uint8_t)(arg >> 24));
  SD_TxRx((uint8_t)(arg >> 16));
  SD_TxRx((uint8_t)(arg >> 8));
  SD_TxRx((uint8_t)arg);

  uint8_t crc = 0x01;
  if (cmd == CMD0) crc = 0x95;
  if (cmd == CMD8) crc = 0x87;
  SD_TxRx(crc);

  if (cmd == CMD12) {
    SD_TxRx(0xFF);
  }

  uint8_t n = 10;
  do {
    res = SD_TxRx(0xFF);
  } while ((res & 0x80U) && --n);

  return res;
}

static DSTATUS SD_PowerOn(void)
{
  CardType = 0;
  SD_Deselect();
  for (uint8_t i = 0; i < 10; i++) {
    SD_SendClock();
  }

  uint8_t ty = 0;
  if (SD_SendCmd(CMD0, 0) == 1) {
    if (SD_SendCmd(CMD8, 0x1AA) == 1) {
      uint8_t ocr[4];
      ocr[0] = SD_TxRx(0xFF);
      ocr[1] = SD_TxRx(0xFF);
      ocr[2] = SD_TxRx(0xFF);
      ocr[3] = SD_TxRx(0xFF);
      if (ocr[2] == 0x01 && ocr[3] == 0xAA) {
        for (uint32_t t = 1000; t; t--) {
          if (SD_SendCmd(ACMD41, 1UL << 30) == 0) break;
          HAL_Delay(1);
        }
        if (SD_SendCmd(CMD58, 0) == 0) {
          uint8_t ocr2[4];
          ocr2[0] = SD_TxRx(0xFF);
          ocr2[1] = SD_TxRx(0xFF);
          ocr2[2] = SD_TxRx(0xFF);
          ocr2[3] = SD_TxRx(0xFF);
          ty = (ocr2[0] & 0x40U) ? (CT_SD2 | CT_BLOCK) : CT_SD2;
        }
      }
    } else {
      uint8_t cmd;
      if (SD_SendCmd(ACMD41, 0) <= 1) {
        ty = CT_SD1;
        cmd = ACMD41;
      } else {
        ty = CT_MMC;
        cmd = CMD1;
      }
      for (uint32_t t = 1000; t; t--) {
        if (SD_SendCmd(cmd, 0) == 0) break;
        HAL_Delay(1);
      }
      if (SD_SendCmd(CMD16, 512) != 0) {
        ty = 0;
      }
    }
  }

  CardType = ty;
  SD_Deselect();
  SD_TxRx(0xFF);

  if (ty) {
    Stat &= ~STA_NOINIT;
  } else {
    Stat = STA_NOINIT;
  }
  return Stat;
}

static DRESULT SD_ReadSectors(BYTE *buff, DWORD sector, UINT count)
{
  if (!(CardType & CT_BLOCK)) {
    sector *= 512U;
  }

  for (UINT i = 0; i < count; i++) {
    if (SD_SendCmd(CMD17, sector + i) != 0) {
      SD_Deselect();
      return RES_ERROR;
    }

    uint32_t start = HAL_GetTick();
    uint8_t token;
    do {
      token = SD_TxRx(0xFF);
    } while (token == 0xFF && (HAL_GetTick() - start) < 200);

    if (token != 0xFE) {
      SD_Deselect();
      return RES_ERROR;
    }

    for (uint16_t j = 0; j < 512; j++) {
      buff[j + (i * 512)] = SD_TxRx(0xFF);
    }
    SD_TxRx(0xFF); // CRC
    SD_TxRx(0xFF);
  }
  SD_Deselect();
  return RES_OK;
}

static DRESULT SD_WriteSectors(const BYTE *buff, DWORD sector, UINT count)
{
  if (!(CardType & CT_BLOCK)) {
    sector *= 512U;
  }

  for (UINT i = 0; i < count; i++) {
    if (SD_SendCmd(CMD24, sector + i) != 0) {
      SD_Deselect();
      return RES_ERROR;
    }

    if (!SD_WaitReady(500)) {
      SD_Deselect();
      return RES_NOTRDY;
    }

    SD_TxRx(0xFE);
    for (uint16_t j = 0; j < 512; j++) {
      SD_TxRx(buff[j + (i * 512)]);
    }
    SD_TxRx(0xFF); // CRC
    SD_TxRx(0xFF);

    uint8_t resp = SD_TxRx(0xFF);
    if ((resp & 0x1FU) != 0x05U) {
      SD_Deselect();
      return RES_ERROR;
    }

    if (!SD_WaitReady(500)) {
      SD_Deselect();
      return RES_ERROR;
    }
  }
  SD_Deselect();
  return RES_OK;
}
/* USER CODE END Private */

/* USER CODE END DECL */

/* Private function prototypes -----------------------------------------------*/
DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif /* _USE_IOCTL == 1 */

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE
  USER_write,
#endif  /* _USE_WRITE == 1 */
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_initialize (
	BYTE pdrv           /* Physical drive nmuber to identify the drive */
)
{
  /* USER CODE BEGIN INIT */
  if (pdrv != 0) {
    return STA_NOINIT;
  }
  return SD_PowerOn();
  /* USER CODE END INIT */
}

/**
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_status (
	BYTE pdrv       /* Physical drive number to identify the drive */
)
{
  /* USER CODE BEGIN STATUS */
  if (pdrv != 0) {
    return STA_NOINIT;
  }
  return Stat;
  /* USER CODE END STATUS */
}

/**
  * @brief  Reads Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT USER_read (
	BYTE pdrv,      /* Physical drive nmuber to identify the drive */
	BYTE *buff,     /* Data buffer to store read data */
	DWORD sector,   /* Sector address in LBA */
	UINT count      /* Number of sectors to read */
)
{
  /* USER CODE BEGIN READ */
  if (pdrv != 0 || count == 0) {
    return RES_PARERR;
  }
  if (Stat & STA_NOINIT) {
    return RES_NOTRDY;
  }
  return SD_ReadSectors(buff, sector, count);
  /* USER CODE END READ */
}

/**
  * @brief  Writes Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT USER_write (
	BYTE pdrv,          /* Physical drive nmuber to identify the drive */
	const BYTE *buff,   /* Data to be written */
	DWORD sector,       /* Sector address in LBA */
	UINT count          /* Number of sectors to write */
)
{
  /* USER CODE BEGIN WRITE */
  if (pdrv != 0 || count == 0) {
    return RES_PARERR;
  }
  if (Stat & STA_NOINIT) {
    return RES_NOTRDY;
  }
  return SD_WriteSectors(buff, sector, count);
  /* USER CODE END WRITE */
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT USER_ioctl (
	BYTE pdrv,      /* Physical drive nmuber (0..) */
	BYTE cmd,       /* Control code */
	void *buff      /* Buffer to send/receive control data */
)
{
  /* USER CODE BEGIN IOCTL */
  if (pdrv != 0) {
    return RES_PARERR;
  }
  if (Stat & STA_NOINIT) {
    return RES_NOTRDY;
  }

  DRESULT res = RES_ERROR;
  switch (cmd) {
    case CTRL_SYNC:
      if (SD_WaitReady(500)) res = RES_OK;
      SD_Deselect();
      break;
    case GET_SECTOR_SIZE:
      *(WORD*)buff = 512;
      res = RES_OK;
      break;
    case GET_BLOCK_SIZE:
      *(DWORD*)buff = 1;
      res = RES_OK;
      break;
    case GET_SECTOR_COUNT:
      *(DWORD*)buff = 0; /* Unknown without CSD parse */
      res = RES_OK;
      break;
    default:
      res = RES_PARERR;
      break;
  }
  return res;
  /* USER CODE END IOCTL */
}
#endif /* _USE_IOCTL == 1 */

