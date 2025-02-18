//--------------------------------------------------
//! @file Sysdef.h
//! @brief システム定義
//--------------------------------------------------

#ifndef SysdefH
#define SysdefH

/*===	file include		===*/

#include <time.h>
#include <syslog.h>
#include <vector>
#include <string>
#include "Typedef.h"
#include <opencv2/opencv.hpp>
#include <atomic>

/*===	user definition		===*/
/////   SYSTEM CONFIG       ===
#define CRANE_MODEL_MAX 1


#define INPUT_TVI720P			// カメラを720p入力にする場合（カーネルの変更も必要）
#define SD_LOG_ENABLE			// 出力映像のブラックアウト調査用のSDログ残すモード

// システム関連
#define MONITOR_MAX		2	    // 出力映像CH数

//--------------------------------------------------
//! @brief ｴﾗｰｺｰﾄﾞ
//!
//! 全てのｴﾗｰを登録するわけではない
//! 特に、CAN通信などでｴﾗｰを送信する場合には不要
//! 主な目的はOS絡みの予期せぬｴﾗｰ処理などである
//--------------------------------------------------
enum class ErrorCode{
	None = 0,						//!< ｴﾗｰなし
	PulseWDT,						//!< WDT専用線出力 ｴﾗｰ
	OutputOSWakeup,					//!< OS起動完了信号を出力 ｴﾗｰ
	IsShutdownReq,					//!< ｼｬｯﾄﾀﾞｳﾝ信号を受信したか？ ｴﾗｰ
	OutputAppWakeup,				//!< ｱﾌﾟﾘ起動完了信号を出力 ｴﾗｰ
	GetMicrowattsOfSOM,				//!< SOMの消費電力（μW）を取得 ｴﾗｰ
	GetInternalTempZone0,			//!< CPU内部の温度（℃）を取得 ｿﾞｰﾝ0 ｴﾗｰ
	GetInternalTempZone1,			//!< CPU内部の温度（℃）を取得 ｿﾞｰﾝ1 ｴﾗｰ
	GetExternalTemp,				//!< 温度ｾﾝｻｰの温度（℃）を取得 ｴﾗｰ
	GetRTCStatus,					//!< RTCのｽﾃｰﾀｽを取得 ｴﾗｰ
	LowBatteryRTC,					//!< RTCのLBATの電圧低下を検出
	OscillationStopRTC,				//!< RTCの発振停止を検出
	NotFoundTempDevice,				//!< 温度ｾﾝｻが見つからない
	NotFoundPowerDevice,			//!< 電源が見つからない
	Shutdown,						//!< ｼｬｯﾄﾀﾞｳﾝ処理失敗
	Reboot,							//!< ﾘﾌﾞｰﾄ処理失敗
	NotFoundMountPoint,				//!< ﾏｳﾝﾄﾎﾟｲﾝﾄが見つからない
	UnreadableAccOnGPIO,			//!< ACCを確認するGPIOが読めなかった
	UnreadableReverseSigGPIO,		//!< ﾊﾞｯｸ信号を確認するGPIOが読めなかった
	StartUpFPGA,					//!< FPGAのリセット解除失敗
	SetVideoSelect,					//!< VideoSelect切替失敗
	Canenable,						//!< Canenable失敗
	GetFPGAVersion,					//!< FPGAのﾊﾞｰｼﾞｮﾝ取得失敗
	Max								//!< ｴﾗｰｺｰﾄﾞ最大数
};


#define IMAGE_WIDTH				831								// 画像幅
#define IMAGE_HEIGHT			416								// 画像高さ
#define IMAGE_WIDTH_WIDE		1280							// 画像幅(録画用)
#define IMAGE_HEIGHT_WIDE		720								// 画像高さ(録画用)
#define IMAGE_WIDTH_4k			3840							// 画像幅(録画用)
#define IMAGE_HEIGHT_4k			2160							// 画像高さ(録画用)							// 画像高さ(録画用)
#define IMAGE_WIDTH_FHD 		1920							// 画像幅(録画用)
#define IMAGE_HEIGHT_FHD		1080							// 画像高さ(録画用)
#define YOLO_IMAGE_WIDTH 		416								// 画像幅(検知用)
#define YOLO_IMAGE_HEIGHT 		416								// 画像幅(検知用)
#define THICKNESS 				6

