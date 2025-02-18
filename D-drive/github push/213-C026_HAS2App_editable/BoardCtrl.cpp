//--------------------------------------------------
//! @file common.cpp
//! @brief 共通処理ﾓｼﾞｭｰﾙ
//!
//! 本流の機能にかかわるもの、危険判定、シリアル通信
//--------------------------------------------------
#include "BoardCtrl.hpp"

#include <cstdio>
#include <string.h>
#include <math.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "Sysdef.h"
#include "Global.h"
#include "LinuxUtility.hpp"

#include <linux/rtc.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/spi/spidev.h>

#include "Global.h"

using namespace std;

/*===	SECTION NAME		===*/


/*===	user definition		===*/


/*===	pragma variable	===*/


/*===	external variable	===*/


/*===	external function	===*/


/*===	public function prototypes	===*/


/*===	static function prototypes	===*/


/*===	static typedef variable		===*/


/*===	static variable		===*/
static const string DevicePathI2C0 = "/dev/i2c-0";		//!< I2C-0のﾊﾟｽ
static const string DevicePathI2C1 = "/dev/i2c-1";		//!< I2C-1のﾊﾟｽ
static const string DevicePathI2C2 = "/dev/i2c-2";		//!< I2C-2のﾊﾟｽ
static const string DevicePathI2C3 = "/dev/i2c-3";		//!< I2C-3のﾊﾟｽ
static const string DevicePathI2C4 = "/dev/i2c-4";		//!< I2C-4のﾊﾟｽ

static const string DevicePathSPI1_0 = "/dev/spidev1.0";	// LatticeとのSPIパス


/*===	constant variable	===*/
const std::string LOG_FILE_NAME = "debug_blackout.txt";
const std::string LOG_FILE_PATH = std::string(MOUNT_SDCARD_DIR) + "/" + LOG_FILE_NAME; // ログファイルのパス
const size_t LOG_FILE_SIZE_LIMIT = 1 * 1024 * 1024; // 1MBの制限
const size_t LOG_LINES_TO_REMOVE = 100;  // サイズオーバー時に削除する行数


/*===	global variable		===*/


/*-------------------------------------------------------------------
  FUNCTION :	OutputWDTPulse()
  EFFECTS  :	WDTパルス出力
  NOTE	 :
  -------------------------------------------------------------------*/
void OutputWDTPulse(bool isOutput){
	auto level = isOutput ? LEVEL::H : LEVEL::L;
	bool isSuccess = WriteGPIO(GPIO_1_NO, level);
	g_errorManager.SetError(ErrorCode::PulseWDT, !isSuccess);
}


/*-------------------------------------------------------------------
  FUNCTION :	OutputOSWakeup
  EFFECTS  :	OS起動完了通知
  NOTE	 :
  -------------------------------------------------------------------*/
void OutputOSWakeup(){
	bool isSuccess = WriteGPIO(GPIO_2_NO, LEVEL::L);
	g_errorManager.SetError(ErrorCode::OutputOSWakeup, !isSuccess);
}


/*-------------------------------------------------------------------
  FUNCTION :	IsShutdownReq()
  EFFECTS  :	シャットダウン要求確認
  NOTE	 :
  -------------------------------------------------------------------*/
bool IsShutdownReq(){
	LEVEL level = LEVEL::L;
	bool isSuccess = ReadGPIO(&level, GPIO_3_NO);
	g_errorManager.SetError(ErrorCode::IsShutdownReq, !isSuccess);
	return level == LEVEL::L;
}


/*-------------------------------------------------------------------
  FUNCTION :	OutputAppWakeup
  EFFECTS  :	アプリ起動完了通知
  NOTE	 :
  -------------------------------------------------------------------*/
void OutputAppWakeup(bool isOutput){
	auto level = isOutput ? LEVEL::H : LEVEL::L;
	bool isSuccess = WriteGPIO(GPIO_4_NO, level);
	g_errorManager.SetError(ErrorCode::OutputAppWakeup, !isSuccess);
}


