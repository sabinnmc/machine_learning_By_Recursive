/********************************************************************************
;*                                                                                *
;*        Copyright (C) 2024 PAL Inc. All Rights Reserved                         *
;*                                                                                *
;********************************************************************************
;*                                                                                *
;*        ﾓｼﾞｭｰﾙ名      :    SirialCommand.cpp                                    *
;*                                                                                *
;*        作　成　者    :    内之浦　伸治                                         *
;*                                                                                *
;********************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>

#include "Sysdef.h"
#include "SerialCommand.h"
#include "BoardCtrl.hpp"
#include "LinuxUtility.hpp"
#include "Global.h"

/*===	SECTION NAME		===*/

/*===	user definition		===*/

/*===	pragma variable	===*/

/*===	external variable	===*/
extern ErrorInfoType errorinfo;									// ｴﾗｰ情報
extern ErrorDetailType errordetail;								// ｴﾗｰ詳細情報

/*===	external function	===*/
extern int serial_send_req(OpcSendDataType);					// SirialCtrl.cpp
extern void error_info_save(ErrorInfoType);						// utility.cpp
extern void error_detail_save(ErrorDetailType);					// utility.cpp
extern void error_reset(ErrorInfoType *info,ErrorDetailType *); // utility.cpp
extern int sdcard_test(void);									// main.

/*===	public function prototypes	===*/
void error_info_send_req(void);

/*===	static function prototypes	===*/

/*===	static typedef variable		===*/

/*===	static variable		===*/
std::vector<OpcDataType> opcdata;											// 受信伝文ﾃﾞｰﾀ
pthread_mutex_t  NoticeMutex;												// 通知用Mutex

/*===	constant variable	===*/

/*===	global variable		===*/


// エラーログ出力マクロ関連
enum class eOpc4BErrID{
	None = 0,										//!< 異常なし
	PastFailure,									//!< 過去故障
	DontCare,										//!< 未定義
	PresentFailure									//!< 現在故障/過去故障
};

// 0x4B通常(err{x}.{target},{"エラーログメッセージ"})
#define OUTPUT_04B_ERROR_LOG(errx_target,msg)	\
		if(static_cast<eOpc4BErrID>(errorinfo.errx_target) != eOpc4BErrID::PresentFailure && static_cast<eOpc4BErrID>(opc.opc04B.data.info.errx_target) == eOpc4BErrID::PresentFailure){	\
			ERR(msg" error occurred :%d", opc.opc04B.data.info.errx_target);}	\
		else if(static_cast<eOpc4BErrID>(errorinfo.errx_target) == eOpc4BErrID::PresentFailure && static_cast<eOpc4BErrID>(opc.opc04B.data.info.errx_target) != eOpc4BErrID::PresentFailure){	\
			LOG(LOG_COMMON_OUT,msg" error resolved :%d", opc.opc04B.data.info.errx_target);	\
		}	\
		errorinfo.errx_target = opc.opc04B.data.info.errx_target | ( errorinfo.errx_target & 0x01);

// 0x4C受信時のエラーログ出力マクロ
// 0x4C通常(err{x}.{target},{"エラーログメッセージ"})
#define OUTPUT_04C_ERROR_LOG(errx_target,msg)	\
		if((errordetail.errx_target) == 0 && (opc.opc04C.data.detail.errx_target) == 1){	\
			ERR(msg" error occurred :%d", opc.opc04C.data.detail.errx_target);}	\
		else if((errordetail.errx_target) == 1 && (opc.opc04C.data.detail.errx_target) == 0){	\
			LOG(LOG_COMMON_OUT,msg" error resolved :%d", opc.opc04C.data.detail.errx_target);	\
		}	\
		errordetail.errx_target = opc.opc04C.data.detail.errx_target;



/*-------------------------------------------------------------------
  FUNCTION :	RL78CommunicationInit()
  EFFECTS  :	受信伝文通知
  NOTE	   :	ベース基板からの受信伝文通知
  -------------------------------------------------------------------*/
