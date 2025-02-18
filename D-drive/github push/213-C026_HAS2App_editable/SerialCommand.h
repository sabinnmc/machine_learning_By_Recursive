/********************************************************************************
;*                                                                              *
;*        Copyright (C) 2024 PAL Inc. All Rights Reserved                       *
;*                                                                              *
;********************************************************************************
;*                                                                              *
;*        開発製品名    :    HAS2					                            *
;*                                                                              *
;*        ﾓｼﾞｭｰﾙ名      :    SirialCommand.h                                    *
;*                                                                              *
;*        作　成　者    :    内之浦　伸治                                       *
;*                                                                              *
;********************************************************************************/

#ifndef SIRIALCOMMAND_H_
#define SIRIALCOMMAND_H_

/*===    file include        ===*/
#include "Sysdef.h"

/*===    user definition        ===*/
// 伝文フォーマット
#define OPC_HEADER_SIZE 		4				// 伝文ﾍｯﾀﾞｻｲｽﾞ
#define OPC_CRC_SIZE			1				// 伝文CRCｻｲｽﾞ
#define OPC_DATA_SIZE_MIN		1				// 伝文ﾃﾞｰﾀ最小ｻｲｽﾞ
#define OPC_DATA_SIZE_MAX		72				// 伝文ﾃﾞｰﾀ最大ｻｲｽﾞ
#define OPC_SEND_DATA_SIZE_MAX	90				// 送信伝文ﾃﾞｰﾀ最大ｻｲｽﾞ
#define OPC_SIZE_MIN	(OPC_HEADER_SIZE+OPC_CRC_SIZE+OPC_DATA_SIZE_MIN)
#define OPC_SIZE_MAX	(OPC_HEADER_SIZE+OPC_CRC_SIZE+OPC_DATA_SIZE_MAX)
#define OPC_SEND_SIZE_MAX	(OPC_HEADER_SIZE+OPC_CRC_SIZE+OPC_SEND_DATA_SIZE_MAX)
#define OPC_SEND_QUEUE_NUM		10				// 伝文送信ｷｭｰｻｲｽﾞ

// 伝文ID定義
#define OPC_ID_SOM_STATUS		0x10			// SoMｽﾃｰﾀｽ通知
#define OPC_ID_ERROR_INFO		0x11			// ｴﾗｰ情報通知
#define OPC_ID_RTC_SETREQ		0x44			// RTC設定要求
#define OPC_ID_RTC_SETACK		0x45			// RTC設定応答
#define OPC_ID_ERROR_RESET		0x4A			// ｴﾗｰﾘｾｯﾄ
#define OPC_ID_ERROR_DETECT		0x4B			// ｴﾗｰ検知通知
#define OPC_ID_ERROR_DETAIL		0x4C			// ｴﾗｰ詳細通知
#define OPC_ID_SOUND_REQ		0x4E			// 再生音声要求
#define OPC_ID_SOUND_INFO		0x4F			// 再生音声通知
#define OPC_ID_WAITTIME_INFO	0x50			// 待機時間設定通知
#define OPC_ID_WARNING_OUTPUT	0x51			// 警告信号出力指示
#define OPC_ID_SURVIVAL_REQ		0x60			// 生存確認要求
#define OPC_ID_SERVIVAL_ACK		0x61			// 生存確認応答
#define OPC_ID_SURVIVAL_SETTING	0x70			// 生存確認設定
#define OPC_ID_GETDATE_REQ		0x90			// 時刻取得要求
#define OPC_ID_GETDATE_ACK		0x91			// 時刻取得応答
#define OPC_ID_VERSION_REQ		0x92			// ﾊﾞｰｼﾞｮﾝ確認要求
#define OPC_ID_VERSION_ACK		0x93			// ﾊﾞｰｼﾞｮﾝ確認応答
#define OPC_ID_SDCARD_TEST_REQ	0x94			// SDｶｰﾄﾞﾃｽﾄ要求
#define OPC_ID_SDCARD_TEST_ACK	0x95			// SDｶｰﾄﾞﾃｽﾄ応答
#define OPC_ID_FPGA_VERSION_REQ	0x97			// FPGAﾊﾞｰｼﾞｮﾝ確認要求
#define OPC_ID_FPGA_VERSION_ACK	0x98			// FPGAﾊﾞｰｼﾞｮﾝ確認応答

#pragma pack(1)
// 伝文ﾍｯﾀﾞ
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
} OPCHEADER;

// SoMｽﾃｰﾀｽ通知伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct{
		union {
			uchar data;
			struct{
				uchar linuxstate	: 2;		// Linux準備完了
				uchar appstate		: 2;		// ｱﾌﾟﾘｹｰｼｮﾝ準備完了
				uchar dummy 		: 4;		// ﾀﾞﾐｰ
			}bit;
		} status;								// 起動状態
		uchar	mode;							// ﾓｰﾄﾞ
		uchar	update;							// ｱｯﾌﾟﾃﾞｰﾄ
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC010;

// ｴﾗｰ情報通知伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		ErrorInfoType info;							// ｴﾗｰ情報
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC011;

// RTC設定要求伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct{
		uchar		year;						// 年
		uchar		month;						// 月
		uchar		day;						// 日
		uchar		hour;						// 時
		uchar		minite;						// 分
		uchar		second;						// 秒
		uchar		dummy[2];					// Dummy
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC044;

// RTC設定応答伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct{
		uchar	result;							// 結果
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC045;

// ｴﾗｰ検知通知伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		ErrorInfoType info;						// ｴﾗｰ情報
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC04B;