/*-------------------------------------------------------------------
  FUNCTION :	IsAccOn()
  EFFECTS  :	ACC状態の確認
  NOTE	 :
  -------------------------------------------------------------------*/
bool IsAccOn(){
	LEVEL level = LEVEL::L;
	bool isSuccess = ReadGPIO(&level, GPIO_7_NO);
	g_errorManager.SetError(ErrorCode::UnreadableAccOnGPIO, !isSuccess);
	return level == LEVEL::H;
}



static const string GPIO_L_TEXT = "0\n";				//!< GPIOがLの時の設定値
static const string GPIO_H_TEXT = "1\n";				//!< GPIOがHの時の設定値

//--------------------------------------------------
//! @brief GPIOのLEVELに応じたﾃｷｽﾄ設定値を取得
//! @param[in]		lv					ﾚﾍﾞﾙ
//! @return	ﾃｷｽﾄ
//--------------------------------------------------
static string ConvGPIOText(const LEVEL lv){
	switch(lv){
		case LEVEL::L:
			return GPIO_L_TEXT;
		case LEVEL::H:
			return GPIO_H_TEXT;
	}
	assert(false);
}

//--------------------------------------------------
//! @brief GPIOのﾃｷｽﾄに応じたﾚﾍﾞﾙを取得
//! @param[out]		level				ﾚﾍﾞﾙ
//! @param[in]		text				ﾃｷｽﾄ
//! @retval	true	取得成功
//! @retval	false	取得失敗
//--------------------------------------------------
static bool ConvGPIOText(LEVEL*const level, const string& text){
	if(text == GPIO_L_TEXT){
		*level = LEVEL::L;
		return true;
	}else if(text == GPIO_H_TEXT){
		*level = LEVEL::H;
		return true;
	}
	return false;
}


bool ReadGPIO(LEVEL*const result, int no){
	const string path = "/sys/class/gpio/gpio" + to_string(no) + "/value";
	// ﾌｧｲﾙ読み込み
	string text;
	const bool isSuccess = ReadStringFile(&text, path);
	if(isSuccess == false){
		return false;
	}

	// ﾌｧｲﾙの内容に応じてﾚﾍﾞﾙ判断
	const bool isConvSuccess = ConvGPIOText(result, text);
	if(isConvSuccess == false){
		ERR("failed read %s : Unexpected value 「%s」.", path.c_str(), text.c_str());
		return false;
	}

	return true;
}

bool WriteGPIO(int no, LEVEL lv){
	const string path = "/sys/class/gpio/gpio" + to_string(no) + "/value";

	// ﾌｧｲﾙ書き込み
	return WriteStringFile(ConvGPIOText(lv), path);
}

bool ReadI2C(std::vector<uint8_t>*const pReadData, const std::string& device, uint8_t deviceAddress, uint8_t registerAddress){
	assert(pReadData);
	// ﾃﾞﾊﾞｲｽｵｰﾌﾟﾝ
	const int32_t fd = open(device.c_str(), O_RDONLY);
	if (fd == -1) {
		ERR("%s open Failed errno=%d : %s", device.c_str(), errno, strerror(errno));
		return false;
	}

	// I2C-Readﾒｯｾｰｼﾞ作成
	struct i2c_msg messages[] = {
		{ deviceAddress, 0, 1, &registerAddress },										// ﾚｼﾞｽﾀｱﾄﾞﾚｽをｾｯﾄ
		{ deviceAddress, I2C_M_RD, static_cast<uint16_t>(pReadData->size()), pReadData->data() },	// ﾃﾞｰﾀを読み込む
	};
	struct i2c_rdwr_ioctl_data ioctl_data = { messages, 2 };

	// 読み込み
	if (ioctl(fd, I2C_RDWR, &ioctl_data) != 2) {
		ERR("%s 0x%02X 0x%02X ioctl Failed errno=%d : %s", device.c_str(), deviceAddress, registerAddress, errno, strerror(errno));
		close(fd);
		return false;
	}

	close(fd);
	return true;
}