void RL78CommunicationInit(void)
{
	pthread_mutex_init(&NoticeMutex, NULL);
}


/*-------------------------------------------------------------------
	FUNCTION :	rcvopc_notice()
	EFFECTS  :	受信伝文通知
	NOTE	 :	ベース基板からの受信伝文通知
-------------------------------------------------------------------*/
void rcvopc_notice(OpcDataType opc)
{
	pthread_mutex_lock(&NoticeMutex);
	opcdata.push_back(opc);
	pthread_mutex_unlock(&NoticeMutex);
}


/*-------------------------------------------------------------------
	FUNCTION :	som_status_send_req()
	EFFECTS  :	SoMステータス通知伝文送信要求
	NOTE	 :	SoMステータス通知伝文送信要求
-------------------------------------------------------------------*/
void som_status_send_req(int state, int mode, int update)
{
	OpcSendDataType opc;
	int i;
	uchar sum = 0;

	memset(&opc,0x00,sizeof(OpcSendDataType));

	opc.opc010.id = 0x10;											// 伝文ID(10h)
	opc.opc010.len = sizeof(OPC010::data);							// 伝文データ長

	// 起動状態設定
	if( state == APPSTATE_LINUX_STARTUP ){
		opc.opc010.data.status.bit.linuxstate = 2;
		opc.opc010.data.status.bit.appstate = 1;
	}
	else{
		opc.opc010.data.status.bit.linuxstate = 2;
		opc.opc010.data.status.bit.appstate = 2;
	}
	opc.opc010.data.mode = (uchar)mode;								// モード
	opc.opc010.data.update = (uchar)update;							// アップデート
	for( i = 0; i < opc.opc010.len; i++ ){
		sum += opc.data[OPC_HEADER_SIZE+i];
	}
	sum = ~sum & 0xFF;
	opc.opc010.sum = sum;

	LOG(LOG_OPC_DEBUG,"[OPC]SoM Status Send ls:%02X as:%02X md:%02X ud:%02X",(int)opc.opc010.data.status.bit.linuxstate,(int)opc.opc010.data.status.bit.appstate,(int)opc.opc010.data.mode,(int)opc.opc010.data.update);
	serial_send_req(opc);
}


/*-------------------------------------------------------------------
	FUNCTION :	error_info_send_req()
	EFFECTS  :	エラー情報通知伝文送信要求
	NOTE	 :
-------------------------------------------------------------------*/
void error_info_send_req(void)
{
	OpcSendDataType opc;
	int i;
	uchar sum = 0;

	memset(&opc,0x00,sizeof(OpcSendDataType));

	opc.opc011.id = 0x11;											// 伝文ID(11h)
	opc.opc011.len = sizeof(OPC011::data);							// 伝文データ長
	opc.opc011.data.info = errorinfo;

	for( i = 0; i < opc.opc011.len; i++ ){
		sum += opc.data[OPC_HEADER_SIZE+i];
	}
	sum = ~sum & 0xFF;
	opc.opc011.sum = sum;

	LOG(LOG_OPC_DEBUG,"[OPC]Error Info Send err1:%02X err2:%02X err3:%02X err4:%02X",(int)opc.data[4],(int)opc.data[5],(int)opc.data[6],(int)opc.data[7]);
	serial_send_req(opc);

}


/*-------------------------------------------------------------------
	FUNCTION :	rtc_ack_send_req()
	EFFECTS  :	RTC設定応答伝文送信要求
	NOTE	 :	RTC設定応答伝文送信要求
-------------------------------------------------------------------*/
void rtc_ack_send_req(int result)
{
	OpcSendDataType opc;
	int i;
	uchar sum = 0;

	memset(&opc,0x00,sizeof(OpcSendDataType));

	opc.opc045.id = OPC_ID_RTC_SETACK;								// 伝文ID(45h)
	opc.opc045.len = sizeof(OPC045::data);							// 伝文ﾃﾞｰﾀ長

	if(result)	opc.opc045.data.result = 1;							// 成功
	else		opc.opc045.data.result = 0;							// 失敗

	for( i = 0; i < opc.opc045.len; i++ ){
		sum += opc.data[OPC_HEADER_SIZE+i];
	}
	sum = ~sum & 0xFF;
	opc.opc045.sum = sum;

	LOG(LOG_OPC_DEBUG,"[OPC]Rtc Response Send ret:%02X",(int)opc.opc045.data.result);
	serial_send_req(opc);
}