// 物体認識判定定義
#define YOLO_PERSON_DANGER_JUDGE		0x01                     // YOLO(Person)危険判定
#define YOLO_PERSON_DETECT_JUDGE		0x02                     // YOLO(Person)人物検知判定 				// person dedtection judgment
#define YOLO_PERSON_CAMADJ_JUDGE		0x04                     // YOLO(Person)カメラ位置ずれ点検領域検知判定
#define YOLO_DETECTION_STOP				0x10                     // 検知停止

// ログ定義
#if 1
#define LOG(filter,fmt,...) if(g_ulDebugLog0 & filter){							\
							fprintf(stdout, fmt"\n", ## __VA_ARGS__);					\
							syslog(LOG_INFO,fmt" |(info)", ## __VA_ARGS__);		\
							fflush(stdout);	\
						}

#define WARNING(fmt,...) fprintf(stderr,fmt"\n", ## __VA_ARGS__); syslog( LOG_WARNING,fmt" |(warning)", ## __VA_ARGS__); fflush(stderr);

#define ERR(fmt,...) fprintf(stderr,fmt"\n", ## __VA_ARGS__); syslog(LOG_ERR,fmt" |(error)", ## __VA_ARGS__); fflush(stderr);

#define TEMP_DEBUG(...)	syslog(LOG_DEBUG,__VA_ARGS__);

#define DEBUG_LOG(...)	syslog(LOG_DEBUG,__VA_ARGS__);

#define STD_LOG(fmt,...) {	\
		fprintf(stdout, fmt"\n", ## __VA_ARGS__);		\
		syslog(LOG_INFO,fmt, ## __VA_ARGS__);	\
		fflush(stdout);									\
	}

#else
#define LOG(filter,...) if(g_ulDebugLog0 & filter){							\
							fprintf(stdout, __VA_ARGS__);					\
							fflush(stdout);	\
						}
#define ERR(...) fprintf(stderr, __VA_ARGS__); fflush(stderr);
#endif

extern ulong	g_ulDebugLog0;													// Debugログスイッチ

//--------------------------------------------------
//! @brief ｱﾌﾟﾘﾛｸﾞ出力
//! @param[in]		text				表示したいﾒｯｾｰｼﾞ
//--------------------------------------------------
inline void OutputAppLog(int filter, const std::string& text){
	LOG(filter, "%s", text.c_str());
}

//--------------------------------------------------
//! @brief 警告ﾛｸﾞ出力
//! @param[in]		text				表示したいﾒｯｾｰｼﾞ
//--------------------------------------------------
inline void OutputWarningLog(const std::string& text){
	WARNING("%s", text.c_str());
}

//--------------------------------------------------
//! @brief ｴﾗｰﾛｸﾞ出力
//! @param[in]		text				表示したいﾒｯｾｰｼﾞ
//--------------------------------------------------
inline void OutputErrorLog(const std::string& text){
	ERR("%s", text.c_str());
}


// ログフィルタ定義
#define LOG_COMMON_OUT					0x0001					 // 共通ログ出力
#define LOG_DANGER_JUDGE				0x0002					 // 危険判定
#define LOG_YOLO_INFO					0x0004					 // YOLO情報
#define LOG_STARTUP						0x0008					 // 始業点検
#define LOG_SERIAL_DEBUG				0x0010					 // ｼﾘｱﾙ通信ﾃﾞﾊﾞｯｸﾞ
#define LOG_OPC_DEBUG					0x0020					 // 伝文ﾃﾞﾊﾞｯｸﾞ
#define LOG_SD_DEBUG					0x0040					 // SDｶｰﾄﾞﾃﾞﾊﾞｯｸﾞ
#define LOG_RECODER_DEBUG				0x0080					 // ﾚｺｰﾀﾞｰﾃﾞﾊﾞｯｸﾞ
#define LOG_SOUND_DEBUG					0x0100					 // 音ﾃﾞﾊﾞｯｸﾞ
#define LOG_DIRECTION_DEBUG				0x0200					 // 移動方向判定ﾃﾞﾊﾞｯｸﾞ
#define LOG_MEMORY_DEBUG				0x0800					 // ﾒﾓﾘﾃﾞﾊﾞｯｸﾞ
#define LOG_DETAIL_DEBUG				0x1000					 // 詳細ﾃﾞﾊﾞｯｸﾞ


// SDｶｰﾄﾞ関連
#define MOUNT_SDCARD_SRC_PATH	"/dev/mmcblk1p1"				// SDｶｰﾄﾞﾏｳﾝﾄ元
#define MOUNT_SDCARD_DIR		"/mnt/card"						// SDｶｰﾄﾞﾏｳﾝﾄ先

// USB関連
#define MOUNT_USB_SRC_PATH	"/dev/sda1"							// USBﾏｳﾝﾄ元
#define MOUNT_USB_DIR		"/mnt/usb"							// USBﾏｳﾝﾄ先

const std::string MOUNT_USB_SSD_DIR = "/mnt/usb";			    // USBﾏｳﾝﾄ先

// ﾚｺｰﾀﾞｰ関連
const std::string MOUNT_REDODER_DIR = "/mnt/card";			// SSDﾏｳﾝﾄ先
const std::string RECODER_DIR = "recoder";						// 動画保存ﾌｫﾙﾀﾞ
constexpr long SSD_DEL_CHECK_FREE_SIZE		= 1024L*1024L*1024L;				// 空き容量サイズ(削除確認)
constexpr long SSD_SAVE_CHECK_FREE_SIZE		= 512L*1024L*1024L;			// 空き容量サイズ(保存確認)
#define ADD_FILE_NAME			"_cam"							// ﾌｧｲﾙ名への付加文字
#define RECODER_INDEX_NAME		"index"

// 学習ﾃﾞｰﾀ関連
#define WEIGHTS_FLDR "./model/"								// 学習ﾃﾞｰﾀ格納ﾌｫﾙﾀﾞ
#define MNSSD_FILENAME_WEIGHTS "TrainData.bin"					// Yolo学習ﾃﾞｰﾀﾌｧｲﾙ名
#define MNSSD_BK_FILENAME_WEIGHTS "TrainData_bk.bin"			// Yolo学習ﾃﾞｰﾀﾊﾞｯｸｱｯﾌﾟﾌｧｲﾙ名

// 推論ﾃﾞﾊﾞｲｽ関連
#define HAILO_DEVICE			"/dev/hailo0"

// ｱｯﾌﾟﾃﾞｰﾄ関連
#define UPDATE_FILE			"DetectionAppUpdate.zip"			// ｱｯﾌﾟﾃﾞｰﾄ圧縮ﾌｧｲﾙ名
#define UPDATE_PACK_PREFIX	"DetectionAppPackage_"				// ｱｯﾌﾟﾃﾞｰﾄﾃﾞｰﾀﾊﾟｯｹｰｼﾞﾌﾟﾚﾌｨｯｸｽ
#define UPDATE_PACK_FILE	"DetectionAppPackage_After.zip"		// ｱｯﾌﾟﾃﾞｰﾄﾃﾞｰﾀﾊﾟｯｹｰｼﾞﾌｧｲﾙ
#define UPDATE_APP_FILE		"Has2App"					        // ｱﾌﾟﾘﾌｧｲﾙ名
#define UPDATE_TRAIN_FILE	MNSSD_FILENAME_WEIGHTS			    // 学習ﾃﾞｰﾀﾌｧｲﾙ名
#define UPDATE_PACK_MD5		"package.md5"					    // ﾊﾟｯｹｰｼﾞMD5ﾁｪｯｸｻﾑﾌｧｲﾙ名
#define UPDATE_PASSWORD		"pal"						        // 圧縮ﾌｧｲﾙﾊﾟｽﾜｰﾄﾞ

// ｴﾗｰ情報関連
#define ERRINFO_FLDR "./Info/"									// ｴﾗｰ情報格納ﾌｫﾙﾀﾞ
#define ERRINFO_NAME "errinfo.dat"								// ｴﾗｰ情報ﾌｧｲﾙ
#define ERRDETAIL_NAME "errdetail.dat"							// ｴﾗｰ詳細ﾌｧｲﾙ

// 設定関係
#define CONFIG_FLDR			"./config/"
#define CONFIG_FILE			"config.cfg"
#define AREA_FLDR			"./area/"
#define LUT_FLDR			"./lut/"
#define FPGA_FLDR			"./fpga/"

const int WDT_PULSE_TIME	= 500;								//!< WDTﾊﾟﾙｽ更新の頻度（ms）
const int INFERENCE_ERROR_TIME = 5'000'000;						//!< 推論失敗と判定する時間(us)


// ACC関連
#define POWER_LATCH_TIME	3600								// 電源保持時間(60min)

// 魚眼関係
#define FISHEYE_FCOL_OFFSET 2030            // Front colum
#define FISHEYE_RCOL_OFFSET 230             // Rear colum
#define FISHEYE_ROW_OFFSET  200             // fisheye row
#define FISHEYE_COL_CUT     1800            // Rear colum
#define FISHEYE_ROW_CUT     1800            // fisheye row
#define FISHEYE_FCOL_OFFSET_DETECT 416      // Front colum
#define FISHEYE_RCOL_OFFSET_DETECT 0        // Rear colum
#define FISHEYE_ROW_OFFSET_DETECT  0        // fisheye colum
#define FISHEYE_COL_CUT_DETECT     416      // Rear colum
#define FISHEYE_ROW_CUT_DETECT     416      // fisheye colum


// ｱﾌﾟﾘｹｰｼｮﾝｽﾃｰﾀｽ
enum{
	APPSTATE_NONE = 0,
	APPSTATE_LINUX_STARTUP,
	APPSTATE_APP_STARTUP,
	APPSTATE_MAX
};

struct boundingbox;
struct BboxInfo{
	int	cls_idx;
	int	x1;
	int	y1;
	int	x2;
	int	y2;
	int	flg;
	float score;
	BboxInfo(){}
	BboxInfo(const boundingbox& box);
	BboxInfo& operator=(const boundingbox& box);
};

struct BboxJudge{
	int judge;
	BboxInfo box;
};

typedef struct{
	int flag;
	double judge_time;
} JudgeInfo;


// 動作ﾓｰﾄﾞ
enum {
	FL_MODE_NORMAL = 0, 						// 通常動作モード
	FL_MODE_MAX
};

// ｱｯﾌﾟﾃﾞｰﾄ状態
enum {
	UPDSTATE_NONE,								// 非ｱｯﾌﾟﾃﾞｰﾄ状態
	UPDSTATE_EXEC,								// ｱｯﾌﾟﾃﾞｰﾄ中
	UPDSTATE_COMPLETE,							// ｱｯﾌﾟﾃﾞｰﾄ完了
	UPDSTATE_FAILED,							// ｱｯﾌﾟﾃﾞｰﾄ失敗
	UPDSTATE_MAX
};


// ｴﾗｰ情報
typedef struct{
	struct {
		uchar impprc_wdt		: 2;	// 画像処理ﾌﾟﾛｾｯｻWDT異常
		uchar pwvlt_uplimit		: 2;	// 電源電圧上昇異常
		uchar pwvlt_downlimit	: 2;	// 電源電圧低下異常
		uchar serial			: 2;	// ｼﾘｱﾙ通知異常
	} err1;									// ｴﾗｰ1
	struct {
		uchar os				: 2;	// OS異常
		uchar application		: 2;	// ｱﾌﾟﾘｹｰｼｮﾝ異常
		uchar imgprc			: 2;	// 画像処理ﾌﾟﾛｾｯｻ異常
		uchar imgprc_temp		: 2;	// 画像処理ﾌﾟﾛｾｯｻ温度異常
	} err2;									// ｴﾗｰ2
	struct {
		uchar rtc				: 2;	// RTC異常
		uchar mntor_out			: 2;	// ﾓﾆﾀ出力異常	(ｴﾗｰ検知として無効)
		uchar input_img			: 2;	// 映像異常
		uchar data_save			: 2;	// ﾃﾞｰﾀ保存異常
	} err3;									// ｴﾗｰ3
	struct {
		uchar base_cpu			: 2;	// ﾍﾞｰｽ基板CPU異常
		uchar input_area		   : 2;    // エリア画像以上
		uchar dummy				: 6;	// ﾀﾞﾐｰ
	} err4;									// ｴﾗｰ4
} ErrorInfoType;

// ｴﾗｰ詳細情報
typedef struct{
	struct {
		uchar ram_chk			: 1;	// RAMﾁｪｯｸ異常
		uchar cpu_chk			: 1;	// CPUﾁｪｯｸ異常
		uchar rom_chksum		: 1;	// ROMﾁｪｯｸｻﾑ異常
		uchar cpu_err			: 1;	// CPU例外発生
		uchar dummy				: 4;	// ﾀﾞﾐｰ
	} err1;								// ｴﾗｰ1
	struct {
		uchar rst_illegalorder	: 1;	// 不正命令実行によるﾘｾｯﾄ
		uchar rst_wdt_clock		: 1;	// WDTまたはｸﾛｯｸモニタ異常によるﾘｾｯﾄ
		uchar rst_illegalaccess	: 1;	// 不正ﾒﾓﾘアクセスによるﾘｾｯﾄ
		uchar rst_pwr_donw		: 1;	// 電圧降下によるﾘｾｯﾄ
		uchar dummy				: 4;	// ﾀﾞﾐｰ
	} err2;								// ｴﾗｰ1
	struct {
		uchar batmon_upper		: 1;	// BATMON上昇異常
		uchar mon_24v_upper		: 1;	// MON_24V上昇異常
		uchar mon_3r3v_upper	: 1;	// MON_3R3V上昇異常
		uchar mon_5v_upper		: 1;	// MON_5V上昇異常
		uchar mon_1r8vd_upper	: 1;	// MON_1R8VD上昇異常
		uchar mon_3r3va_upper	: 1;	// MON_3R3VA上昇異常
		uchar mon_1r8va_upper	: 1;	// MON_1R8VA上昇異常
		uchar mon_1r8vap_upper	: 1;	// MON_1R8VAP上昇異常
	} err3;
	struct {
		uchar mon_12va_upper	: 1;	// MON_12VA上昇異常
		uchar dummy				: 7;	// ﾀﾞﾐｰ
	} err4;
	struct {
		uchar batmon_lower		: 1;	// BATMON低下異常
		uchar mon_24v_lower		: 1;	// MON_24V低下異常
		uchar mon_3r3v_lower	: 1;	// MON_3R3V低下異常
		uchar mon_5v_lower		: 1;	// MON_5V低下異常
		uchar mon_1r8vd_lower	: 1;	// MON_1R8VD低下異常
		uchar mon_3r3va_lower	: 1;	// MON_3R3VA低下異常
		uchar mon_1r8va_lower	: 1;	// MON_1R8VA低下異常
		uchar mon_1r8vap_lower	: 1;	// MON_1R8VAP低下異常
	} err5;
	struct {
		uchar mon_12va_lower	: 1;	// MON_12VA低下異常
		uchar dummy				: 7;	// ﾀﾞﾐｰ
	} err6;
	struct {
		uchar psv_ch0			: 1;    // CAN通信エラーパッシブフラグ
		uchar bof_ch0			: 1;    // CAN通信バスオフグラグ
		uchar psv_ch1			: 1;    // CAN通信エラーパッシブフラグ
		uchar bof_ch1			: 1;    // CAN通信バスオフグラグ
		uchar dummy  			: 4;    // ﾀﾞﾐｰ
	} err7;
	struct {
		uchar mfd_disp			: 1;    // MFD通信異常フラグ(断線異常)
		uchar vcu_disp			: 1;    // VCU通信異常フラグ(断線異常)
		uchar cap_stat			: 1;    // Capture異常フラグ(断線異常)
		uchar dummy   			: 5;    // ﾀﾞﾐｰ
	} err8;
} ErrorDetailType;



// ｼﾘｱﾙ関連
#define SERIAL_BUF_SIZE			1024			// ｼﾘｱﾙﾊﾞｯﾌｧｻｲｽﾞ

// ｼﾘｱﾙ受信ﾊﾞｯﾌｧ
typedef struct {
	uchar buff[SERIAL_BUF_SIZE];
	int	rp;
	int wp;
	int cnt;
} SerialBuffType;

//--------------------------------------------------
//! @brief AI分類
//!
//! ﾓﾃﾞﾙﾌｧｲﾙ作成用ﾂｰﾙと数値を合わせること
//! (model_convertor.py AI_TYPE)
//--------------------------------------------------
enum eAiType{
	AI_TYPE_INVALID = 0,														//!< 設定不可
	AI_TYPE_OBJECT_DETECTION,													//!< 物体検出
	MAX_AI_TYPE																	//!< 最大値
};

//--------------------------------------------------
//! @brief 推論ｴﾝｼﾞﾝ
//!
//! ﾓﾃﾞﾙﾌｧｲﾙ作成用ﾂｰﾙと数値を合わせること
//! (model_convertor.py ENGINE_TYPE)
//--------------------------------------------------
enum eEngineType{
	ENGINE_TYPE_INVALID = 0,													//!< 設定不可
	ENGINE_TYPE_HAILO = 2,														//!< HAILO
	MAX_ENGINE_TYPE																//!< 最大値
};

//--------------------------------------------------
//! @brief ﾓﾃﾞﾙﾀｲﾌﾟ
//!
//! ﾓﾃﾞﾙﾌｧｲﾙ作成用ﾂｰﾙと数値を合わせること
//! (model_convertor.py MODEL_TYPE)
//--------------------------------------------------
enum eModelType{
	MODEL_TYPE_NONE = 0,														//!< ﾓﾃﾞﾙﾀｲﾌﾟが存在しない
	MODEL_TYPE_YOLOV5 = 1,														//!< YOLOV5
	MODEL_TYPE_YOLOX_S,															//!< YOLOX_S
	MODEL_TYPE_YOLOV4_TINY,
	MODEL_TYPE_YOLOX_TINY,
};

//--------------------------------------------------
//! @brief 推論ﾀｲﾌﾟ
//!
//! 前ｶﾒﾗと後ろｶﾒﾗなど、複数の推論を実施する際に種類を増やす
//--------------------------------------------------
enum eInferenceType{
	INFERENCE_TYPE_FIRST = 0,													//!< 1つめ
	//INFERENCE_TYPE_SECOND,													//!< 2つめ
	MAX_INFERENCE_TYPE															//!< 推論ﾀｲﾌﾟの種類数
};

// CAN
#define CAN_CH	            1               // CAN通信CH1
#define CAN_IFNAME_CH       "can1"          // CAN通信 IF名 CH1
#define CAN_MAXSOCK         2
#define MAX_TXPACKET        256             // 送信伝文種数最大(5種類+予備11)
#define MAX_RXPACKET        512
#define TIMER100            0x0001          // 100m周期のタイマ
#define TIMER200            0x0002          // 200ms周期のタイマ
#define TIMER600            0x0006          // 600ms周期のタイマ
#define TIMER1000           0x0010          // 1000ms周期のタイマ
#define TIMER2000           0x0020          // 2000ms周期のタイマ
#define TIMEOUT_CAN100      100             // CAN送信用タイマ(100ms)
#define TIMEOUT_CAN2000     2000            // CAN送信用タイマ(2000ms)
#define TIMEOUT_CAN1000     1000            // CAN送信用タイマ(1000ms)
#define TIMEOUT_MFD			1000			// CAN断線タイマ（1000ms）
#define TIMEOUT_VCU			1000			// CAN断線タイマ（1000ms）
#define TIMER_CRCV_OUT      0x0002          // CAN受信断線タイムアウト
#define TIMER_CPSV_OUT      0x0010          // CANエラーパッシブタイムアウト
#define TIMER_MAX           30
#define RCV_SETTIME         0               // CAN受信断線検知用
#define PSV_SETTIME         1               // CANエラーパッシブタイムアウト検知用
#define GETTIME             2               // 経過時間取得用
#define LUT_DATA_MAX        4               // LTU用のテーブル格納
#define MFD_FAVORITE_MAX	4				// MFDのお気に入り登録数
#define LUT_PTO_TYPE		2				// MFD表示のPTO状態数

/* special address description flags for the CAN_ID */
#define CAN_EFF_FLAG        0x80000000U     // EFF/SFF is set in the MSB
#define CAN_RTR_FLAG        0x40000000U     // remote transmission request
#define CAN_ERR_FLAG        0x20000000U     // error message frame
#define CAN_MFD_FLAG        0xDFU           // error mfd connect
#define CAN_VCU_FLAG        0xD0U           // error vcu connect

/* valid bits in CAN ID for frame formats */
#define CAN_SFF_MASK        0x000007FFU     // standard frame format (SFF)
#define CAN_EFF_MASK        0x1FFFFFFFU     // extended frame format (EFF)
#define CAN_ERR_MASK        0x1FFFFFFFU     // omit EFF, RTR, ERR flags

#define DA_GLOBAL           (0xFFu)
#define PF_TP_BAM           (0xECu)         // 236
#define PF_TP_DT            (0xEBu)         // 235

// エラー
enum {
	kErrCodeDecoder = 0,
	kErrCodeEncoder,
	kErrCodeFPGA,
	kErrCodeCPU,
	kErrCodeWDT,
	kErrCodeDDR,
	kErrCodeVideoLost,
	kErrCodeCANBussoff,
	kErrCodeCANPassive,
	kErrCodeCANLostMFD,
	kErrCodeCANLostVCU,
	kErrCodeMax
};

// ERR CODE
#define	ERCD_DECODER	  3					//ﾃﾞｺｰﾀﾞｴﾗｰｺｰﾄﾞ
#define	ERCD_ENCODER	  4					//ｴﾝｺｰﾀﾞｴﾗｰｺｰﾄﾞ
#define	ERCD_ALTERA		  5					//ｱﾙﾃﾗｴﾗｰｺｰﾄﾞ
#define	ERCD_RESET		  6					//例外ｴﾗｰｺｰﾄﾞ
#define	ERCD_WDT		  7					//ｳｫｯﾁﾄﾞｯｸｴﾗｰｺｰﾄﾞ
#define	ERCD_DDRMEM		  8					//DDRﾒﾓﾘｴﾗｰｺｰﾄﾞ
#define	ERCD_NOSYNC		100					//映像断ｴﾗｰｺｰﾄﾞ
#define	ERCD_BUSSOFF	200					//ﾊﾞｽｵﾌｴﾗｰｺｰﾄﾞ
#define	ERCD_PASSIVE	201					//ｴﾗｰﾊﾟｯｼﾌﾞｴﾗｰｺｰﾄﾞ
#define	ERCD_MFD		202					//MFD通信断ｴﾗｰｺｰﾄﾞ
#define	ERCD_VCU		203					//VCU通信断ｴﾗｰｺｰﾄﾞ

typedef	struct {
	int		s;								// socket
	char	*ifname;
	uint	dropcnt;
	uint	last_dropcnt;
} T_CANFD_INFO;

typedef	struct {
	uint	can_id;  						// 32 bit CAN_ID + EFF/RTR/ERR flags
	uchar	len;     						// frame payload length in byte
	uchar	flags;   						// additional flags for CAN FD
	uchar	__res0;  						// reserved / padding
	uchar	__res1;  						// reserved / padding
	uchar	data[8];
} T_CANFD_FRAME;

typedef struct {
    std::atomic<bool> monitor1{false};
    std::atomic<bool> monitor2{false};
    std::atomic<bool> save_req{false};
    std::atomic<bool> cancel_req{false};
    std::atomic<bool> ack_req{false};
} T_MFD_BUTTON_CONTROL;

typedef	struct {
	uchar	vehicle_speed;
	uchar	stop;
	uchar	left;
	uchar	buzzor;
	uchar	adjust;
	uchar	pto;
	uchar	second;
	uchar	minute;
	uchar	hours;
	uchar	month;
	uchar	day;
	uchar	year;
	uchar	lut1;
	uchar	lut2;
	uchar	mfd_button_state_recv;
	uchar	mfd_favorite0;
	uchar	mfd_favorite1;
	uchar	mfd_change_range;
	uchar	mfd_change_pto;
	uchar	mfd_change_favorite;
} T_CAN_RECV_DATA;

typedef struct {																	//
	union {
		uchar byte;																	//
		struct {
			uchar	do0	:1;															//
			uchar	do1	:1;															//
			uchar	do2	:1;															//
			uchar	do3	:1;															//
			uchar	do4 :1;															//
			uchar	do5 :1;															//
			uchar	do6 :1;															//
			uchar	do7 :1;															//
		} bit;
	} data;
} T_CAN_DO;

typedef struct {
	ulong	sa   		:8;                                                     /* Source Address                   */
	ulong	pgn  		:18;                                                    /* PGN                              */
	ulong	p			:3;                                                     /* Priority                         */
	ulong	dummy		:3;                                                     /* 32bitﾊﾞｳﾝﾀﾞﾘ用                   */
} TCANFD_ARB;

typedef	struct {
	int		msec;																/* ﾐﾘ秒								*/
	struct {
		char	hex;															/* Byte Access (秒)					*/
		struct {
			uchar	un	:4;														/* Nible Access (upper)				*/
			uchar	ln	:4;														/* Nible Access (lower)				*/
		} bcd;
	} sec;
	struct {
		char	hex;															/* Byte Access (分)					*/
		struct {
			uchar	un	:4;														/* Nible Access (upper)				*/
			uchar	ln	:4;														/* Nible Access (lower)				*/
		} bcd;
	} min;
	struct {
		char	hex;															/* Byte Access (時)					*/
		struct {
			uchar	un	:4;														/* Nible Access (upper)				*/
			uchar	ln	:4;														/* Nible Access (lower)				*/
		} bcd;
	} hour;
	struct {
		char	hex;															/* Byte Access (曜日)				*/
		struct {
			uchar	un	:4;														/* Nible Access (upper)				*/
			uchar	ln	:4;														/* Nible Access (lower)				*/
		} bcd;
	} week;
	struct {
		char	hex;															/* Byte Access (日)					*/
		struct {
			uchar	un	:4;														/* Nible Access (upper)				*/
			uchar	ln	:4;														/* Nible Access (lower)				*/
		} bcd;
	} day;
	struct {
		char	hex;															/* Byte Access (月)					*/
		struct {
			uchar	un	:4;														/* Nible Access (upper)				*/
			uchar	ln	:4;														/* Nible Access (lower)				*/
		} bcd;
	} month;
	struct {
		char	hex;															/* Byte Access (西暦下２桁年)		*/
		struct {
			uchar	un	:4;														/* Nible Access (upper)				*/
			uchar	ln	:4;														/* Nible Access (lower)				*/
		} bcd;
	} year;
	char	yearh;																/* 西暦上２桁年						*/
} T_RTC_DAT;

typedef struct{
	uchar candata;
	std::string filename;
} T_LUT_CONFIG;

typedef struct {
	cv::Mat image0;
	cv::Mat image1;
} T_DISP_IMAGES;

enum {
	kUpdateErr=-1,
	kUpdateStop=0,
	kUpdateWait,
	kUpdateStart,
	kUpdateFinsh
};


enum {
	kMfdReqNone,
	kMfdReqConfirm,
	kMfdReqCancel
};

enum {
	kMfdAck,
	kMfdNack
};

#ifdef SD_LOG_ENABLE
typedef struct {
	uint8_t tp2850;
	uint8_t fpga_0;
	uint8_t fpga_1;
	uint8_t tp2912_0;
	uint8_t tp2912_1;
	uint8_t exio;
} VideoStatuType;
#endif

//---------------------------------------------------------------------------
#endif
//-----------end of file-----------------------------------------------------
