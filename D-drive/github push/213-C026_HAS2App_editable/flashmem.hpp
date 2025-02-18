/********************************************************************************
;*																				*
;*		Copyright (C) 2013 PAL Giken Co.,Ltd. All Rights Reserved				*
;*																				*
;********************************************************************************
;*																				*
;*		開発製品名	:	事故防止ﾕﾆｯﾄ											*
;*																				*
;*		親　図　番	:	113-B092												*
;*																				*
;*		ｿﾌﾄ管理番号	:	SP0117													*
;*																				*
;*		ﾓｼﾞｭｰﾙ名	:	configmem.h											*
;*																				*
;*		作　成　者	:	内之浦 伸治												*
;*																				*
;*		Environment	:	CPU		: AM3352 cortex-A8 <ARM>						*
;*						CLOCK   : 800MHz										*
;*						動作ﾓｰﾄﾞ:												*
;*																				*
;*		IDE			:	Code Composer Studio Version: 5.5.0.00077				*
;*		ｺﾝﾊﾟｲﾗ		:	TI v5.1.1												*
;*		ﾗｲﾌﾞﾗﾘﾌｧｲﾙ	:	NORTi Version 4 (ARM/CCS)  Release 4.3J					*
;*										ｶｰﾈﾙ	   Version 4.39					*
;*										TCP/IP	   Version 4.39					*
;*																				*
;*																				*
;*		＊　＊　＊　＊　変更来歴（設計来歴）　＊　＊　＊　＊					*
;*																				*
;*	--------------------------------------------------------------------		*
;*	| Version |  作成日  |  変 更 者  | 　変更関数名　・　変更内容     |		*
;*	--------------------------------------------------------------------		*
;*	| Ver.0.01|2013/11/03|  内 之 浦  |●初版                          |		*
;*	|         |          |            |･spi_configmem.c ﾍｯﾀﾞﾓｼﾞｭｰﾙ     |		*
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
extern int spi_flash_init(int fd);										// FPGAコンフィグメモリ（SPIフラッシュ）更新初期設定処理
extern int error_check(int fd,char *s);
extern void pabort(const char *s);
extern bool spi_transfer(int fd, T_SPI_CTRL obj);
extern bool activate_spiflash(int fd);
extern bool change_spiflash(int fd);									// SPI FLASH チップイレース処理
extern bool update_rom(int fd, const ulong src, const ulong size, const ulong dis);

/*===	global variable   ===*/


/*===	port number definition   ===*/


/*===	Memory Map Address definition   ===*/

/*===	structure of External I/O Register   ===*/

/*===	External I/O Register address definition   ===*/


#endif	/* __SPI_FLASHMEM_H_ */



