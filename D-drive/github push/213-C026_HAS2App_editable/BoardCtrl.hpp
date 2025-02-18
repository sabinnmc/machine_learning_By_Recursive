//--------------------------------------------------
//! @file common.hpp
//! @brief 共通処理ﾓｼﾞｭｰﾙ
//!
//! 本流の機能にかかわるもの、危険判定、シリアル通信
//--------------------------------------------------

#pragma once
#include <string>
#include <vector>
#include <stdint.h>

//--------------------------------------------------
//! @brief ﾀｲﾄﾙ表示
//--------------------------------------------------
//--------------------------------------------------
//! @brief WDT専用線出力
//!
//! ｴﾗｰ発生時は ErrorManager に ErrorCode::PulseWDT を登録
//! @param[in]		isOutput			出力するか？ true:出力 false:出力しない
//! @retval	true	設定成功
//! @retval	false	設定失敗
//--------------------------------------------------
void OutputWDTPulse(bool isOutput);

//--------------------------------------------------
//! @brief OS起動完了信号を出力
//!
//! ｴﾗｰ発生時は ErrorManager に ErrorCode::OutputOSWakeup を登録
//! @retval	true	設定成功
//! @retval	false	設定失敗
//! @note			ｴﾗｰ発生時はErrorManagerにて処理
//--------------------------------------------------
void OutputOSWakeup();

//--------------------------------------------------
//! @brief ｼｬｯﾄﾀﾞｳﾝ信号を受信したか？
//!
//! ｴﾗｰ発生時は ErrorManager に ErrorCode::IsShutdownReq を登録
//! @retval	true	受信中
//! @retval	false	受信していない
//! @attention		読み込みｴﾗｰ発生時は戻り値が不定なのでErrorManagerにてｴﾗｰが発生していないことを要確認
//--------------------------------------------------
bool IsShutdownReq();

//--------------------------------------------------
//! @brief ｱﾌﾟﾘ起動完了信号を出力
//!
//! ｴﾗｰ発生時は ErrorManager に ErrorCode::OutputAppWakeup を登録
//! @param[in]		isOutput			出力するか？ true:出力 false:出力しない
//! @retval	true	設定成功
//! @retval	false	設定失敗
//! @note			ｴﾗｰ発生時はErrorManagerにて処理
//--------------------------------------------------
void OutputAppWakeup(bool isOutput);

//--------------------------------------------------
//! @brief ACC ON確認
//!
//! ｴﾗｰ発生時は ErrorManager に ErrorCode::UnreadableAccOnGPIO を登録
//! @retval	true	ACC ON
//! @retval	false	ACC OFF
//! @attention		読み込みｴﾗｰ発生時は戻り値が不定なのでErrorManagerにてｴﾗｰが発生していないことを要確認
//--------------------------------------------------
bool IsAccOn();

//--------------------------------------------------
//! @brief GPIOの数値表現を計算
//!
//! 「GPIOxx_IOyy」の形式からLinuxでｱｸｾｽする際に使用する数値へ変換する
//! https://developer.toradex.com/knowledge-base/gpio-alphanumeric-to-gpio-numeric-assignment#iMX_6_iMX_6ULL_iMX_7_iMX_8M_Mini_iMX_8M_Plus
//! @param[in]		controller			GPIOｺﾝﾄﾛｰﾗｰ番号（GPIOxx_IOyyの、xx）
//! @param[in]		gpio				IO番号（GPIOxx_IOyyの、yy）
//! @return	Linuxでｱｸｾｽする際に使用する数値
//--------------------------------------------------
constexpr int CalcGPIO(const int controller, const int gpio){
	return 32 * (controller - 1) + gpio;
}

constexpr int GPIO_1_NO = CalcGPIO(1, 0);			//!< GPIO1の数値表現（Linuxでのｱｸｾｽ時に使用）
constexpr int GPIO_2_NO = CalcGPIO(1, 1);			//!< GPIO2の数値表現（Linuxでのｱｸｾｽ時に使用）
constexpr int GPIO_3_NO = CalcGPIO(1, 5);			//!< GPIO3の数値表現（Linuxでのｱｸｾｽ時に使用）
constexpr int GPIO_4_NO = CalcGPIO(1, 6);			//!< GPIO4の数値表現（Linuxでのｱｸｾｽ時に使用）
constexpr int GPIO_7_NO = CalcGPIO(4, 3);			//!< GPIO7_CSIの数値表現（Linuxでのｱｸｾｽ時に使用）
constexpr int GPIO_8_NO = CalcGPIO(4, 1);			//!< GPIO8_CSIの数値表現（Linuxでのｱｸｾｽ時に使用）


//--------------------------------------------------
//! @brief I2C読み込み
//! @param[out]		pReadData			読み込んだ内容を格納する場所。ｴﾗｰ発生時は不定の値
//! @param[in]		device				ﾃﾞﾊﾞｲｽの名前
//! @param[in]		deviceAddress		ﾃﾞﾊﾞｲｽｱﾄﾞﾚｽ
//! @param[in]		registerAddress		ﾚｼﾞｽﾀｱﾄﾞﾚｽ
//! @retval	true	取得成功
//! @retval	false	取得失敗
//--------------------------------------------------
bool ReadI2C(std::vector<uint8_t>*const pReadData, const std::string& device, uint8_t deviceAddress, uint8_t registerAddress);