bool WriteI2C(std::vector<uint8_t>*const pWriteData, const std::string& device, uint8_t deviceAddress, uint8_t registerAddress){
	assert(pWriteData);
	// ﾃﾞﾊﾞｲｽｵｰﾌﾟﾝ
	const int32_t fd = open(device.c_str(), O_RDONLY);
	if (fd == -1) {
		ERR("%s open Failed errno=%d : %s", device.c_str(), errno, strerror(errno));
		return false;
	}
	uint16_t length = pWriteData->size();
	uint8_t* buffer = (uint8_t*)malloc(length + 1);
	if (buffer == NULL) {
		ERR("WriteI2C memory alloc failed");
		close(fd);
		return false;
	}
	buffer[0] = registerAddress;
	memcpy(&buffer[1], pWriteData->data(), length );

	// I2C-Writeﾒｯｾｰｼﾞ作成
	struct i2c_msg messages[] = {
		{ deviceAddress, 0, static_cast<uint16_t>(length+1), buffer }									// ﾚｼﾞｽﾀｱﾄﾞﾚｽをｾｯﾄ
	};
	struct i2c_rdwr_ioctl_data ioctl_data = { messages, 1 };

	// 読み込み
	if (ioctl(fd, I2C_RDWR, &ioctl_data) != 1) {
		ERR("%s 0x%02X 0x%02X ioctl Failed errno=%d : %s", device.c_str(), deviceAddress, registerAddress, errno, strerror(errno));
		free(buffer);
		close(fd);
		return false;
	}

	free(buffer);
	close(fd);
	return true;
}

bool ReadSPI(std::vector<uint8_t>*const pReadData, const std::string& device, uint8_t registerAddress, uint8_t spiMode, uint8_t spiBits, uint32_t spiSpeed){
	assert(pReadData);
	uint8_t mode;
	uint8_t bits;
	uint32_t speed;

	struct spi_ioc_transfer ioctl_data[2];
	int len = static_cast<int>(pReadData->size());
	int i;
	uint8_t tx_buff[2];
	uint8_t rx_buff[1];

	// ﾃﾞﾊﾞｲｽｵｰﾌﾟﾝ
	const int32_t fd = open(device.c_str(), O_RDONLY);
	if (fd == -1) {
		ERR("%s open Failed errno=%d : %s", device.c_str(), errno, strerror(errno));
		return false;
	}
	// 通信設定
	mode = spiMode;
	bits = spiBits;
	speed = spiSpeed;

	// ﾓｰﾄﾞ設定
	if( ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0 ){                             // spi mode(write)
		ERR("%s 0x%02X ioctl SPI_IOC_WR_MODE Failed errno=%d : %s", device.c_str(), registerAddress, errno, strerror(errno));
		close(fd);
		return false;
	}

	// bit設定
	if( ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ){                             // spi mode(write)
		ERR("%s 0x%02X ioctl SPI_IOC_WR_BITS_PER_WORD Failed errno=%d : %s", device.c_str(), registerAddress, errno, strerror(errno));
		close(fd);
		return false;
	}

	// Speed設定
	if( ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0 ){                             // spi mode(write)
		ERR("%s 0x%02X ioctl SPI_IOC_WR_BITS_PER_WORD Failed errno=%d : %s", device.c_str(), registerAddress, errno, strerror(errno));
		close(fd);
		return false;
	}


	// 通信データ生成
	for( i = 0; i < len; i++ ){
	tx_buff[0] = 0x03;
		tx_buff[1] = registerAddress + i;
	memset(ioctl_data,0,sizeof(ioctl_data));
	ioctl_data[0].tx_buf = (unsigned long)tx_buff;
	ioctl_data[0].len = (int)sizeof(tx_buff);
		ioctl_data[1].rx_buf = (unsigned long)rx_buff;
		ioctl_data[1].len = (int)sizeof(rx_buff);
	// 読み込み
	if (ioctl(fd, SPI_IOC_MESSAGE(2), &ioctl_data) < 0) {
		ERR("%s 0x%02X ioctl SPI_IOC_MESSAGE Failed errno=%d : %s", device.c_str(), registerAddress, errno, strerror(errno));
		close(fd);
		return false;
	}
		*( pReadData->data() + i ) = rx_buff[0];
	}

	close(fd);
	return true;
}