/*-------------------------------------------------------------------
	FUNCTION :	waittime_info_send_req()
	EFFECTS  :	待機時間設定通知伝文送信要求
	NOTE	 :
-------------------------------------------------------------------*/
void waittime_info_send_req(ushort sec)
{
	uchar wt = 0;
	OpcSendDataType opc;
	int i;
	uchar sum = 0;

	memset(&opc,0x00,sizeof(OpcSendDataType));

	opc.opc050.id = OPC_ID_WAITTIME_INFO;							// 伝文ID(50h)
	opc.opc050.len = sizeof(OPC050::data);							// 伝文ﾃﾞｰﾀ長

	// 範囲外の対応と秒から分への変換
	if(0 > sec)
	{
		wt = 0;
	}
	else if(3600 < sec)
	{
		wt = (uchar)60;
	}
	else
	{
		wt = (uchar)(sec / (ushort)60);
	}

	opc.opc050.data.waitTime = wt;									// 待機時間(分)0～60

	for( i = 0; i < opc.opc050.len; i++ ){
		sum += opc.data[OPC_HEADER_SIZE+i];
	}
	sum = ~sum & 0xFF;
	opc.opc050.sum = sum;

	LOG(LOG_OPC_DEBUG,"[OPC]WaitTime Info Send time:%02X",(int)opc.opc050.data.waitTime);
	serial_send_req(opc);
}


/*-------------------------------------------------------------------
	FUNCTION :	warning_output_send_req()
	EFFECTS  :	警告信号出力指示伝文送信要求
	NOTE	 :	isOutput 0:OFF 1:ON
-------------------------------------------------------------------*/
void warning_output_send_req(int isOutput)
{
	OpcSendDataType opc;
	int i;
	uchar sum = 0;

	memset(&opc,0x00,sizeof(OpcSendDataType));

	opc.opc051.id = OPC_ID_WARNING_OUTPUT;							// 伝文ID(51h)
	opc.opc051.len = sizeof(OPC051::data);							// 伝文データ長

	// 警告信号出力 0:OFF 1:ON
	if(isOutput)	opc.opc051.data.isWarning = 1;					// ON
	else			opc.opc051.data.isWarning = 0;					// OFF

	for( i = 0; i < opc.opc051.len; i++ ){
		sum += opc.data[OPC_HEADER_SIZE+i];
	}
	sum = ~sum & 0xFF;
	opc.opc051.sum = sum;

	LOG(LOG_OPC_DEBUG,"[OPC]Warning Output Send isWarning:%02X",(int)opc.opc051.data.isWarning);
	serial_send_req(opc);
}


/*-------------------------------------------------------------------
	FUNCTION :	servival_ack_send_req()
	EFFECTS  :	生存確認応答伝文送信要求
	NOTE	 :	生存確認応答伝文送信要求
-------------------------------------------------------------------*/
void servival_ack_send_req(void)
{
	OpcSendDataType opc;
	int i;
	uchar sum = 0;

	memset(&opc,0x00,sizeof(OpcSendDataType));

	opc.opc061.id = OPC_ID_SERVIVAL_ACK;							// 伝文ID(61h)
	opc.opc061.len = sizeof(OPC061::data);							// 伝文データ長

	opc.opc061.data.dummy = 0;										// ダミー

	for( i = 0; i < opc.opc061.len; i++ ){
		sum += opc.data[OPC_HEADER_SIZE+i];
	}
	sum = ~sum & 0xFF;
	opc.opc061.sum = sum;

	LOG(LOG_OPC_DEBUG,"[OPC]Survival Response Send");
	serial_send_req(opc);
}


