#pragma once
#ifndef _GlobalH
#define _GlobalH

#include <pthread.h>
#include <string>
#include "Sysdef.h"
#include "ErrorManager.hpp"
#include "Param.hpp"

/*===	global variable				===*/
extern ulong	g_ulDebugLog0;													// Debugログスイッチ
extern std::string	g_debugOutputPath;											// 推論結果出力ﾊﾟｽ
extern ushort	g_usTempMoni;													// 温度ﾓﾆﾀ
extern float	g_fCoreTempThresh;												// 温度上昇閾値
extern ushort	g_usWattageMoni;												// 電力ﾓﾆﾀ
extern ushort	g_usRecoderMinutes;												// ﾚｺｰﾀﾞｰ記録時間設定
extern ushort	g_usRecoderFps;													// ﾚｺｰﾀﾞｰ記録FPS設定
extern pthread_mutex_t  g_SerialMutex;											// Serial通信Mutex
extern ConfigData	g_configData;												//!< ｺﾝﾌｨｸﾞﾌｧｲﾙのﾃﾞｰﾀ
extern CErrorManager g_errorManager;											//!< ｴﾗｰ管理ｸﾗｽ
extern ushort	g_usRecoderEna;													// レコーダー機能有効(0:無効/1:有効)
extern ushort	g_usAlertEna;													// 警告機能有効(0:無効/1:有効)
extern ushort	g_usInferEna;													// 推論機能有効(0:無効/1:有効)
extern ushort	g_usAccOffTimer;												// ACC OFF保持タイマ(秒)
extern std::string g_areabinFilePath;                                           // バイナリファイル呼び出し用
extern T_LUT_CONFIG g_LutConfig[LUT_DATA_MAX][2];                               // LUTの選択用(4K)
extern T_LUT_CONFIG g_LutConfigDETECT[1];                                       // LUTの選択用(LUT10)
extern int	g_lut_favorite_table[LUT_PTO_TYPE][MFD_FAVORITE_MAX][MONITOR_MAX];	// MFDのお気に入りテーブル
extern uchar g_areadata[IMAGE_HEIGHT_FHD][IMAGE_WIDTH_FHD][1];
extern T_CAN_RECV_DATA g_CanRecvData;                                           // CAN受信データ
extern T_MFD_BUTTON_CONTROL g_MfdButtonCtrl;									// MFDのボタン状態制御
extern T_RTC_DAT g_RTC;                                                         // 時刻表示
extern int g_DetectState;                                                       // 物体認識 危険/人物検知判定状態
extern int g_capcomp;															// Capture完了フラグ
extern int g_chsize;															// CANコマンドによるサイズ変更フラグ
extern int g_updatestatus;														// アップデート状況
extern bool g_wdt_occerd;														// WDT発生有無
extern int g_survival_cnt;														// 生存確認ﾁｪｯｸ用ｶｳﾝﾄ
extern int g_survival_flg;														// 生存確認ﾁｪｯｸﾌﾗｸﾞ
extern std::string g_app_ver;													// アプリケーションバージョン
extern std::string g_train_model_ver;											// 学習データバージョン
extern std::string g_fpga_ver;													// FPGAバージョン
extern int g_app_state;															// アプリケーションステータス
extern int g_gb_mode;															// 動作モード
extern int g_checktimer_flg;													// タイマー満了フラグ
extern int g_checktimer_flg2;													// タイマー満了フラグ
extern int g_remove_req;														// 録画データ削除要求
extern int g_remove_comp;														// 録画データ削除完了
extern int g_remove_result;														// 録画データ削除結果
// バージョン情報
extern const char	*PRODUCT;
extern const char	*TDN_VER;
extern const char	*TDN_REV;
extern const char	*TDN_TYPE;
extern const char	*VERSION;
extern const char	*RELEASE;
extern const char	*RELEASE1;

#ifdef SD_LOG_ENABLE
extern VideoStatuType g_video_status;
#endif

#endif
