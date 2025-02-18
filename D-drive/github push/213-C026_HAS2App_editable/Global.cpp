//--------------------------------------------------
//! @file Global.cpp
//! @brief ｸﾞﾛｰﾊﾞﾙ情報管理ﾓｼﾞｭｰﾙ
//--------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "Sysdef.h"
#include "Global.h"

/*===	SECTION NAME		===*/


/*===	user definition		===*/


/*===	pragma variable	===*/


/*===	external variable	===*/


/*===	external function	===*/


/*===	public function prototypes	===*/


/*===	static function prototypes	===*/


/*===	static typedef variable		===*/


/*===	static variable		===*/


/*===	constant variable	===*/


/*===	global variable		===*/
ulong	g_ulDebugLog0;													// Debugログスイッチ
std::string	g_debugOutputPath;											// 推論結果出力ﾊﾟｽ
ushort	g_usTempMoni;													// 温度ﾓﾆﾀ
float	g_fCoreTempThresh;												// 温度上昇閾値
ushort	g_usWattageMoni;												// 電力ﾓﾆﾀ
ushort	g_usRecoderMinutes;												// ﾚｺｰﾀﾞｰ記録時間設定
ushort	g_usRecoderFps;													// ﾚｺｰﾀﾞｰ記録FPS設定
pthread_mutex_t  g_SerialMutex;											// Serial通信Mutex
ConfigData	g_configData;												//!< ｺﾝﾌｨｸﾞﾌｧｲﾙのﾃﾞｰﾀ
CErrorManager g_errorManager;											//!< ｴﾗｰ管理ｸﾗｽ
ushort	g_usRecoderEna;													// レコーダー機能有効(0:無効/1:有効)
ushort	g_usInferEna;													// 推論機能有効(0:無効/1:有効)
ushort	g_usAccOffTimer;												// ACC OFF保持タイマ(秒)
std::string g_areabinFilePath;                                          // バイナリファイル呼び出し用
T_LUT_CONFIG g_LutConfig[LUT_DATA_MAX][MONITOR_MAX];					// LUTの選択用(4K)
T_LUT_CONFIG g_LutConfigDETECT[1];                                      // LUTの選択用(LUT10)
int	g_lut_favorite_table[LUT_PTO_TYPE][MFD_FAVORITE_MAX][MONITOR_MAX];	// MFDのお気に入りテーブル
uchar g_areadata[IMAGE_HEIGHT_FHD][IMAGE_WIDTH_FHD][1];
T_CAN_RECV_DATA	g_CanRecvData;											// CAN受信データ
T_MFD_BUTTON_CONTROL g_MfdButtonCtrl;									// MFDのボタン状態制御
T_RTC_DAT g_RTC;                                                        // 時刻表示
int g_DetectState;                                                      // 物体認識 危険/人物検知判定状態
int g_capcomp;															// Capture完了フラグ
int g_chsize;															// CANコマンドによるサイズ変更フラグ
int g_updatestatus;
bool g_wdt_occerd;														// WDT発生有無通知
int g_survival_cnt;														// 生存確認ﾁｪｯｸ用ｶｳﾝﾄ
int g_survival_flg;														// 生存確認ﾁｪｯｸﾌﾗｸﾞ
std::string g_app_ver;													// アプリケーションバージョン
std::string g_train_model_ver;											// 学習データバージョン
std::string g_fpga_ver;													// FPGAバージョン
int g_app_state;														// アプリケーションステータス
int g_gb_mode;															// 動作モード
int g_checktimer_flg;													// タイマー満了フラグ
int g_checktimer_flg2;													// タイマー満了フラグ
int g_remove_req;														// 録画データ削除要求
int g_remove_comp;														// 録画データ削除完了
int g_remove_result;													// 録画データ削除結果

#ifdef SD_LOG_ENABLE
VideoStatuType g_video_status;
#endif