bool WriteSPI(std::vector<uint8_t>*const pWriteData, const std::string& device, uint8_t registerAddress, uint8_t spiMode, uint8_t spiBits, uint32_t spiSpeed){
#if 0
	assert(pWriteData);
	uint8_t mode;
	uint8_t bits;
	uint32_t speed;

	struct spi_ioc_transfer ioctl_data[1] = {0};
	uint8_t tx_buff[3];

	// ﾃﾞﾊﾞｲｽｵｰﾌﾟﾝ
	const int32_t fd = open(device.c_str(), O_RDONLY);
	if (fd == -1) {
		ERR("%s open Failed errno=%d : %s", device.c_str(), errno, strerror(errno));
		return false;
	}
	// 通信設定
	mode = spiMode;
	bits = spiBits;
	speed = spiSpeed;

	// ﾓｰﾄﾞ設定
	if( ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0 ){                             // spi mode(write)
		ERR("%s 0x%02X ioctl SPI_IOC_WR_MODE Failed errno=%d : %s", device.c_str(), registerAddress, errno, strerror(errno));
		close(fd);
		return false;
	}

	// bit設定
	if( ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ){                             // spi mode(write)
		ERR("%s 0x%02X ioctl SPI_IOC_WR_BITS_PER_WORD Failed errno=%d : %s", device.c_str(), registerAddress, errno, strerror(errno));
		close(fd);
		return false;
	}

	// Speed設定
	if( ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0 ){                             // spi mode(write)
		ERR("%s 0x%02X ioctl SPI_IOC_WR_BITS_PER_WORD Failed errno=%d : %s", device.c_str(), registerAddress, errno, strerror(errno));
		close(fd);
		return false;
	}

	uint16_t length = pWriteData->size();
	uint8_t* buffer = (uint8_t*)malloc(length + 1);
	if (buffer == NULL) {
		ERR("WriteSPI memory alloc failed");
		close(fd);
		return false;
	}
	buffer[0] = registerAddress;
	memcpy(&buffer[1], pWriteData->data(), length );

	// 通信データ生成
	tx_buff[0] = 0x06;
	tx_buff[1] = registerAddress;
	tx_buff[2] = buffer[0];

	memset(ioctl_data,0,sizeof(ioctl_data));
	ioctl_data[0].tx_buf = (unsigned long)tx_buff;
	ioctl_data[0].len = (int)sizeof(tx_buff);

	// 読み込み
	if (ioctl(fd, SPI_IOC_MESSAGE(1), &ioctl_data) < 0) {
		ERR("%s 0x%02X ioctl SPI_IOC_MESSAGE Failed errno=%d : %s", device.c_str(), registerAddress, errno, strerror(errno));
		free(buffer);
		close(fd);
		return false;
	}

	free(buffer);
	close(fd);
	return true;
#else
	assert(pWriteData);
	uint8_t mode;
	uint8_t bits;
	uint32_t speed;
	mode = spiMode;
	bits = spiBits;
	speed = spiSpeed;

	struct spi_ioc_transfer ioctl_data[3] = {0};
	uint8_t tx_buff[3];
	uint8_t rx_buff[3];

	// ﾃﾞﾊﾞｲｽｵｰﾌﾟﾝ
	const int32_t fd = open(device.c_str(), O_RDONLY);
	if (fd == -1) {
		ERR("%s open Failed errno=%d : %s", device.c_str(), errno, strerror(errno));
		return false;
	}

	// ﾓｰﾄﾞ設定
	if( ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0 ){                             // spi mode(write)
		ERR("%s 0x%02X ioctl SPI_IOC_WR_MODE Failed errno=%d : %s", device.c_str(), registerAddress, errno, strerror(errno));
		close(fd);
		return false;
	}

	// 通信データ生成
	tx_buff[0] = 0x06;
	tx_buff[1] = registerAddress;
	tx_buff[2] = (*pWriteData)[0];

	for (int i=0; i<3; i++) {
		ioctl_data[i].tx_buf = (unsigned long)(tx_buff+i);
		ioctl_data[i].rx_buf = (unsigned long)(rx_buff+i);
		ioctl_data[i].len = 1;
		ioctl_data[i].speed_hz = speed;
		ioctl_data[i].delay_usecs = 0;
		ioctl_data[i].bits_per_word = bits;
	}


	// 読み込み
	if (ioctl(fd, SPI_IOC_MESSAGE(3), &ioctl_data) < 1) {
		ERR("%s 0x%02X ioctl SPI_IOC_MESSAGE Failed errno=%d : %s", device.c_str(), registerAddress, errno, strerror(errno));
		close(fd);
		return false;
	}
	close(fd);
	return true;

#endif
}