//--------------------------------------------------
//! @brief I2C書き込み
//! @param[out]		pWriteData			書き込みデータ
//! @param[in]		device				ﾃﾞﾊﾞｲｽの名前
//! @param[in]		deviceAddress		ﾃﾞﾊﾞｲｽｱﾄﾞﾚｽ
//! @param[in]		registerAddress		ﾚｼﾞｽﾀｱﾄﾞﾚｽ
//! @retval	true	取得成功
//! @retval	false	取得失敗
//--------------------------------------------------
bool WriteI2C(std::vector<uint8_t>*const pWriteData, const std::string& device, uint8_t deviceAddress, uint8_t registerAddress);

//--------------------------------------------------
//! @brief SPI読み込み
//! @param[out]		pReadData			読み込んだ内容を格納する場所。ｴﾗｰ発生時は不定の値
//! @param[in]		device				ﾃﾞﾊﾞｲｽの名前
//! @param[in]		registerAddress		ﾚｼﾞｽﾀｱﾄﾞﾚｽ
//! @param[in]		spiMode				SPIﾓｰﾄﾞ
//! @param[in]		spiBits				ﾋﾞｯﾄ設定
//! @param[in]		spiSpeed			通信速度
//! @retval	true	取得成功
//! @retval	false	取得失敗
//--------------------------------------------------
bool ReadSPI(std::vector<uint8_t>*const pReadData, const std::string& device, uint8_t registerAddress, uint8_t spiMode, uint8_t spiBits, uint32_t spiSpeed);

//--------------------------------------------------
//! @brief SPI書き込み
//! @param[out]		pWriteData			書き込みデータ
//! @param[in]		device				ﾃﾞﾊﾞｲｽの名前
//! @param[in]		registerAddress		ﾚｼﾞｽﾀｱﾄﾞﾚｽ
//! @param[in]		spiMode				SPIﾓｰﾄﾞ
//! @param[in]		spiBits				ﾋﾞｯﾄ設定
//! @param[in]		spiSpeed			通信速度
//! @retval	true	取得成功
//! @retval	false	取得失敗
//--------------------------------------------------
bool WriteSPI(std::vector<uint8_t>*const pWriteData, const std::string& device, uint8_t registerAddress, uint8_t spiMode, uint8_t spiBits, uint32_t spiSpeed);

//--------------------------------------------------
//! @brief H,Lの定数
//--------------------------------------------------
enum class LEVEL{
	L = 0,
	H = 1,
};

//--------------------------------------------------
//! @brief GPIOの値を取得
//! @param[out]		result				GPIOの値を格納する場所。ｴﾗｰ発生時は不定の値
//! @param[in]		no					GPIO番号
//! @retval	true	取得成功
//! @retval	false	取得失敗
//--------------------------------------------------
bool ReadGPIO(LEVEL*const result, int no);

//--------------------------------------------------
//! @brief GPIOに値を設定
//! @param[in]		no					GPIO番号
//! @param[in]		lv					設定する値
//! @retval	true	設定成功
//! @retval	false	設定失敗
//--------------------------------------------------
bool WriteGPIO(int no, LEVEL lv);

//--------------------------------------------------
//! @brief RTCのｽﾃｰﾀｽを取得
//!
//! ｴﾗｰ発生時は ErrorManager に ErrorCode::GetRTCStatus を登録
//! @return	{LBATの電圧低下を検出したか？, 発振停止を検出したか？}
//--------------------------------------------------
std::tuple<bool, bool> GetRTCStatus(void);

//--------------------------------------------------
//! @brief FPGAﾘｾｯﾄ解除
//! @retval	true	成功
//! @retval	false	失敗
//--------------------------------------------------
bool StartUpFPGA(void);

//--------------------------------------------------
//! @brief FPGAﾊﾞｰｼﾞｮﾝ取得
//! @param[out]		pReadData		FPGAﾊﾞｰｼﾞｮﾝ
//! @retval	true	成功
//! @retval	false	失敗
//--------------------------------------------------
bool GetFPGAVersion(std::string*const pReadData, std::string*const pLogData);

//--------------------------------------------------
//! @brief 拡張IO設定
//! @param[in]		select		ｾﾚｸﾀ
//! @retval	true	成功
//! @retval	false	失敗
//--------------------------------------------------
bool I2cSelect_Ext(int select);

//--------------------------------------------------
//! @brief Capture成否判定
//!
//! @retval	true		可
//! @retval	false		否
//--------------------------------------------------
bool capture_state(void);

#ifdef SD_LOG_ENABLE
//--------------------------------------------------
//! @brief デバッグ用のログ追加
//!
//! @retval	なし
//--------------------------------------------------
void logStatus(void);
void GetFPGAStatus(void);
void get_tp2912_status(void);
void get_exio_status(void);
#endif
