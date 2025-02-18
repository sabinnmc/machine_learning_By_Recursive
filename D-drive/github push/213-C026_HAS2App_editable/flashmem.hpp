/********************************************************************************
;*																				*
;*		Copyright (C) 2013 PAL Giken Co.,Ltd. All Rights Reserved				*
;*																				*
;********************************************************************************
;*																				*
;*		�J�����i��	:	���̖h�~�Ư�											*
;*																				*
;*		�e�@�}�@��	:	113-B092												*
;*																				*
;*		��ĊǗ��ԍ�	:	SP0117													*
;*																				*
;*		Ӽޭ�ٖ�	:	configmem.h											*
;*																				*
;*		��@���@��	:	���V�Y �L��												*
;*																				*
;*		Environment	:	CPU		: AM3352 cortex-A8 <ARM>						*
;*						CLOCK   : 800MHz										*
;*						����Ӱ��:												*
;*																				*
;*		IDE			:	Code Composer Studio Version: 5.5.0.00077				*
;*		���߲�		:	TI v5.1.1												*
;*		ײ����̧��	:	NORTi Version 4 (ARM/CCS)  Release 4.3J					*
;*										����	   Version 4.39					*
;*										TCP/IP	   Version 4.39					*
;*																				*
;*																				*
;*		���@���@���@���@�ύX�����i�݌v�����j�@���@���@���@��					*
;*																				*
;*	--------------------------------------------------------------------		*
;*	| Version |  �쐬��  |  �� �X ��  | �@�ύX�֐����@�E�@�ύX���e     |		*
;*	--------------------------------------------------------------------		*
;*	| Ver.0.01|2013/11/03|  �� �V �Y  |������                          |		*
;*	|         |          |            |�spi_configmem.c ͯ��Ӽޭ��     |		*
;*	|         |          |            |                                |		*
;*	--------------------------------------------------------------------		*
;*	|         |          |            |                                |		*
;*	--------------------------------------------------------------------		*
;*	|         |          |            |                                |		*
;*	--------------------------------------------------------------------		*
;*																				*
;********************************************************************************/


#if	!defined(__SPI_FLASHMEM_H_)
#define __SPI_FLASHMEM_H_


/*===	file include   ===*/
#include "Sysdef.h"

/*===	Type defined   ===*/


/*===	user definition   ===*/

/* spi info            */
#define SPI_FLASH_BUS0				0
#define SPI_FLASH_BUS1				1
#define SPI_FLASH_CS0				0
#define SPI_FLASH_CS1				1

/* flash info           */
#define SPIFLASH_DEVSIZE			0x1000000
#define SPIFLASH_SECTORSIZE			(64*1024)
#define SPIFLASH_PAGESIZE			0x100

#define MAXTIME_ERASE				250000
#define MAXTIME_SECTORERASE			1000
#define MAXTIME_WRITE				5
#define MAXTIME_WRITESTATUS			8

/* flash command        */
#define I2C_EXTEND_IO				0x20
#define LSC_PROG_SPI    			0x3A
#define CMD_SPIFLASH_READID			0x9F
#define CMD_SPIFLASH_BULKERASE		0xC7
#define CMD_SPIFLASH_READ			0x03
#define CMD_SPIFLASH_WRENABLE		0x06
#define CMD_SPIFLASH_WRDISABLE		0x04
#define CMD_SPIFLASH_WRITE			0x02
#define CMD_SPIFLASH_ACTIVATE		0xA4
#define CMD_SPIFLASH_SCTERASE		0xD8
#define CMD_SPIFLASH_BULKERASE		0xC7
#define CMD_SPIFLASH_READSTATUS		0x05
#define CMD_SPIFLASH_FLAGREADSTATUS	0x70
#define WRITE_DISABLE				0
#define WRITE_ENABLE				1

typedef struct {
	ulong txlen;
	uchar *txbuf;
	ulong rxlen;
	uchar *rxbuf;
} T_SPI_CTRL;

/*===	array of status data   ===*/

/*===	structure of global variables   ===*/

/*===	global function prototypes   ===*/
extern int spi_flash_init(int fd);										// FPGA�R���t�B�O�������iSPI�t���b�V���j�X�V�����ݒ菈��
extern int error_check(int fd,char *s);
extern void pabort(const char *s);
extern bool spi_transfer(int fd, T_SPI_CTRL obj);
extern bool activate_spiflash(int fd);
extern bool change_spiflash(int fd);									// SPI FLASH �`�b�v�C���[�X����
extern bool update_rom(int fd, const ulong src, const ulong size, const ulong dis);

/*===	global variable   ===*/


/*===	port number definition   ===*/


/*===	Memory Map Address definition   ===*/

/*===	structure of External I/O Register   ===*/

/*===	External I/O Register address definition   ===*/


#endif	/* __SPI_FLASHMEM_H_ */