/*-------------------------------------------------------------------
	FUNCTION :	servival_setting_send_req()
	EFFECTS  :	生存確認設定伝文送信要求
	NOTE	 :	生存確認設定伝文送信要求
-------------------------------------------------------------------*/
void servival_setting_send_req(bool enable)
{
	OpcSendDataType opc;
	int i;
	uchar sum = 0;

	memset(&opc,0x00,sizeof(OpcSendDataType));

	opc.opc070.id = OPC_ID_SURVIVAL_SETTING;							// 伝文ID(61h)
	opc.opc070.len = sizeof(OPC070::data);								// 伝文データ長

	if( enable ){
		opc.opc070.data.enable = 1;										// 有効無効設定
	}
	else{
		opc.opc070.data.enable = 0;										// 有効無効設定
	}

	for( i = 0; i < opc.opc070.len; i++ ){
		sum += opc.data[OPC_HEADER_SIZE+i];
	}
	sum = ~sum & 0xFF;
	opc.opc070.sum = sum;

	LOG(LOG_OPC_DEBUG,"[OPC]Survival Setting Send");
	serial_send_req(opc);
}


/*-------------------------------------------------------------------
	FUNCTION :	getdate_ack_send_req()
	EFFECTS  :	時刻取得応答伝文送信要求
	NOTE	 :
-------------------------------------------------------------------*/
void getdate_ack_send_req(int result ,struct tm *t_st)
{
	OpcSendDataType opc;
	int i;
	uchar sum = 0;

	memset(&opc,0x00,sizeof(OpcSendDataType));

	opc.opc091.id = OPC_ID_GETDATE_ACK;								// 伝文ID(91h)
	opc.opc091.len = sizeof(OPC091::data);							// 伝文データ長

	if( result ){
		if( t_st->tm_year >= 120 ){
			opc.opc091.data.year = t_st->tm_year - 100;				// 2000年以降
			opc.opc091.data.month = t_st->tm_mon + 1;				// 月
			opc.opc091.data.day = t_st->tm_mday;					// 日
			opc.opc091.data.hour = t_st->tm_hour;					// 時
			opc.opc091.data.minite = t_st->tm_min;					// 分
			opc.opc091.data.second = t_st->tm_sec;					// 秒
		}
		else{
			result = false;
		}
	}

	if(result)	opc.opc091.data.result = 1;							// 成功
	else		opc.opc091.data.result = 0;							// 失敗

	for( i = 0; i < opc.opc091.len; i++ ){
		sum += opc.data[OPC_HEADER_SIZE+i];
	}
	sum = ~sum & 0xFF;
	opc.opc091.sum = sum;

	LOG(LOG_OPC_DEBUG,"[OPC]Get Date Response Send ret:%02X",(int)opc.opc091.data.result);
	serial_send_req(opc);
}


/*-------------------------------------------------------------------
	FUNCTION :	version_ack_send_req()
	EFFECTS  :	バージョン確認応答伝文送信要求
	NOTE	 :
-------------------------------------------------------------------*/
void version_ack_send_req(void)
{
	OpcSendDataType opc;
	int i;
	uchar sum = 0;
	struct utsname      uname_buff;
	char kernelver_str[150];
	char kernelver[OPC_KERNEL_VERSION];

	memset(&opc,0x00,sizeof(OpcSendDataType));

	opc.opc093.id = OPC_ID_VERSION_ACK;								// 伝文ID(93h)
	opc.opc093.len = sizeof(OPC093::data);							// 伝文データ長
	strncpy(opc.opc093.data.appverion, g_app_ver.c_str(), OPC_APP_VERSION-1);
	strncpy(opc.opc093.data.dataverion, g_train_model_ver.c_str(), OPC_DATA_VERSION-1);
	if (uname(&uname_buff) == 0) {
		sprintf(kernelver_str,"%s %s",uname_buff.release, uname_buff.version);
		memset(kernelver,0x00, OPC_KERNEL_VERSION);
		memcpy(kernelver, kernelver_str, OPC_KERNEL_VERSION-1);
		strncpy(opc.opc093.data.kernelverion, kernelver, OPC_KERNEL_VERSION);
	}

	for( i = 0; i < opc.opc093.len; i++ ){
		sum += opc.data[OPC_HEADER_SIZE+i];
	}
	sum = ~sum & 0xFF;
	opc.opc093.sum = sum;

	LOG(LOG_OPC_DEBUG,"[OPC]Version Response Send");
	serial_send_req(opc);
}