std::tuple<bool, bool> GetRTCStatus(void){
	// 読み込み
	vector<uint8_t> readData(1);
	const bool isSuccess = ReadI2C(&readData, DevicePathI2C0, 0x32, 0x1D);
	g_errorManager.SetError(ErrorCode::GetRTCStatus, !isSuccess);
	if(isSuccess == false){
		return {false, false};
	}

	// 判定
	const bool isLowBattery			= (readData[0] & 0x80) == 1;
	const bool isOscillationStop	= (readData[0] & 0x02) == 1;

	return {isLowBattery, isOscillationStop};
}

bool StartUpFPGA(void){
	vector<uint8_t> writedata(1);
	int i;

	writedata[0] = 0x00;
	const bool isSuccess_plllock0 = WriteSPI(&writedata, DevicePathSPI1_0, 0x30, SPI_MODE_0,8,1000000);
	if(isSuccess_plllock0 == false) {
		g_errorManager.SetError(ErrorCode::StartUpFPGA, !isSuccess_plllock0);
		return false;
	}
	usleep(1000);

	writedata[0] = 0x01;
	const bool isSuccess_plllock1 = WriteSPI(&writedata, DevicePathSPI1_0, 0x30, SPI_MODE_0, 8, 1000000);
	if (isSuccess_plllock1 == false) {
		g_errorManager.SetError(ErrorCode::StartUpFPGA, !isSuccess_plllock1);
		return false;
	}
	usleep(1000);

	writedata[0] = 0x00;
	const bool isSuccess_plllock2 = WriteSPI(&writedata, DevicePathSPI1_0, 0x30, SPI_MODE_0, 8, 1000000);
	if (isSuccess_plllock2 == false) {
		g_errorManager.SetError(ErrorCode::StartUpFPGA, !isSuccess_plllock2);
		return false;
	}

	// wait PLL Lock

	//	cv::TickMeter meter0;
	//meter0.start();
#if 1
	for (i=0; i<1000; i++) {
		static vector<uint8_t> readData(1);
		ReadSPI(&readData, DevicePathSPI1_0, 0x30, SPI_MODE_0,8,1000000);
		if ((readData[0] & 0x10) == 0x10) {
			break;
		}
		usleep(1000);
	}
	if (i==1000) {
		g_errorManager.SetError(ErrorCode::StartUpFPGA, true);
		return false;
	}
#else
	usleep(100000);
#endif
	//meter0.stop();
	//LOG(LOG_COMMON_OUT, "@@@@ Busy time : %f [ms]", meter0.getAvgTimeMilli());

	writedata[0] = 0x11;
	const bool isSuccess_720p_enanle = WriteSPI(&writedata, DevicePathSPI1_0, 0x31, SPI_MODE_0,8,1000000);

	if(isSuccess_720p_enanle == false) {
		g_errorManager.SetError(ErrorCode::StartUpFPGA, !isSuccess_720p_enanle);
		return false;
	}

	return true;
}

