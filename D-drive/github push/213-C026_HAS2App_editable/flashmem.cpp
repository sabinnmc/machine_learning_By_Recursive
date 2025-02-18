/********************************************************************************
;*																				*
;*		Copyright (C) 2013 PAL Giken Co.,Ltd. All Rights Reserved				*
;*																				*
;********************************************************************************
;*																				*
;*		開発製品名	:	事故防止ユニット											*
;*																				*
;*		親　図　番	:	113-B092												*
;*																				*
;*		ソフト管理番号	:	SP0117													*
;*																				*
;*		モジュール名	:	flashmem.c											*
;*																				*
;*		作　成　者	:	内之浦 伸治												*
;*																				*
;*		Environment	:	CPU		: AM3352 cortex-A8 <ARM>						*
;*						CLOCK   : 800MHz										*
;*						動作モード:												*
;*																				*
;*		IDE			:	Code Composer Studio Version: 5.5.0.00077				*
;*		コンパイラ		:	TI v5.1.1												*
;*		ライブラリファイル	:	NORTi Version 4 (ARM/CCS)  Release 4.3J					*
;*										カーネル	   Version 4.39					*
;*										TCP/IP	   Version 4.39					*
;*																				*
;*																				*
;*		＊　＊　＊　＊　変更来歴（設計来歴）　＊　＊　＊　＊					*
;*																				*
;*	--------------------------------------------------------------------		*
;*	| Version |  作成日  |  変 更 者  | 　変更関数名　・　変更内容     |		*
;*	--------------------------------------------------------------------		*
;*	| Ver.0.01|2013/11/03|  内 之 浦  |●初版                          |		*
;*	|         |          |            |・SPIフラッシュドライバモジュール           |		*
;*	|         |          |            | Macronix製 MX25L12835EMI-10G   |		*
;*	|         |          |            |                                |		*
;*	--------------------------------------------------------------------		*
;*	|         |          |            |                                |		*
;*	--------------------------------------------------------------------		*
;*	|         |          |            |                                |		*
;*	--------------------------------------------------------------------		*
;*																				*
;********************************************************************************/


/*===	file include		===*/
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdint.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <unistd.h>
#include "flashmem.hpp"
#include "Sysdef.h"
#include "LinuxUtility.hpp"

//#include <opencv2/core/utility.hpp>

/*===	SECTION NAME		===*/


/*===	user definition		===*/


/*===	external variable	===*/


/*===	external function	===*/

/*===	public function prototypes	===*/
int spi_flash_init(void);														// FPGAコンフィグメモリ（SPIフラッシュ）更新初期設定処理
int error_check(int fd);
bool spi_transfer(int fd, T_SPI_CTRL obj);
bool activate_spiflash(int fd);
bool sectorerase_spiflash(int fd, const ulong addr, const ulong size);
bool update_rom(int fd, const ulong src, const ulong size, const ulong dis);
bool write_spiflash(int fd, const ulong src, const ulong size, const ulong dis);
bool read_spiflash(int fd, const ulong src, ulong size);
bool verify_spiflash(int fd, const ulong src, const ulong size, const ulong dis);
int	writeenable_spiflash(int fd, int flg);										// SPI FLASH ライトイネーブル処理
bool write_enable_busy(int fd);
bool erase_busy(int fd);

/*===	static function prototypes	===*/

//static int	chkstatus_spiflash(const int, const int);						// SPI FLASH 状態(Busy)確認処理

/*===	static typedef variable		===*/


/*===	static variable		===*/
static int		Id_wait_tm = 0;                                           // ライトタイムアウト監視用インターバルタイマID
static const char *Device = "/dev/spidev1.0";
static const std::string DevicePathI2C1 = "/dev/i2c-1";                   //!< I2C-1のﾊﾟｽ
static uint8_t Mode = SPI_MODE_0;
static uint8_t Bits = 8;
static uint32_t Speed = 1000000;
static uint16_t Delay = 0;
static uint8_t Tx_SpiChange[8];                                           // SPI送信用
static uint8_t Tx_SpiCommand[8];                                          // 送信コマンド用
static uint8_t Rx_SpiData[8];                                             // 受信データ用
static uint8_t Tx_SpiChange_Reg[8];                                       // SPI送信用
static uint8_t Tx_SpiCommand_Reg[8];                                      // 送信コマンド用
static uint8_t Rx_SpiData_Reg[8];                                         // 受信データ用
struct spi_ioc_transfer SpiIocTr[2];                                      // SPIコマンド送信用
struct spi_ioc_transfer SpiIocTr_Reg[2];                                  // SPIコマンド送信用

