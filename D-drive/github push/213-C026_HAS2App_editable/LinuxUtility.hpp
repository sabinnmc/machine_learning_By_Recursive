//--------------------------------------------------
//! @file utility.hpp
//! @brief ﾕｰﾃｨﾘﾃｨﾓｼﾞｭｰﾙ
//!
//! 本流の機能に関わらない付属的な機能
//--------------------------------------------------

#pragma once
#include <tuple>
#include <vector>
#include "Sysdef.h"
#include <chrono>
#include <iostream>
#include <stdint.h>


//--------------------------------------------------
//! @brief 安全なﾒﾓﾘ解放処理
//!
//! deleteした後nullptrを設定する
//! @param[in,out]	p					解放するﾎﾟｲﾝﾀ
//--------------------------------------------------
template<class T>
void SafeDelete( T*& p ){
	delete p;
	p = nullptr;
}

//--------------------------------------------------
//! @brief ﾊﾞｲﾅﾘﾌｧｲﾙ読み込み
//! @param[out]		pReadData			読み込んだ内容を格納する場所。ｴﾗｰ発生時は不定の値
//! @param[in]		path				読み込むﾌｧｲﾙのﾊﾟｽ
//! @retval	true	取得成功
//! @retval	false	取得失敗
//--------------------------------------------------
bool ReadBinaryFile(std::vector<char>*const pReadData, const std::string& path);

//--------------------------------------------------
//! @brief ﾃｷｽﾄﾌｧｲﾙ読み込み
//! @param[out]		pReadData			読み込んだ内容を格納する場所。ｴﾗｰ発生時は不定の値
//! @param[in]		path				読み込むﾌｧｲﾙのﾊﾟｽ
//! @retval	true	取得成功
//! @retval	false	取得失敗
//--------------------------------------------------
bool ReadStringFile(std::string*const pReadData, const std::string& path);

//--------------------------------------------------
//! @brief ﾃｷｽﾄﾌｧｲﾙ書き込み
//! @param[in]		writeData			書き込む内容
//! @param[in]		path				書き込むﾌｧｲﾙのﾊﾟｽ
//! @retval	true	書き込み成功
//! @retval	false	書き込み失敗
//--------------------------------------------------
bool WriteStringFile(const std::string& writeData, const std::string& path);

//--------------------------------------------------
//! @brief HWMON用の初期化
//!
//! HWMONのﾌｧｲﾙは起動ﾀｲﾐﾝｸﾞによりﾊﾟｽが変化するため、ﾌｧｲﾙの存在を確認しﾊﾟｽを設定する
//! ｴﾗｰ発生時は ErrorManager に ErrorCode::NotFoundTempDevice か ErrorCode::NotFoundPowerDevice を登録
//! @retval	true	初期化成功
//! @retval	false	初期化成功
//--------------------------------------------------
bool InitHWMON(void);

//--------------------------------------------------
//! @brief SOMの消費電力（μW）を取得
//!
//! ｴﾗｰ発生時は ErrorManager に ErrorCode::GetMicrowattsOfSOM を登録
//! @return	消費電力（μW）
//! @note	参考ｻｲﾄ：<https://developer.toradex.com/knowledge-base/power-consumption-of-the-verdin-imx8m-plus-module#Power_Measurement_of_Verdin_SoMs>
//--------------------------------------------------
int GetMicrowattsOfSOM(void);

//--------------------------------------------------
//! @brief CPU内部の温度（℃）を取得
//!
//! ｴﾗｰ発生時は ErrorManager に ErrorCode::GetInternalTempZone0 か ErrorCode::GetInternalTempZone1 を登録
//! @return	{ｿﾞｰﾝ0（A53CPU）の温度（℃）, ｿﾞｰﾝ1（ANAMIX付近のSoC）の温度（℃）}
//! @note	参考ｻｲﾄ：<https://developer.toradex.com/knowledge-base/temperature-sensor-linux#NXPFreescale_iMX_8M_Mini8M_Plus_Based_Modules>
//--------------------------------------------------
std::tuple<float, float> GetInternalTemp(void);

//--------------------------------------------------
//! @brief 温度ｾﾝｻｰの温度（℃）を取得
//!
//! ｴﾗｰ発生時は ErrorManager に ErrorCode::GetExternalTemp を登録
//! @return	温度（℃）
//--------------------------------------------------
float GetExternalTemp(void);