bool GetFPGAVersion(std::string*const pReadData, std::string*const pLogData){
	static vector<uint8_t> readData(8);
	char version_str[100];

	const bool isSuccess = ReadSPI(&readData, DevicePathSPI1_0, 0xF8, SPI_MODE_0,8,1000000);
	g_errorManager.SetError(ErrorCode::GetFPGAVersion, !isSuccess);
	if(isSuccess == false){
		strcpy(version_str, "XXXX/XX/XX XX:XX XXXX");
		*pReadData = string(version_str);
		strcpy(version_str, "XX.XX XXXX/XX/XX XX:XX");
		*pLogData = string(version_str);
		return false;
	}
	sprintf(version_str,"%02X%02X/%02X/%02X %02X:%02X %02X%02X",
			readData[0],readData[1],readData[2],readData[3],			// 年月日
			readData[4],readData[5],									// 時分
			readData[6],readData[7]);									// ﾊﾞｰｼﾞｮﾝ
	*pReadData = string(version_str);
	sprintf(version_str,"%02X.%02X  %02X%02X/%02X/%02X %02X:%02X",
			readData[6],readData[7],									// ﾊﾞｰｼﾞｮﾝ
			readData[0],readData[1],readData[2],readData[3],			// 年月日
			readData[4],readData[5]										// 時分
			);
	*pLogData = string(version_str);

	return true;

}

bool I2cSelect_Ext(int select){
	static vector<uint8_t> readData(1);
	static vector<uint8_t> writeData(1);
	bool isSuccess;

	// 出力設定
	isSuccess = ReadI2C(&readData, DevicePathI2C1, 0x20, 0x07);
	g_errorManager.SetError(ErrorCode::SetVideoSelect, !isSuccess);
	if(isSuccess == false){
		return false;
	}
	writeData[0] = readData[0];
	writeData[0] &= 0b00111111;
	isSuccess = WriteI2C(&writeData, DevicePathI2C1, 0x20, 0x07);
	g_errorManager.SetError(ErrorCode::SetVideoSelect, !isSuccess);
	if(isSuccess == false){
		return false;
	}

	// Select設定
	isSuccess = ReadI2C(&readData, DevicePathI2C1, 0x20, 0x03);
	g_errorManager.SetError(ErrorCode::SetVideoSelect, !isSuccess);
	if(isSuccess == false){
		return false;
	}
	writeData[0] = readData[0];
	if(select){
		writeData[0] |= 0b01000000;
	}
	else{
		writeData[0] &= 0b10111111;
	}
	isSuccess = WriteI2C(&writeData, DevicePathI2C1, 0x20, 0x03);
	g_errorManager.SetError(ErrorCode::SetVideoSelect, !isSuccess);
	if(isSuccess == false){
		return false;
	}

	// Can設定
	isSuccess = ReadI2C(&readData, DevicePathI2C1, 0x20, 0x03);
	g_errorManager.SetError(ErrorCode::Canenable, !isSuccess);
	if(isSuccess == false){
		return false;
	}
	writeData[0] = readData[0];
	writeData[0] |= 0b10000000;
	isSuccess = WriteI2C(&writeData, DevicePathI2C1, 0x20, 0x03);
	g_errorManager.SetError(ErrorCode::Canenable, !isSuccess);
	if(isSuccess == false){
		return false;
	}
	return true;
}