/*===	constant variable	===*/


/*===	global variable		===*/
uchar tx_buff[4+SPIFLASH_PAGESIZE];

/*===	implementation		===*/

/*!-------------------------------------------------------------------          @n
	FUNCTION :  i2c_programn_low()                                              @n
	EFFECTS  :  拡張IO操作(PROGRAMN H→L)                                       @n
	ARGUMENT :                                                                  @n
				[I] device      制御BUS(I2C1)                                   @n
	RETURN   :                                                                  @n
				<true> 成功                                                     @n
				<false> 失敗                                                    @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
bool i2c_programn_low(const std::string& device){
	int i2cfd = 0;
	int i;

	uint8_t buffer[2];	             	 							// I2C-Write用のバッファを準備する. */
	if (buffer == NULL) {
		ERR("i2c_programn_low:can't buffer prepair");
		return false;
	}
	i2cfd = open(device.c_str(), O_RDONLY);	                     	 // ﾃﾞﾊﾞｲｽｵｰﾌﾟﾝ
	if (i2cfd == -1) {
		ERR("i2c_programn_low:can't open i2c-1");
		return false;
	}
	for (i = 0; i < 2; i++){
		if(i==0){
			buffer[0] = 0x06;                                 	 	 // 1バイト目にレジスタアドレスをセット. */
			buffer[1] = 0xFD;                                    	 // 2バイト目以降にデータをセット. */
		}else if(i == 1){
			buffer[0] = 0x02;                                 	 	 // 1バイト目にレジスタアドレスをセット. */
			buffer[1] = 0xFD;                                    	 // 2バイト目以降にデータをセット. */
		}

		struct i2c_msg message = { I2C_EXTEND_IO, 0, 3, buffer };	 // I2C-Writeメッセージを作成する. */
		struct i2c_rdwr_ioctl_data ioctl_data = { &message, 1 };

		if (ioctl(i2cfd, I2C_RDWR, &ioctl_data) != 1) {	             // I2C-Writeを行う. */
			ERR("i2c_programn_low:can't write");
			close(i2cfd);
			return false;
		}
	}
	close(i2cfd);
	return true;
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  spi_flash_init()                                                @n
	EFFECTS  :  SPIフラッシュ 更新初期設定処理                                  @n
	ARGUMENT :                                                                  @n
				[I] fd      制御BUS(SPIDEV1)                                    @n
	RETURN   :                                                                  @n
				< 1> 成功                                                       @n
				<-1> 失敗                                                       @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