/*-------------------------------------------------------------------
  FUNCTION :	fpga_version_ack_send_req()
  EFFECTS  :	FPGAバージョン確認応答伝文送信要求
  NOTE	 :
  -------------------------------------------------------------------*/
void fpga_version_ack_send_req(void)
{
	OpcSendDataType opc;
	int i;
	uchar sum = 0;

	memset(&opc,0x00,sizeof(OpcSendDataType));

	opc.opc098.id = OPC_ID_FPGA_VERSION_ACK;									// 伝文ID(93h)
	opc.opc098.len = sizeof(OPC098::data);										// 伝文データ長
	strncpy(opc.opc098.data.fpgaversion, g_fpga_ver.c_str(),OPC_FPGA_VERSION-1);

	for( i = 0; i < opc.opc098.len; i++ ){
		sum += opc.data[OPC_HEADER_SIZE+i];
	}
	sum = ~sum & 0xFF;
	opc.opc098.sum = sum;

	LOG(LOG_OPC_DEBUG,"[OPC]FPGA Version Response Send");
	serial_send_req(opc);
}


/*-------------------------------------------------------------------
	FUNCTION :	sdcard_test_ack_send_req()
	EFFECTS  :	SDカードテスト応答伝文送信要求
	NOTE	 :
-------------------------------------------------------------------*/
void sdcard_test_ack_send_req(int result)
{
	OpcSendDataType opc;
	int i;
	uchar sum = 0;

	memset(&opc,0x00,sizeof(OpcSendDataType));

	opc.opc095.id = OPC_ID_SDCARD_TEST_ACK;							// 伝文ID(95h)
	opc.opc095.len = sizeof(OPC095::data);							// 伝文データ長

	if(result)	opc.opc095.data.result = 1;							// 成功
	else		opc.opc095.data.result = 0;							// 失敗

	for( i = 0; i < opc.opc095.len; i++ ){
		sum += opc.data[OPC_HEADER_SIZE+i];
	}
	sum = ~sum & 0xFF;
	opc.opc095.sum = sum;

	LOG(LOG_OPC_DEBUG,"[OPC]SDCard Test Response Send ret:%02X",(int)opc.opc095.data.result);
	serial_send_req(opc);
}