/*-------------------------------------------------------------------
	FUNCTION :	capture_state()
	EFFECTS  :
	NOTE	 :
-------------------------------------------------------------------*/
bool capture_state(void)
{
	static vector<uint8_t> writeData(1);
	static vector<uint8_t> readData(1);
	static vector<uint8_t> readData_mpage(1);
	bool isSuccess_mpage,isSuccess;
	isSuccess_mpage = ReadI2C(&readData_mpage, DevicePathI2C3, 0x45, 0x40);
	if(isSuccess_mpage == false){
		return false;
	}
	if(readData_mpage[0] == 0x08){
		writeData[0] = 0x00;
		isSuccess_mpage = WriteI2C(&writeData, DevicePathI2C3, 0x45, 0x40);
		if(isSuccess_mpage == false){
			return false;
		}
	}

	isSuccess = ReadI2C(&readData, DevicePathI2C3, 0x45, 0x01);
	if(isSuccess == false){
		return false;
	}
#ifdef SD_LOG_ENABLE
	g_video_status.tp2850 = readData[0];
#endif
	if(readData[0] & 0x80) return false;
	else return true;
}




// for debug_log
#ifdef SD_LOG_ENABLE
void GetFPGAStatus(void){
	static vector<uint8_t> readData0(1);
	const bool isSuccess0 = ReadSPI(&readData0, DevicePathSPI1_0, 0x30, SPI_MODE_0,8,1000000);
	if(isSuccess0 == true){
		g_video_status.fpga_0 = readData0[0];
	}
	else {
		g_video_status.fpga_0 = 0xFF;
	}
	static vector<uint8_t> readData1(1);
	const bool isSuccess1 = ReadSPI(&readData1, DevicePathSPI1_0, 0x40, SPI_MODE_0,8,1000000);
	if(isSuccess1 == true){
		g_video_status.fpga_1 = readData1[0];
	}
	else {
		g_video_status.fpga_1 = 0xFF;
	}
}

/*-------------------------------------------------------------------
  FUNCTION :	get_tp2912_status()
  EFFECTS  :
  NOTE	 :
  -------------------------------------------------------------------*/
void get_tp2912_status(void)
{
	static vector<uint8_t> writeData(1);
	static vector<uint8_t> readData(1);
	static vector<uint8_t> readData_mpage(1);
	bool isSuccess;

	isSuccess = ReadI2C(&readData, DevicePathI2C2, 0x44, 0xF6);
	if(isSuccess == false){
		g_video_status.tp2912_0 = 0xFF;
	}
	else {
		g_video_status.tp2912_0 = readData[0];
	}

	isSuccess = ReadI2C(&readData, DevicePathI2C2, 0x45, 0xF6);
	if(isSuccess == false){
		g_video_status.tp2912_1 = 0xFF;
	}
	else {
		g_video_status.tp2912_1 = readData[0];
	}

}

/*-------------------------------------------------------------------
  FUNCTION :	get_exio_status()
  EFFECTS  :
  NOTE	 :
  -------------------------------------------------------------------*/
void get_exio_status(void)
{
	static vector<uint8_t> writeData(1);
	static vector<uint8_t> readData(1);
	static vector<uint8_t> readData_mpage(1);
	bool isSuccess;

	isSuccess = ReadI2C(&readData, DevicePathI2C1, 0x20, 0x03);
	if(isSuccess == false){
		g_video_status.exio= 0xFF;
	}
	else {
		g_video_status.exio = readData[0];
	}
}

// ファイルサイズをチェックする関数
size_t getFileSize(const std::string& filename) {
    struct stat stat_buf;
    if (stat(filename.c_str(), &stat_buf) == 0) {
        return stat_buf.st_size;
    }
    return 0;
}