int spi_flash_init(void)
{
	int fd;
	int ret=0;
	fd = open(Device, O_RDWR);
	if (fd < 0) {
		ERR("spi_flash_init:can't open spidev1.0");
		return -1;
	}

	ret = error_check(fd);
	if(ret < 0) {
		return -1;
	}

	SpiIocTr[0].tx_buf = (unsigned long)Tx_SpiChange;
	SpiIocTr[1].tx_buf = (unsigned long)Tx_SpiCommand;
	for(int i = 0; i < 1; i++){
		SpiIocTr[i].rx_buf = (unsigned long)Rx_SpiData;
		SpiIocTr[i].len = 1;
		SpiIocTr[i].speed_hz = Speed;
		SpiIocTr[i].delay_usecs = Delay;
		SpiIocTr[i].bits_per_word = Bits;
	}
	SpiIocTr_Reg[0].tx_buf = (unsigned long)Tx_SpiChange_Reg;
	SpiIocTr_Reg[0].tx_buf = (unsigned long)Tx_SpiCommand_Reg;
	for(int i = 0; i < 1; i++){
		SpiIocTr_Reg[i].rx_buf = (unsigned long)Rx_SpiData_Reg;
		SpiIocTr_Reg[i].len = 1;
		SpiIocTr_Reg[i].speed_hz = Speed;
		SpiIocTr_Reg[i].delay_usecs = Delay;
		SpiIocTr_Reg[i].bits_per_word = Bits;
	}

	if(Id_wait_tm == 0) Id_wait_tm = GetTimeMsec();								// ライトタイムアウト監視タイマ生成
	return fd;
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  error_check()                                                   @n
	EFFECTS  :  SPIフラッシュ 初期設定エラー処理                                @n
	ARGUMENT :                                                                  @n
				[I] fd      制御BUS(SPIDEV1)                                    @n
				[I] *s      ｴﾗｰﾒｯｾｰｼﾞ                                           @n
	RETURN   :                                                                  @n
				< 1> 成功                                                       @n
				<-1> 失敗                                                       @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
int error_check(int fd)
{
	int ret = 1;

	ret = ioctl(fd, SPI_IOC_WR_MODE, &Mode);                             // spi mode(write)
	if (ret == -1) {
		ERR("spi_flash_init:can't set spi mode");
		return ret;
	}

	ret = ioctl(fd, SPI_IOC_RD_MODE, &Mode);                             // spi mode(read)
	if (ret == -1) {
		ERR("spi_flash_init:can't get spi mode");
		return ret;
	}
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &Bits);                    // bits per word(write)
	if (ret == -1) {
		ERR("spi_flash_init:can't set bits per word");
		return ret;
	}
	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &Bits);                    // bits per word(read)
	if (ret == -1) {
		ERR("spi_flash_init:can't get bits per word");
		return ret;
	}
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &Speed);                    // max speed hz(write)
	if (ret == -1) {
		ERR("spi_flash_init:can't set max speed hz");
		return ret;
	}
	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &Speed);                    // max speed hz(read)
	if (ret == -1) {
		ERR("spi_flash_init:can't get max speed hz");
		return ret;
	}
	return ret;
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  update_lattice()                                                @n
	EFFECTS  :  SPIフラッシュ アップデート処理                                  @n
	ARGUMENT :                                                                  @n
				[I] file      パラメータ更新用ファイル                          @n
	RETURN   :                                                                  @n
				<true> 成功                                                     @n
				<false> 失敗                                                    @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