//--------------------------------------------------
//! @brief 温度ｾﾝｻｰの取得パスを削除
//!
//! 温度取得失敗時のOS異常発生の検証用
//--------------------------------------------------
void DeleteExternalTempPath(void);

//--------------------------------------------------
//! @brief 月の英語名を取得
//!
//! monthが範囲外ならassert
//! @param[in]		month			月（1～12）
//! @return			英語名
//--------------------------------------------------
std::string GetMonthEnglishText(int month);

//--------------------------------------------------
//! @brief ｽｸﾘｰﾝｻｲｽﾞ(横幅)を取得
//! @return	ｽｸﾘｰﾝｻｲｽﾞ(横幅)
//--------------------------------------------------
uint32_t get_screen_width();

//--------------------------------------------------
//! @brief ｽｸﾘｰﾝｻｲｽﾞ(縦幅)を取得
//! @return	ｽｸﾘｰﾝｻｲｽﾞ(縦幅)
//--------------------------------------------------
uint32_t get_screen_height();



//--------------------------------------------------
//! @brief RTCの初期化が完了しているか？
//! @retval	true	初期化完了
//! @retval	false	未完了
//--------------------------------------------------
bool IsCompleteInitRTC(void);

//--------------------------------------------------
//! @brief ファイルをsync
//--------------------------------------------------
void sync_exec(void);

//--------------------------------------------------
//! @brief ﾃﾞｨﾚｸﾄﾘをsync
//!
//! ﾃﾞｨﾚｸﾄﾘをsyncしないと確実な保存はされないらしい
//! https://linuxjm.osdn.jp/html/LDP_man-pages/man2/fsync.2.html
//! `fsync() の呼び出しは、ファイルが存在しているディレクトリのエントリーが
//! ディスクへ 書き込まれたことを保証するわけではない。
//! 保証するためには明示的にそのディレクトリのファイルディスクリプターに
//! 対してもfsync() する必要がある。`
//! @retval	0		成功
//! @retval	-1		失敗
//--------------------------------------------------
int sync_dir(const std::string& dir);

//--------------------------------------------------
//! @brief ﾊﾟｽからﾌｧｲﾙ名（拡張子なし）のみを抽出
//! @param[in]		path				抽出元のﾊﾟｽ
//! @return	ﾌｧｲﾙ名（拡張子なし）
//--------------------------------------------------
std::string GetFileName(const std::string& path);

//--------------------------------------------------
//! @brief ﾊﾟｽから拡張子を抽出
//! @param[in]		path				抽出元のﾊﾟｽ
//! @return	拡張子
//--------------------------------------------------
std::string GetExtensionName(const std::string& path);

//--------------------------------------------------
//! @brief ﾃﾞｨﾚｸﾄﾘまでのﾊﾟｽを抽出
//!
//! 末尾の「/」と「\」を確認し、そこまでを抽出する
//! 見つからない場合はｶﾗの文字列を返す
//! @param[in]		path				抽出元のﾊﾟｽ
//! @return	ﾃﾞｨﾚｸﾄﾘまでのﾊﾟｽ
//--------------------------------------------------
std::string GetDirPath(const std::string& path);

//--------------------------------------------------
//! @brief 2進数配列から16進数の文字列へ変換
//!
//! ﾌｫｰﾏｯﾄは、0xﾌﾟﾚﾌｨｯｸｽ込みで1ﾊﾞｲﾄごとに空白
//! 例）
//! 「b0000 b1111 b1010 b1010」を入力した場合（右端がbin[0]）
//! 「0x0F 0xAA」
//! @param[in]		bin					2進数配列
//! @return	16進数文字列
//--------------------------------------------------
std::string BinToHexString(const std::vector<bool>& bin);


//-------------------------------------------------------------------
//! @brief 起動からの経過時間msec（double）で取得
//-------------------------------------------------------------------
double GetTimeMsec(void);


typedef std::chrono::steady_clock::time_point TP;

std::chrono::steady_clock::time_point getTime();

uint64_t getTimeDiff(std::chrono::steady_clock::time_point nTS, std::chrono::steady_clock::time_point nTE);

float getFPS(std::chrono::steady_clock::time_point nTS, std::chrono::steady_clock::time_point nTE, int nNumFrames, uint64_t nTimeAdj = 0);