// ログファイルの先頭から一定行数削除する関数
void removeOldLogLines(const std::string& filename, size_t linesToRemove) {
    std::ifstream inFile(filename);
    std::deque<std::string> logLines;
    std::string line;

    // 全ての行を読み込んでメモリ上に保存
    while (std::getline(inFile, line)) {
        logLines.push_back(line);
    }
    inFile.close();

    // 指定された行数だけ古い行を削除
    for (size_t i = 0; i < linesToRemove && !logLines.empty(); ++i) {
        logLines.pop_front();
    }

    // 残ったログを書き戻す
    std::ofstream outFile(filename, std::ofstream::trunc);
    for (const auto& logLine : logLines) {
        outFile << logLine << "\n";
    }
    outFile.close();
}

// 現在の時間を "yyyy/mm/dd hh:mm:ss" 形式で取得する関数
std::string getCurrentTime() {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(now, "%Y/%m/%d %H:%M:%S");
    return oss.str();
}

// ログ書き込み関数
void logStatus(void) {
	static VideoStatuType master_status = {0x7E, 0x10, 0x10, 0x01, 0x01, 0xCF};
	static int invalidstatus_counter = 0;			// 異常状態カウンタ

	if ((g_video_status.tp2850 == 0x00) && (g_video_status.exio == 0x00) &&
		(g_video_status.fpga_0 == 0x00) && (g_video_status.fpga_1 == 0x00) &&
		(g_video_status.tp2912_0 == 0x00) && (g_video_status.tp2912_1 == 0x00)) {
		invalidstatus_counter = 0;
		return;
	}
	else if ((g_video_status.tp2850 == master_status.tp2850) &&
			 (g_video_status.exio == master_status.exio) &&
			 (g_video_status.fpga_0 == master_status.fpga_0) &&
			 (g_video_status.fpga_1 == master_status.fpga_1) &&
			 (g_video_status.tp2912_0 == master_status.tp2912_0) &&
			 (g_video_status.tp2912_1 == master_status.tp2912_1)) {
		invalidstatus_counter = 0;
		return;
	}
	else if (invalidstatus_counter < 10) {
		invalidstatus_counter++;
		return;
	}
	else {
		master_status.tp2850 = g_video_status.tp2850;
		master_status.exio = g_video_status.exio;
		master_status.fpga_0 = g_video_status.fpga_0;
		master_status.fpga_1 = g_video_status.fpga_1;
		master_status.tp2912_0 =  g_video_status.tp2912_0;
		master_status.tp2912_1 =  g_video_status.tp2912_1;
		invalidstatus_counter = 0;
	}
    // ファイルサイズが制限を超えている場合、古いログを削除
    if (getFileSize(LOG_FILE_PATH) >= LOG_FILE_SIZE_LIMIT) {
        removeOldLogLines(LOG_FILE_PATH, LOG_LINES_TO_REMOVE);
    }

    // 新しいログを書き込む
    std::ofstream logFile(LOG_FILE_PATH, std::ofstream::app);
    if (logFile.is_open()) {
        logFile << getCurrentTime() << " ";  // 時間情報を追加
        logFile << std::hex << std::uppercase;
        logFile << "ExIO:" << std::setfill('0') << std::setw(2) << (int)master_status.exio << ","
                << "CAM0:" << std::setfill('0') << std::setw(2) << (int)master_status.tp2850 << ","
                << "FPGA0:" << std::setfill('0') << std::setw(2) << (int)master_status.fpga_0 << ","
                << "FPGA1:" << std::setfill('0') << std::setw(2) << (int)master_status.fpga_1 << ","
                << "MONI0:" << std::setfill('0') << std::setw(2) << (int)master_status.tp2912_0 << ","
                << "MONI1:" << std::setfill('0') << std::setw(2) << (int)master_status.tp2912_1 << "\n";
        logFile.close();
    } else {
        std::cerr << "Failed to open log file." << std::endl;
    }
}
#endif