// ｴﾗｰ詳細通知伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		ErrorDetailType	detail;
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC04C;

// 音声再生要求伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		uchar soundId;							// 音声ﾊﾟﾀｰﾝ
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC04E;

// 再生音声通知伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct{
		uchar soundId;							// 音声ﾊﾟﾀｰﾝ
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC04F;

// 待機時間設定通知伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct{
		uchar waitTime;							// 待機時間(分)0～60
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC050;

// 警告信号出力指示伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct{
		uchar isWarning;						// 警告信号出力 0:OFF 1:ON
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC051;

// 生存確認要求伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		uchar	dummy;							// ﾀﾞﾐｰ
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC060;

// 生存確認応答伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		uchar	dummy;							// ﾀﾞﾐｰ
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC061;

// 生存確認設定伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		uchar	enable;							// 有効無効設定
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC070;

// 時刻取得要求伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		uchar	dummy;							// ﾀﾞﾐｰ
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC090;

// 時刻取得応答伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		uchar	result;							// 結果
		uchar	year;							// 年
		uchar	month;							// 月
		uchar	day;							// 日
		uchar	hour;							// 時
		uchar	minite;							// 分
		uchar	second;							// 秒
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC091;

// ﾊﾞｰｼﾞｮﾝ確認要求伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		uchar	dummy;							// ﾀﾞﾐｰ
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC092;

#define OPC_APP_VERSION		10
#define OPC_DATA_VERSION	10
#define OPC_KERNEL_VERSION	70
// ﾊﾞｰｼﾞｮﾝ確認応答伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		char appverion[OPC_APP_VERSION];		// ｱﾌﾟﾘﾊﾞｰｼﾞｮﾝ
		char dataverion[OPC_DATA_VERSION];		// 学習ﾃﾞｰﾀﾊﾞｰｼﾞｮﾝ
		char kernelverion[OPC_KERNEL_VERSION];	// Kernelﾊﾞｰｼﾞｮﾝ
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC093;

// SDｶｰﾄﾞﾃｽﾄ要求伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		uchar	dummy;							// ﾀﾞﾐｰ
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC094;

// SDｶｰﾄﾞﾃｽﾄ応答伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		uchar	result;							// 結果
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC095;


// FPGAﾊﾞｰｼﾞｮﾝ確認要求伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		uchar	dummy;							// ﾀﾞﾐｰ
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC097;

#define OPC_FPGA_VERSION	22
// FPGAﾊﾞｰｼﾞｮﾝ確認応答伝文
typedef struct{
	ushort	id;									// OPC ID
	ushort	len;								// ﾃﾞｰﾀｻｲｽﾞ
	struct {
		char fpgaversion[OPC_FPGA_VERSION];		// FPGAﾊﾞｰｼﾞｮﾝ
	} data;
	uchar	sum;								// ﾁｪｯｸｻﾑ
} OPC098;

// 受信伝文定義
typedef union {
	uchar data[OPC_SIZE_MAX];
	OPCHEADER	header;							// ﾍｯﾀﾞｰ情報
	OPC044		opc044;							// RTC設定要求伝文
	OPC04B		opc04B;							// ｴﾗｰ検知通知伝文
	OPC04C		opc04C;							// ｴﾗｰ詳細通知伝文
	OPC04E		opc04E;							// 音声再生要求伝文
	OPC060		opc060;							// 生存確認要求伝文
	OPC090		opc090;							// 時刻取得要求伝文
	OPC092		opc092;							// ﾊﾞｰｼﾞｮﾝ確認要求伝文
	OPC094		opc094;							// SDｶｰﾄﾞﾃｽﾄ要求伝文
	OPC097		opc097;							// FPGAﾊﾞｰｼﾞｮﾝ確認要求伝文
} OpcDataType;


// 送信伝文定義
typedef union {
	uchar data[OPC_SEND_SIZE_MAX];
	OPCHEADER	header;							// ﾍｯﾀﾞｰ情報
	OPC010		opc010;							// SoMｽﾃｰﾀｽ通知伝文
	OPC011		opc011;							// ｴﾗｰ情報通知伝文
	OPC045		opc045;							// RTC設定応答伝文
	OPC04F		opc04F;							// 再生音声通知伝文
	OPC050		opc050;							// 待機時間設定通知伝文
	OPC051		opc051;							// 警告信号出力指示伝文
	OPC061		opc061;							// 生存確認応答伝文
	OPC070		opc070;							// 生存確認設定伝文
	OPC091		opc091;							// 時刻取得応答伝文
	OPC093		opc093;							// ﾊﾞｰｼﾞｮﾝ確認応答伝文
	OPC095		opc095;							// SDｶｰﾄﾞﾃｽﾄ応答伝文
	OPC098		opc098;							// FPGAﾊﾞｰｼﾞｮﾝ確認応答伝文
} OpcSendDataType;
#pragma pack()


/*===    external variable    ===*/

/*===    external function    ===*/
extern void rcvopc_notice(OpcDataType);
extern void som_status_send_req(int, int, int);
extern void error_info_send_req(void);
extern void warning_output_send_req(int);
extern void waittime_info_send_req(ushort);
extern void servival_setting_send_req(bool);
extern void RL78CommunicationInit(void);
extern void ExecRL78Command(void);


/*===    public function prototypes    ===*/

/*===    static function prototypes    ===*/

/*===    static typedef variable        ===*/

/*===    static variable        ===*/

/*===    constant variable    ===*/

/*===    global variable        ===*/

/*===    implementation        ===*/

/***********************************************************************************************************************
Global functions
***********************************************************************************************************************/

#endif /* SIRIALCOMMAND_H_ */