/*-------------------------------------------------------------------
  FUNCTION :	opc44_exec()
  EFFECTS  :	RTC設定要求
  NOTE     :
-------------------------------------------------------------------*/
void opc44_exec(const OpcDataType &opc)
{
	char command[100];
	uchar days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	uchar lastd;
	int ret;
	int check;

	LOG(LOG_OPC_DEBUG,"[OPC]Rtc Set");
	// 日付判定
	check = 0;
	if( opc.opc044.data.year < 20 )	 check = 1;										// 年範囲ﾁｪｯｸ
	if( opc.opc044.data.month < 1 || opc.opc044.data.month > 12 )	check = 1;		// 月範囲ﾁｪｯｸ
	if( opc.opc044.data.day < 1 || opc.opc044.data.day > 31 )	check = 1;			// 日範囲ﾁｪｯｸ
	if( opc.opc044.data.hour > 23 )	check = 1;										// 時範囲ﾁｪｯｸ
	if( opc.opc044.data.minite > 59 )	check = 1;									// 分範囲ﾁｪｯｸ
	if( opc.opc044.data.second > 59 )	check = 1;									// 秒範囲ﾁｪｯｸ

	if( !check ){
		// 日有効ﾁｪｯｸ
		lastd = days[opc.opc044.data.month-1];
		if(opc.opc044.data.month == 2){
			if( (opc.opc044.data.year % 4 == 0 && opc.opc044.data.year % 100 != 0 ) || opc.opc044.data.year % 400 == 0 ){	// うるう年ﾁｪｯｸ
				lastd = 29;
			}
		}
		if( opc.opc044.data.day > lastd )	check = 1;
	}
	if( check ){																		// 無効日時
		rtc_ack_send_req(false);
		ERR("illeagal date Y:%d M:%d D:%d H:%d M:%d S:%d",(int)opc.opc044.data.year,(int)opc.opc044.data.month,(int)opc.opc044.data.day,(int)opc.opc044.data.hour,(int)opc.opc044.data.minite,(int)opc.opc044.data.second);
		return;
	}
	sprintf(command,"hwclock --set --date \"%d %s %d %d:%d:%d\"", (int)opc.opc044.data.day, GetMonthEnglishText((int)opc.opc044.data.month).substr(0, 3).c_str(), (int)opc.opc044.data.year, (int)opc.opc044.data.hour, (int)opc.opc044.data.minite, (int)opc.opc044.data.second);
	LOG(LOG_OPC_DEBUG,"[OPC]Rtc Set %s",command);
	ret = system(command);
	if( ret == 0 ){
		sprintf(command,"hwclock --hctosys");
		ret = system(command);
		if( ret == 0 ){
			rtc_ack_send_req(true);
		}
		else{
			rtc_ack_send_req(false);
			ERR("hwclock --hctosys Failed");
		}
	}
	else{
		rtc_ack_send_req(false);
		ERR("hwclock --set --date Failed");
	}
}


/*-------------------------------------------------------------------
  FUNCTION :	opc4A_exec()
  EFFECTS  :	エラーリセット
  NOTE     :
  -------------------------------------------------------------------*/
void opc4A_exec(const OpcDataType &opc)
{
	LOG(LOG_OPC_DEBUG,"[OPC]ERROR Reset");
	error_reset(&errorinfo,&errordetail);
	error_info_send_req();
}


/*-------------------------------------------------------------------
  FUNCTION :	opc4B_exec()
  EFFECTS  :	エラー検知通知
  NOTE     :
  -------------------------------------------------------------------*/
void opc4B_exec(const OpcDataType &opc)
{
	LOG(LOG_OPC_DEBUG,"[OPC]ERROR Info err1:%02X err2:%02X err3:%02X err4:%02X",(int)opc.data[4],(int)opc.data[5],(int)opc.data[6],(int)opc.data[7]);
	OUTPUT_04B_ERROR_LOG(err1.impprc_wdt, 		"ImageProcesor WDT");
	OUTPUT_04B_ERROR_LOG(err1.pwvlt_uplimit, 	"Voltage Rise");
	OUTPUT_04B_ERROR_LOG(err1.pwvlt_downlimit, 	"Voitage Drop");
	OUTPUT_04B_ERROR_LOG(err1.serial, 			"Serial I/F");
	OUTPUT_04B_ERROR_LOG(err4.base_cpu, 		"BaseBoard CPU");
	error_info_save(errorinfo);
	error_info_send_req();
	if(errorinfo.err1.impprc_wdt == 3) g_wdt_occerd = true;
}


/*-------------------------------------------------------------------
  FUNCTION :	opc4C_exec()
  EFFECTS  :	エラー詳細通知
  NOTE     :
  -------------------------------------------------------------------*/