bool update_lattice(std::string file)
{
    int fd = 0;
	int ret,bitsize;
	bool err = true;
	ulong dis =0x00000;
	FILE *fp = NULL;
	uchar *bitdata;
	std::string mfpga_filename = FPGA_FLDR + file;
	if(i2c_programn_low(DevicePathI2C1) == false) return false;                     // PROGRAMNをLowにする

    // spiコマンドの初期化
	fd = spi_flash_init();
	if(fd < 0) {
		close(fd);
		return false;
	}

    // アクティベートキー送信
	if(activate_spiflash(fd) == false) return false;
	fp = fopen(mfpga_filename.c_str(), "r");
	if (fp == NULL) {
		ERR("can't open bitfile");
		return false;
	}

	// ファイルサイズ取得
   	fseek(fp, 0, SEEK_END);
   	bitsize = ftell(fp);
 	bitdata= (uchar *)malloc(sizeof(uchar)*bitsize);
   	fseek(fp, 0, SEEK_SET);

	// bitファイルロード
	ret = fread(bitdata,sizeof(uchar),bitsize,fp);
	if( ret < (int)sizeof(uchar) ){
		ERR("can't read bitfile");
		free(bitdata);
		fclose(fp);
		return false;
	}
	fclose(fp);

	// Latticeの書き換え開始
	if(update_rom(fd,(ulong)&bitdata[0],bitsize,dis) == false)
	{
		free(bitdata);
		close(fd);
		return false;
	}

	free(bitdata);
	close(fd);
	return err;
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  activate_spiflash()                                             @n
	EFFECTS  :  SPI FLASH 切り替え処理(アクティベート)                          @n
	ARGUMENT :                                                                  @n
				[I] fd      制御BUS(SPIDEV1)                                    @n
	RETURN   :                                                                  @n
				<true> 成功                                                     @n
				<false> 失敗                                                    @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
bool activate_spiflash(int fd)
{
	T_SPI_CTRL	spi;
	int			err  = true;
	uint8_t		cmd[4]={0};

	cmd[0] = CMD_SPIFLASH_ACTIVATE;
	cmd[1] = 0xC6;
	cmd[2] = 0xF4;
	cmd[3] = 0x8A;

	//SPIドライバ用バッファ設定
	spi.txlen = sizeof(cmd);
	spi.txbuf = cmd;

	//送信
	err = spi_transfer(fd,spi);
	if(err == false){
		ERR("activate_spiflash:can't activate");
	}
	return(err);
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  spi_transfer()                                                  @n
	EFFECTS  :  SPI FLASH データ送信処理                                        @n
	ARGUMENT :                                                                  @n
				[I] fd      制御BUS(SPIDEV1)                                    @n
				[I] obj     送信データの構造体                                  @n
	RETURN   :                                                                  @n
				<true> 成功                                                     @n
				<false> 失敗                                                    @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
bool spi_transfer(int fd, T_SPI_CTRL obj)
{
	// int time_out;
	int i;
	int err = true;
	unsigned char tx_buf[8] = { 0 };
	int ret;
	//uchar dummy[1];

	for(i = 0; i<(int)obj.txlen;i++) {
		tx_buf[i] = obj.txbuf[i];
	}

    SpiIocTr[0].tx_buf = (unsigned long)tx_buf;
    SpiIocTr[0].len = (int)obj.txlen;

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &SpiIocTr);
	if (ret < 1){
		ERR("can't send spi message");
		err = false;
	}

    return err;
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  writeenable_spiflash()                                          @n
	EFFECTS  :  SPI FLASH ライトイネーブル処理                                  @n
	ARGUMENT :                                                                  @n
				[I] fd      制御BUS(SPIDEV1)                                    @n
				[I] flg     フラッシュへの書き込み 有効/無効                    @n
	RETURN   :                                                                  @n
				< 1> 成功                                                       @n
				<-1> 失敗                                                       @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
int writeenable_spiflash(int fd, int flg)
{
	T_SPI_CTRL	spi;
	int			err = true ;
	uchar		cmd[5];

	//コマンド設定
	cmd[0] = 0x3A;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	cmd[3] = 0x00;
	if (flg) cmd[4] = CMD_SPIFLASH_WRENABLE;
	else     cmd[4] = CMD_SPIFLASH_WRDISABLE;

	//SPIドライバ用バッファ設定
	spi.txlen = sizeof(cmd);
	spi.txbuf = cmd;

	// 送信
	err = spi_transfer(fd,spi);
	if (err == false) {
		ERR("spi faild to write enable");
	}
	else {
		err = write_enable_busy(fd);
		if (err == false) {
			ERR("faild to write enable");
		}
	}
	return(err);
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  write_spiflash()                                                @n
	EFFECTS  :  SPI FLASH ﾗｲﾄ処理                                               @n
	ARGUMENT :                                                                  @n
				[I] fd       制御BUS(SPIDEV1)                                   @n
				[I] src      ﾗｲﾄ元ﾃﾞｰﾀ格納ｱﾄﾞﾚｽ                                 @n
				[I] size     ﾗｲﾄｻｲｽﾞ                                            @n
				[I] dis      ﾗｲﾄ先先頭ｱﾄﾞﾚｽ                                     @n
	RETURN   :                                                                  @n
				<true> 成功                                                     @n
				<false> 失敗                                                    @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
bool write_spiflash(int fd, const ulong src, const ulong size, const ulong dis)
{
	int			err   = true;
	int			page  = (int)((size + SPIFLASH_PAGESIZE -1) / SPIFLASH_PAGESIZE);
	ulong		wp;
	ulong		len;
	uchar		*rp;
	int			i,ret;
	uchar		address[4];

	//SPIﾗｲﾄｲﾈｰﾌﾞﾙ
	for (i=0; i<page; i++) {
		ret = writeenable_spiflash(fd,WRITE_ENABLE);
		if (ret != 1) {
			break;
		}
		//ｺﾏﾝﾄﾞ設定
		wp = dis + SPIFLASH_PAGESIZE * i;
		rp = (uchar *)(src + SPIFLASH_PAGESIZE * i);
		if (i < page -1) len = SPIFLASH_PAGESIZE;
		else             len = size - SPIFLASH_PAGESIZE * i;
		address[0] = LSC_PROG_SPI;
		address[1] = 0x00;
		address[2] = 0x00;
		address[3] = 0x00;
		tx_buff[0] = CMD_SPIFLASH_WRITE;
		tx_buff[1] = (uchar)((wp >> 16) & 0x000000FFL);
		tx_buff[2] = (uchar)((wp >>  8) & 0x000000FFL);
		tx_buff[3] = (uchar)((wp >>  0) & 0x000000FFL);
		memcpy(&tx_buff[4], rp, len);
		SpiIocTr[0].tx_buf = (unsigned long) address;
		SpiIocTr[0].len =4;
		SpiIocTr[1].tx_buf = (unsigned long) tx_buff;
		SpiIocTr[1].len =4+len;

		ret = ioctl(fd, SPI_IOC_MESSAGE(2), &SpiIocTr);
		if (ret < 1) {
			ERR("spi faild to write to device");
			err = false;
			break;
		}
		usleep(1200); // tPP Page Program Cycle Time
	}

	return(err);
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  read_spiflash()                                                 @n
	EFFECTS  :  SPI FLASH ﾘｰﾄﾞ処理                                              @n
	ARGUMENT :                                                                  @n
				[I] fd       制御BUS(SPIDEV1)                                   @n
				[I] src      ﾗｲﾄ元ﾃﾞｰﾀ格納ｱﾄﾞﾚｽ                                 @n
				[I] size     ﾗｲﾄｻｲｽﾞ                                            @n
	RETURN   :                                                                  @n
				<true> 成功                                                     @n
				<false> 失敗                                                    @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
bool read_spiflash(int fd, const ulong src, const ulong size, uchar *dis)
{
	int			err   = true;
	int			ret;
	uchar		address[4];
	uchar		cmd[4] = { 0 };
	uchar 		recvdata[SPIFLASH_PAGESIZE+4];

	//ｺﾏﾝﾄﾞ設定
	address[0] = LSC_PROG_SPI;
	address[1] = 0x00;
	address[2] = 0x00;
	address[3] = 0x00;
	cmd[0] = CMD_SPIFLASH_READ;
	cmd[1] = (uchar)((src >> 16) & 0x000000FFL);
	cmd[2] = (uchar)((src >>  8) & 0x000000FFL);
	cmd[3] = (uchar)((src >>  0) & 0x000000FFL);

	SpiIocTr[0].tx_buf =(unsigned long)address;
	SpiIocTr[1].tx_buf =(unsigned long)cmd;
	SpiIocTr[1].rx_buf =(unsigned long)recvdata;
   	SpiIocTr[0].len = 4;
   	SpiIocTr[1].len = 4+size;

	ret = ioctl(fd, SPI_IOC_MESSAGE(2), &SpiIocTr);
	if (ret < 1) {
		ERR("spi faild to read from device");
		err =false;
	}
	memcpy(dis, &recvdata[4], size);

	return(err);
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  sectorerase_spiflash()                                          @n
	EFFECTS  :  SPI FLASH セクター削除                                          @n
	ARGUMENT :                                                                  @n
				[I] fd       制御BUS(SPIDEV1)                                   @n
				[I] addr     ｲﾚｰｽ先頭ｱﾄﾞﾚｽ                                      @n
				[I] size     ﾗｲﾄｻｲｽﾞ                                            @n
	RETURN   :                                                                  @n
				<true> 成功                                                     @n
				<false> 失敗                                                    @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
bool sectorerase_spiflash(int fd,const ulong addr, const ulong size)
{
	T_SPI_CTRL	spi;
	int			err = true;
	int			sector;
	ulong		p;
	uchar		cmd[8];
	int			i;
	int 		timer;

	sector = (int)((addr + size + SPIFLASH_SECTORSIZE -1) / SPIFLASH_SECTORSIZE) - (int)(addr / SPIFLASH_SECTORSIZE);
	for (i=0; i<sector; i++) {
		//SPIﾗｲﾄｲﾈｰﾌﾞﾙ
		writeenable_spiflash(fd,WRITE_ENABLE);
		//ｺﾏﾝﾄﾞ設定
		p = addr + SPIFLASH_SECTORSIZE * i;

		cmd[0] = LSC_PROG_SPI;
		cmd[1] = 0;
		cmd[2] = 0;
		cmd[3] = 0;
		cmd[4] = CMD_SPIFLASH_SCTERASE;
		cmd[5] = (uchar)((p >> 16) & 0x000000FFL);
		cmd[6] = (uchar)((p >>  8) & 0x000000FFL);
		cmd[7] = (uchar)((p >>  0) & 0x000000FFL);
		spi.txlen = sizeof(cmd);
		spi.txbuf = cmd;

		err = spi_transfer(fd,spi);
		timer = erase_busy(fd);
		if(timer != 1)err = false;
		if (err == false) {
			ERR("spi faild to sector erase");
			break;
		}
		sleep(1); //tBE Block Erase Cycle Time
	}

	return(err);
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  verify_spiflash()                                               @n
	EFFECTS  :  SPI FLASH ﾍﾞﾘﾌｧｲ処理                                            @n
	ARGUMENT :                                                                  @n
				[I] fd       制御BUS(SPIDEV1)                                   @n
				[I] src      ﾗｲﾄﾃﾞｰﾀ格納ｱﾄﾞﾚｽ                                   @n
				[I] dst      ﾘｰﾄﾞﾃﾞｰﾀ格納ｱﾄﾞﾚｽ                                  @n
				[I] size     ﾍﾞﾘﾌｧｲｻｲｽﾞ                                         @n
	RETURN   :                                                                  @n
				<true> 成功                                                     @n
				<false> 失敗                                                    @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
bool verify_spiflash(int fd, const ulong src, const ulong size, const ulong dis)
{
	uchar rx_buff[SPIFLASH_PAGESIZE];
	int			err   = true;
	int			page  = (int)((size + SPIFLASH_PAGESIZE -1) / SPIFLASH_PAGESIZE);
	uchar* 		p0;
	ulong		p1;
	int			len;
	int			i,j;

	for (i=0; i<page; i++) {
		//ｺﾏﾝﾄﾞ設定
		p0 = (uchar *)(src + SPIFLASH_PAGESIZE * i);
		p1 = (ulong)(dis + SPIFLASH_PAGESIZE * i);
		if (i < page -1) len = (int)(SPIFLASH_PAGESIZE);
		else             len = (int)(size - SPIFLASH_PAGESIZE * i);
		if (read_spiflash(fd, p1, len, rx_buff) == true) {
			for (j=0; j<len; j++) {
				if (p0[j] != rx_buff[j]) {
					ERR("verify ng at %08lX:%02X -> %02X", p1+j, p0[j], rx_buff[j]);
					err = false;
					break;
				}
			}
		}
		if (err == false) break;
	}
	return(err);
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  update_rom()                                                    @n
	EFFECTS  :  SPI FLASH 書き込みﾒｲﾝ処理                                       @n
	ARGUMENT :                                                                  @n
				[I] fd       制御BUS(SPIDEV1)                                   @n
				[I] src      ﾗｲﾄ元ﾃﾞｰﾀ格納ｱﾄﾞﾚｽ                                 @n
				[I] size     ﾗｲﾄｻｲｽﾞ                                            @n
				[I] dis      ﾗｲﾄ先先頭ｱﾄﾞﾚｽ                                     @n
	RETURN   :                                                                  @n
				<true> 成功                                                     @n
				<false> 失敗                                                    @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
bool update_rom(int fd, const ulong src, const ulong size, const ulong dis)
{
	bool err = true;
	int 	r;

	for (r=0; r<3; r++) {														// ３回までﾘﾄﾗｲする
		LOG(LOG_COMMON_OUT, "Lattice update");
		LOG(LOG_COMMON_OUT, "Erase start");
		err = sectorerase_spiflash(fd, dis, size);								// ﾗｲﾄ前にｾｸﾀｲﾚｰｽ実行
		if(err == false) continue;												// 異常時は抜ける(retry)

		LOG(LOG_COMMON_OUT, "Write start");
		err = write_spiflash(fd, src, size, dis);								// ﾗｲﾄ処理
		if (err == false) continue;

		LOG(LOG_COMMON_OUT, "Verify start");
		err = verify_spiflash(fd, src, size, dis);								// ﾍﾞﾘﾌｧｲ処理
		if (err == false) continue;

		LOG(LOG_COMMON_OUT, "Finish");
		if (err == true) break;													// 正常時は抜ける
	}
	return err;
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  write_enable_busy()                                             @n
	EFFECTS  :  SPI FLASH write状態監視                                         @n
	ARGUMENT :                                                                  @n
				[I] fd       制御BUS(SPIDEV1)                                   @n
	RETURN   :                                                                  @n
				<0> 失敗                                                        @n
				<1> 成功		                                                @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
bool write_enable_busy(int fd)
{
	int			ret;
	bool 		err = false;
	int 		i;
	uchar		add[4];
	uchar		cmd[4] = { 0 };
	uchar 		recv[4];
	//コマンド設定
	add[0] = LSC_PROG_SPI;
	add[1] = 0x00;
	add[2] = 0x00;
	add[3] = 0x00;
	cmd[0] = CMD_SPIFLASH_READSTATUS;

	//SPIドライバ用バッファ設定
	SpiIocTr_Reg[0].tx_buf =(unsigned long)add;
	SpiIocTr_Reg[1].tx_buf =(unsigned long)cmd;
	SpiIocTr_Reg[1].rx_buf =(unsigned long)recv;
   	SpiIocTr_Reg[0].len = 4;
   	SpiIocTr_Reg[1].len = 1+sizeof(recv);

	for(i=0; i<10;i++){										//20us*10 =200us
		// 送信
		ret = ioctl(fd, SPI_IOC_MESSAGE(2), &SpiIocTr_Reg);
		if (ret < 1) {
			ERR("can't read status registor");
			err =false;
		}
		if ((recv[1] & 0x02)){                              //bit1が1の時完了
			err =true;
			break;
		}
        usleep(20); // tW Write Status Register Cycle Time
	}
	if(err == false) {
		ERR("status busy timeout");
	}
	return err;
}


/*!-------------------------------------------------------------------          @n
	FUNCTION :  erase_busy()													@n
	EFFECTS  :  SPI FLASH セクター削除                                          @n
	ARGUMENT :                                                                  @n
				[I] fd       制御BUS(SPIDEV1)                                   @n
	RETURN   :                                                                  @n
				<true> 成功                                                     @n
				<false> 失敗                                                    @n
	NOTE     :                                                                  @n
-------------------------------------------------------------------*/
bool erase_busy(int fd)
{
	int			ret;
	bool 		err = false;
	int 		i;
	uchar		add[4];
	uchar		cmd[4] = { 0 };
	uchar 		recv[4];
	//コマンド設定
	add[0] = LSC_PROG_SPI;
	add[1] = 0x00;
	add[2] = 0x00;
	add[3] = 0x00;
	cmd[0] = CMD_SPIFLASH_FLAGREADSTATUS;

	//SPIドライバ用バッファ設定
	SpiIocTr_Reg[0].tx_buf =(unsigned long)add;
	SpiIocTr_Reg[1].tx_buf =(unsigned long)cmd;
	SpiIocTr_Reg[1].rx_buf =(unsigned long)recv;
   	SpiIocTr_Reg[0].len = 4;
   	SpiIocTr_Reg[1].len = 1+sizeof(recv);


	for(i = 0; i < 50000;i++){                                 // tBE: Block Erase Time: 1.0s
		// 送信
		ret = ioctl(fd, SPI_IOC_MESSAGE(2), &SpiIocTr_Reg);
		if (ret < 1) {
			ERR("can't read status registor");
			err = false;
		}
		if(recv[1] & 0x80) {                                    //bit7が1の時 Erase完了
		    err = true;
			break;
		}
        usleep(20);
	}
	if(err == false) {
		ERR("erase timeout");
	}
	return err;
}


int i2c_debugtest(int v){
	int i2cfd = 0;

	uint8_t buffer[2];	             	 						// I2C-Write用のバッファを準備する. */
	if (buffer == NULL) {
		printf("[DEBUG]buffer == NULL\n");
		return -1;
	}
	i2cfd = open(DevicePathI2C1.c_str(), O_RDONLY);	            // ﾃﾞﾊﾞｲｽｵｰﾌﾟﾝ
	if (i2cfd == -1) {
		printf("[DEBUG]i2c_programn_low:can't open i2c-1\n");
		return -1;
	}
	if(v == 1){
		buffer[0] = 0x03;                                 	 	 // 1バイト目にレジスタアドレスをセット. */
		buffer[1] = 0x8f;                                    	 // 2バイト目以降にデータをセット. */
	}else{
		buffer[0] = 0x03;                                 	 	 // 1バイト目にレジスタアドレスをセット. */
		buffer[1] = 0xcf;                                    	 // 2バイト目以降にデータをセット. */
	}
	struct i2c_msg message = { I2C_EXTEND_IO, 0, 3, buffer };	 // I2C-Writeメッセージを作成する. */
	struct i2c_rdwr_ioctl_data ioctl_data = { &message, 1 };

	if (ioctl(i2cfd, I2C_RDWR, &ioctl_data) != 1) {	             // I2C-Writeを行う. */
		printf("[DEBUG]i2c_programn_low:can't write");
		close(i2cfd);
		return -1;
	}
	close(i2cfd);
	return v;
}