void opc4C_exec(const OpcDataType &opc)
{
	LOG(LOG_OPC_DEBUG,"[OPC]ERROR Detail err1:%02X err2:%02X err3:%02X err4:%02X err5:%02X err6:%02X",(int)opc.data[4],(int)opc.data[5],(int)opc.data[6],(int)opc.data[7],(int)opc.data[8],(int)opc.data[9]);
	OUTPUT_04C_ERROR_LOG(err1.ram_chk,			"BaseCPU RAM NG");
	OUTPUT_04C_ERROR_LOG(err1.cpu_chk,			"BaseCPU Calc NG");
	OUTPUT_04C_ERROR_LOG(err1.rom_chksum,		"BaseCPU ROM Check Sum NG");
	OUTPUT_04C_ERROR_LOG(err1.cpu_err,			"BaseCPU CPU Exception Occurred");
	OUTPUT_04C_ERROR_LOG(err2.rst_illegalorder,	"Reset illeagal order");
	OUTPUT_04C_ERROR_LOG(err2.rst_wdt_clock,	"Reset WDT");
	OUTPUT_04C_ERROR_LOG(err2.rst_illegalaccess,"Reset illeagal access");
	OUTPUT_04C_ERROR_LOG(err2.rst_pwr_donw,		"Reset voltage drop");
	OUTPUT_04C_ERROR_LOG(err3.batmon_upper,		"BATMON Rise");
	OUTPUT_04C_ERROR_LOG(err3.mon_24v_upper,	"MON_24V Rise");
	OUTPUT_04C_ERROR_LOG(err3.mon_3r3v_upper,	"MON_3R3V Rise");
	OUTPUT_04C_ERROR_LOG(err3.mon_5v_upper,		"MON_5V Rise");
	OUTPUT_04C_ERROR_LOG(err3.mon_1r8vd_upper,	"MON_1R8VD Rise");
	OUTPUT_04C_ERROR_LOG(err3.mon_3r3va_upper,	"MON_3R3VA Rise");
	OUTPUT_04C_ERROR_LOG(err3.mon_1r8va_upper,	"MON_1R8VA Rise");
	OUTPUT_04C_ERROR_LOG(err3.mon_1r8vap_upper,	"MON_1R8VAP Rise");
	OUTPUT_04C_ERROR_LOG(err4.mon_12va_upper,	"MON_12VA Rise");
	OUTPUT_04C_ERROR_LOG(err5.batmon_lower,		"BATMON Drop");
	OUTPUT_04C_ERROR_LOG(err5.mon_24v_lower,	"MON_24V Drop");
	OUTPUT_04C_ERROR_LOG(err5.mon_3r3v_lower,	"MON_3R3V Drop");
	OUTPUT_04C_ERROR_LOG(err5.mon_5v_lower,		"MON_5V Drop");
	OUTPUT_04C_ERROR_LOG(err5.mon_1r8vd_lower,	"MON_1R8VD Drop");
	OUTPUT_04C_ERROR_LOG(err5.mon_3r3va_lower,	"MON_3R3VA Drop");
	OUTPUT_04C_ERROR_LOG(err5.mon_1r8va_lower,	"MON_1R8VA Drop");
	OUTPUT_04C_ERROR_LOG(err5.mon_1r8vap_lower,	"MON_1R8VAP Drop");
	OUTPUT_04C_ERROR_LOG(err6.mon_12va_lower,	"MON_12VA Drop");
	error_detail_save(errordetail);
}


/*-------------------------------------------------------------------
  FUNCTION :	opc60_exec()
  EFFECTS  :	生存確認要求
  NOTE     :
  -------------------------------------------------------------------*/
void opc60_exec(const OpcDataType &opc)
{
	LOG(LOG_OPC_DEBUG,"[OPC]Survival Request");
	if(g_survival_flg){
		LOG(LOG_COMMON_OUT,"BaseBoard Survival Req return");
	}
	g_survival_cnt = 0;										// 生存確認ﾁｪｯｸ用ｶｳﾝﾄｸﾘｱ
	g_survival_flg = 0;										// 生存確認ﾁｪｯｸﾌﾗｸﾞｸﾘｱ
	servival_ack_send_req();								// 生存確認応答
}


/*-------------------------------------------------------------------
  FUNCTION :	opc90_exec()
  EFFECTS  :	時刻取得要求
  NOTE     :
  -------------------------------------------------------------------*/
void opc90_exec(const OpcDataType &opc)
{
	struct tm *t_st;
	time_t t;

	t_st = NULL;
	t = time(NULL);																// 時刻取得
	{
		const auto [isLowBattery, isOscillationStop] = GetRTCStatus();
		if( isLowBattery || isOscillationStop || g_errorManager.IsError(ErrorCode::GetRTCStatus) ){
			ERR("RTC status NG");
			getdate_ack_send_req(false,NULL);								    // 時刻取得応答送信
		}
		else if(IsCompleteInitRTC() == false){								    // RTC電池を抜いてもtime()が取得できてしまうので、hwclockコマンドにて未設定を確認
			ERR("RTC uninitialized NG");
			getdate_ack_send_req(false,NULL);									// 時刻取得応答送信
		}
		else{
			if( t > 0 ){
				t_st = localtime(&t);											// 時刻構造体へ変換
				getdate_ack_send_req(true,t_st);								// 時刻取得応答送信
			}
			else{
				getdate_ack_send_req(false,NULL);								// 時刻取得応答送信
			}
		}
	}
}


/*-------------------------------------------------------------------
  FUNCTION :	opc92_exec()
  EFFECTS  :	バージョン確認応答送信
  NOTE     :
  -------------------------------------------------------------------*/
void opc92_exec(const OpcDataType &opc)
{
	version_ack_send_req();
}


/*-------------------------------------------------------------------
  FUNCTION :	opc94_exec()
  EFFECTS  :	SDカードテスト要求
  NOTE     :
  -------------------------------------------------------------------*/
void opc94_exec(const OpcDataType &opc)
{
	if(!sdcard_test()){										// SDカードテスト実行
		sdcard_test_ack_send_req(true);						// SDカードテスト応答送信
	}
	else{
		sdcard_test_ack_send_req(false);					// SDカードテスト応答送信
	}
}


/*-------------------------------------------------------------------
  FUNCTION :	opc97_exec()
  EFFECTS  :	FPGAバージョン要求
  NOTE     :
  -------------------------------------------------------------------*/
void opc97_exec(const OpcDataType &opc)
{
	LOG(LOG_OPC_DEBUG,"[OPC]FPGA Version Request");
	fpga_version_ack_send_req();												// FPGAバージョン情報確認応答送信
}


/*-------------------------------------------------------------------
  FUNCTION :	RecvSirialCommand()
  EFFECTS  :	OPCコマンド受信処理
  NOTE	 :
-------------------------------------------------------------------*/
void ExecRL78Command(void)
{
	while(opcdata.size()) {
		OpcDataType opc;
		pthread_mutex_lock(&NoticeMutex);
		opc = opcdata[0];
		opcdata.erase(opcdata.begin());
		pthread_mutex_unlock(&NoticeMutex);

		switch(opc.header.id) {
			case OPC_ID_RTC_SETREQ:				//RTC設定要求 -> setting request
				opc44_exec(opc);
				break;

			case OPC_ID_ERROR_RESET:			// エラーリセット
				opc4A_exec(opc);
				break;

			case OPC_ID_ERROR_DETECT:			// エラー検知通知
				opc4B_exec(opc);
				break;

			case OPC_ID_ERROR_DETAIL:			// エラー詳細通知
				opc4C_exec(opc);
				break;

			case OPC_ID_SURVIVAL_REQ:			// 生存確認要求
				opc60_exec(opc);
				break;

			case OPC_ID_GETDATE_REQ:			// 時刻取得要求
				opc90_exec(opc);
				break;

			case OPC_ID_VERSION_REQ:			// バージョン確認要求
				opc92_exec(opc);
				break;

			case OPC_ID_SDCARD_TEST_REQ:		// SDカードテスト要求
				opc94_exec(opc);
				break;

			case OPC_ID_FPGA_VERSION_REQ:		// FPGAバージョン要求
				opc97_exec(opc);
				break;

		}
	}
}
