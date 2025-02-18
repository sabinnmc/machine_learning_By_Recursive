/********************************************************************************
;*                                                                                *
;*        Copyright (C) 2024 PAL Inc. All Rights Reserved                         *
;*                                                                                *
;********************************************************************************
;*                                                                                *
;*        ﾓｼﾞｭｰﾙ名      :    CanCommand.cpp                                       *
;*                                                                                *
;*        作　成　者    :    内之浦　伸治                                         *
;*                                                                                *
;********************************************************************************/
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <net/if.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include "CanCtrl.h"
#include "Sysdef.h"
#include "Global.h"
#include "LinuxUtility.hpp"

using namespace std;


/*===	SECTION NAME		===*/

/*===	user definition		===*/

/*===	pragma variable	===*/

/*===	external variable	===*/
extern ErrorInfoType errorinfo;									// ｴﾗｰ情報
extern ErrorDetailType errordetail;								// ｴﾗｰ詳細情報

/*===	external function	===*/
extern void MakeMessage(ulong id,void *data, uint len);

/*===	public function prototypes	===*/

/*===	static function prototypes	===*/
bool RecvCanCommand(T_CANFD_FRAME pac);
int UpdateTimer(void);

void CreateSendMessage(void);
void ReadErrorHistory(void);

T_CANFD_FRAME *GetSendBuff(ulong id);
int Recv18D93600(T_CANFD_FRAME *pac);
int Recv18FF6800(T_CANFD_FRAME *pac);
int Recv18FF3200(T_CANFD_FRAME *pac);
int Recv18EAFF00(T_CANFD_FRAME *pac);
int Recv18EAD900(T_CANFD_FRAME *pac);
int Recv18FEE600(T_CANFD_FRAME *pac);
int Recv18EF8000(T_CANFD_FRAME *pac);
int Recv18EFD900(T_CANFD_FRAME *pac);

/*===	static typedef variable		===*/

/*===	static variable		===*/
T_CANFD_FRAME   m_buff_18FF3BD9;                                   // ブザー制御 / アイコン制御
T_CANFD_FRAME   m_buff_18FECAD9;                                   // エラーコード送信
T_CANFD_FRAME   m_buff_18FECBD9;                                   // エラー履歴送信
T_CANFD_FRAME   m_buff_18FF00D9;                                   // コード番号
T_CANFD_FRAME   m_buff_18EFDFD9;                                   // MFDへの応答

/*===	constant variable	===*/

/*===	global variable		===*/

/*-------------------------------------------------------------------
	FUNCTION :	RecvCanCommand()
	EFFECTS  :	canコマンド受信処理
	NOTE	 :
-------------------------------------------------------------------*/
bool RecvCanCommand(T_CANFD_FRAME pac)
{
	bool ret = false;

	switch( pac.can_id & 0x1FFFFF00 ) {
		case 0x18D93600:
			Recv18D93600(&pac);
			ret = true;
			break;

		case 0x18FF6800:
			Recv18FF6800(&pac);
			ret = true;
			break;

		case 0x18FF3200:
			Recv18FF3200(&pac);
			ret = true;
			break;

		case 0x18EAFF00:
			Recv18EAFF00(&pac);
			ret = true;
			break;

		case 0x18EAD900:
			Recv18EAD900(&pac);
			ret = true;
			break;

		case 0x18FEE600:
			Recv18FEE600(&pac);
			ret = true;
			break;

		case 0x18EF8000:
			Recv18EF8000(&pac);
			ret = true;
			break;

		case 0x18EFD900:
			Recv18EFD900(&pac);
			ret = true;
			break;

	}
	return ret;
}


/*-------------------------------------------------------------------
	FUNCTION :	InitCanCommand()
	EFFECTS  :	canコマンド初期化
	NOTE	 :
-------------------------------------------------------------------*/
void InitCanCommand(void)
{
	// 伝文初期化
	memset(&g_CanRecvData, 0, sizeof(T_CAN_RECV_DATA));
}


/*-------------------------------------------------------------------
	FUNCTION :	GetSendBuff()
	EFFECTS  :	送信IDのバッファ有無の確認
	NOTE	 :
-------------------------------------------------------------------*/
T_CANFD_FRAME *GetSendBuff(ulong id)
{
	T_CANFD_FRAME	*result;

	result = NULL;
	switch( id ) {

	case 0x18FF3BD9:
		result = &m_buff_18FF3BD9;
		break;

	case 0x18FECAD9:
		result = &m_buff_18FECAD9;
		break;

	case 0x18FECBD9:
		result = &m_buff_18FECBD9;
		break;

	case 0x18FF00D9:
		result = &m_buff_18FF00D9;
		break;

	case 0x18EFDFD9:
		result = &m_buff_18EFDFD9;
		break;

	default:
		result = NULL;
		break;
	}

	return result;
}


/*-------------------------------------------------------------------
	FUNCTION :	Recv18D93600()
	EFFECTS  :	opc18D93600受信処理
	NOTE	 :	車両情報
-------------------------------------------------------------------*/
int Recv18D93600(T_CANFD_FRAME *pac)
{
	int	result;

	result = 0;

	if( pac->len != 8 ) {														// 伝文長異常
		result = -1;
	}
	else {																		// 伝文長が正常であれば
		g_CanRecvData.vehicle_speed = pac->data[0] & 0x01 ? 1 : 0;				// 車速情報（1:25km以下 & PTO OFF）
		g_CanRecvData.stop = pac->data[0] & 0x02 ? 1 : 0;						// 停車情報（1:1km/h未満 & ブレーキON）
		g_CanRecvData.left = pac->data[0] & 0x04 ? 1 : 0;						// 左折情報（1:操作あり）
	}
	return 	result;
}


/*-------------------------------------------------------------------
	FUNCTION :	Recv18FF6800()
	EFFECTS  :	opc18FF6800受信処理
	NOTE	 :	MFD指示
-------------------------------------------------------------------*/
int Recv18FF6800(T_CANFD_FRAME *pac)
{
	int	result;

	result = 0;

	if( pac->len != 8 ) {														// 伝文長異常
		result = -1;
	}
	else {																		// 伝文長が正常であれば
		g_CanRecvData.buzzor = pac->data[0] & 0x02 ? 1 : 0;						// ブザー出力制御（1:ブザー無効）
		g_CanRecvData.adjust = pac->data[1] & 0x01 ? 1 : 0;						// 調整モード遷移（1:調整モードON）

		g_CanRecvData.mfd_button_state_recv = 1;								// ID受信完了状態とする（初受信するまで黒画像）
		g_CanRecvData.mfd_favorite0 = (pac->data[2] >> 0) & 0x0F;				// MFDのお気に入り選択状態（PTO OFF）
		g_CanRecvData.mfd_favorite1 = (pac->data[2] >> 4) & 0x0F;				// MFDのお気に入り選択状態（PTO ON）
		g_CanRecvData.mfd_change_range = (pac->data[4] >> 7) & 0x01;			// MFDのお気に入り変更のレンジ選択状態
		g_CanRecvData.mfd_change_pto = (pac->data[4] >> 6) & 0x01;				// MFDのお気に入り変更のPTO選択状態
		g_CanRecvData.mfd_change_favorite = (pac->data[4] >> 0) & 0x0F;			// MFDのお気に入り変更番号
		if ((pac->data[5] >> 0) & 0x01){										// MFDのお気に入り変更要求 映像1
			g_MfdButtonCtrl.monitor1.store(true);
		}
		if ((pac->data[5] >> 1) & 0x01){										// MFDのお気に入り変更要求 映像1
			g_MfdButtonCtrl.monitor2.store(true);
		}
	}

	return 	result;
}


/*-------------------------------------------------------------------
	FUNCTION :	Recv18FF3200()
	EFFECTS  :	opc18FF3200受信処理
	NOTE	 :	PTO情報
-------------------------------------------------------------------*/
int Recv18FF3200(T_CANFD_FRAME *pac)
{
	int	result;

	result = 0;

	if( pac->len != 8 ) {														// 伝文長異常
		result = -1;
	}
	else {																		// 伝文長が正常であれば
		g_CanRecvData.pto = pac->data[7] & 0x10 ? 1: 0;							// PTO情報（1:作業中）
	}

	return 	result;
}


/*-------------------------------------------------------------------
	FUNCTION :	Recv18EAFF00()
	EFFECTS  :	opc18EAFF00受信処理
	NOTE	 :	エラー履歴送信要求
-------------------------------------------------------------------*/
int Recv18EAFF00(T_CANFD_FRAME *pac)
{
	int	result;

	result = 0;

	if( pac->len != 8 ) {														// 伝文長異常
		result = -1;
	}
	else {																		// 伝文長が正常であれば
		ReadErrorHistory();
		// TODO: send error message
	}
	return 	result;
}


/*-------------------------------------------------------------------
	FUNCTION :	Recv18EAD900()
	EFFECTS  :	opc18EAD900受信処理
	NOTE	 :	エラー履歴消去要求
-------------------------------------------------------------------*/
int Recv18EAD900(T_CANFD_FRAME *pac)
{
	int	result;

	result = 0;

	if( pac->len != 8 ) {														// 伝文長異常
		result = -1;
	}
	else {																		// 伝文長が正常であれば
	// TODO delete_history();
	}

	return 	result;
}


/*-------------------------------------------------------------------
	FUNCTION :	Recv18FEE600()
	EFFECTS  :	opc18FEE600受信処理
	NOTE	 :	日時情報
-------------------------------------------------------------------*/
int Recv18FEE600(T_CANFD_FRAME *pac)
{
	int	result;
	int year;
	result = 0;

	if( pac->len != 8 ) {														// 伝文長異常
		result = -1;
	}
	else {																		// 伝文長が正常であれば
		// エラー情報保存用
		g_RTC.sec.hex   = (int)(pac->data[0]/4);								// 秒
		g_RTC.min.hex   = pac->data[1];											// 分
		g_RTC.hour.hex  = pac->data[2];											// 時
		g_RTC.month.hex = pac->data[3];											// 月
		g_RTC.day.hex   = (int)(pac->data[4]/4);								// 日
		year		    = 1985 + pac->data[5];									// 西暦
		g_RTC.year.hex  = year - (year / 100) * 100;							// 年（下2桁）
		g_RTC.yearh     = year / 100;											// 年（上2桁）
	}
	return 	result;
}


/*-------------------------------------------------------------------
	FUNCTION :	Recv18EF8000()
	EFFECTS  :	opc18EF8000受信処理
	NOTE	 :	LUT変更情報
-------------------------------------------------------------------*/
int Recv18EF8000(T_CANFD_FRAME *pac)
{
	int	result;

	result = 0;

	if( pac->len != 8 ) {														// 伝文長異常
		result = -1;
	}
	else {																		// 伝文長が正常であれば
		g_CanRecvData.lut1 = pac->data[0];										// モニタ１側のLUT更新
		g_CanRecvData.lut2 = pac->data[1];										// モニタ２側のLUT更新
	}

	return 	result;
}


/*-------------------------------------------------------------------
  FUNCTION :	Recv18EFD900()
  EFFECTS  :	opc18EFD900受信処理
  NOTE	   :	MFD設定変更通知
  -------------------------------------------------------------------*/
int Recv18EFD900(T_CANFD_FRAME *pac)
{
	int	result;

	result = 0;

	if( pac->len != 8 ) {														// 伝文長異常
		result = -1;
	}
	else {																		// 伝文長が正常であれば
		if (pac->data[0] == 0x20) {												// お気に入りの設定変更通知で
			if (pac->data[1] == kMfdReqConfirm){								// 確定ボタン押下通知であれば
				g_MfdButtonCtrl.save_req.store(true);							// ボタン状態を更新
			}
			if (pac->data[1] == kMfdReqCancel){									// キャンセルボタン押下通知であれば
				g_MfdButtonCtrl.cancel_req.store(true);							// ボタン状態を更新
			}
		}
	}

	return 	result;
}


/*-------------------------------------------------------------------
	FUNCTION :	Send18FF3BD9()
	EFFECTS  :	opc18FF3BD9送信処理
	NOTE	 :	DO信号
-------------------------------------------------------------------*/
void Send18FF3BD9(void)
{
	static int state1 = 0;
	static int interval1 = 0;
	static int level = 0;
	uchar buff[8];
	T_CAN_DO out0;
	int datastate;
	memset(buff, 0x00, sizeof(buff));
	out0.data.byte = 0;

	datastate = g_DetectState;

	if((errordetail.err7.bof_ch0 == 0) && (errordetail.err7.psv_ch0 == 0)){
		if((g_DetectState & YOLO_DETECTION_STOP) == 0) {						// 検出が有効状態であれば
			switch (state1) {
				case 0:
					if(datastate & YOLO_PERSON_DANGER_JUDGE)		out0.data.bit.do6 = 1;
					else if(datastate & YOLO_PERSON_DETECT_JUDGE)	out0.data.bit.do5 = 1;
					else											out0.data.bit.do4 = 1;
					if (datastate != 0x00) {
						if (g_CanRecvData.buzzor == 0x00)			out0.data.bit.do0 = 1;
						level = datastate;
						interval1 = 0;
						state1 = 1;
					}
					break;

				case 1:															// 鳴動ON中
					if(g_CanRecvData.buzzor == 0x00 ) out0.data.bit.do0 = 1;	// ﾌﾞｻﾞｰが有効であれば 出力
					if(level & YOLO_PERSON_DETECT_JUDGE) {						// 検知状態：注意の場合
						out0.data.bit.do5 = 1;									// 検知状態：注意
						if (++interval1 >= 1) interval1=0, state1=2;			// 鳴動時間経過の場合、次のｽﾃｰﾄへ
					}
					else if(level & YOLO_PERSON_DANGER_JUDGE) {					// 検知状態：警報の場合
						out0.data.bit.do6 = 1;									// 検知状態：警報
						if (++interval1 >= 5) interval1=0, state1=2;			// 鳴動時間経過の場合、次のｽﾃｰﾄへ
					}
					break;

				case 2:															// 鳴動OFF中
					out0.data.bit.do0 = 0;										// ﾌﾞｻﾞｰ出力OFF
					if(level & YOLO_PERSON_DETECT_JUDGE) {						// 検知状態：注意の場合
						out0.data.bit.do5 = 1;									// 検知状態：注意
						if (++interval1 >= 30*(g_CanRecvData.stop+1)) {			// 鳴動停止時間経過の場合、次のｽﾃｰﾄへ
							interval1=0;										// 休止間隔ｸﾘｱ
							state1=0;											// ｽﾃｰﾄを戻す
						}
					}
					else if(level & YOLO_PERSON_DANGER_JUDGE) {					// 検知状態：警報の場合
						out0.data.bit.do6 = 1;									// 検知状態：警報
						if (++interval1 >= 20*(g_CanRecvData.stop+1)) {			// 鳴動停止時間経過の場合、次のｽﾃｰﾄへ
							interval1=0;										// 休止間隔ｸﾘｱ
							state1=0;											// ｽﾃｰﾄを戻す
						}
					}
					break;

				default:														// 不明のｽﾃｰﾄの場合
					interval1=0;												// 初期化
					state1=0;													// 初期化
					break;
			}

			if(datastate & YOLO_PERSON_DANGER_JUDGE)		out0.data.bit.do3 = 1;
			else if(datastate & YOLO_PERSON_DETECT_JUDGE)	out0.data.bit.do2 = 1;
			else											out0.data.bit.do1 = 1;

			buff[0] = out0.data.byte;
		}
		MakeMessage(0x18FF3BD9, buff, 8);
	}
}


/*-------------------------------------------------------------------
	FUNCTION :	Send18FECAD9()
	EFFECTS  :	opc18FECAD9送信処理
	NOTE	 :	エラーコード送信
-------------------------------------------------------------------*/
void Send18FECAD9(void)
{
	uchar	errcode[kErrCodeMax] = {
		ERCD_DECODER,	ERCD_ENCODER, 	ERCD_ALTERA,	ERCD_RESET,		ERCD_WDT,
		ERCD_DDRMEM,	ERCD_NOSYNC,	ERCD_BUSSOFF,	ERCD_PASSIVE,	ERCD_MFD,
		ERCD_VCU
	};
	uchar		buff[8];

	if (errordetail.err7.bof_ch0 == 0) {
		memset(buff, 0x00, sizeof(buff));

		//E203
		if(errordetail.err8.vcu_disp == 1) buff[0] = errcode[kErrCodeCANLostVCU];
		//E202
		if(errordetail.err8.mfd_disp == 1) buff[0] = errcode[kErrCodeCANLostMFD];
		//E201
		if(errordetail.err7.psv_ch0 == 1) buff[0] = errcode[kErrCodeCANPassive];
		//E200
		if(errordetail.err7.bof_ch0 == 1) buff[0] = errcode[kErrCodeCANBussoff];
		//E100
		if(errordetail.err8.cap_stat == 1) buff[0] = errcode[kErrCodeVideoLost];
		//E007
		if(g_wdt_occerd)                   buff[0] = errcode[kErrCodeWDT];

		MakeMessage(0x18FECAD9, buff, 8);
	}
}


/*-------------------------------------------------------------------
	FUNCTION :	Send18FECBD9()
	EFFECTS  :	opc18FECBD9送信処理
	NOTE	 :
-------------------------------------------------------------------*/
void Send18FECBD9(void)
{
	uchar		buff[8];

	memset(buff, 0x00, sizeof(buff));
	// TODO: create error message
	MakeMessage(0x18FECBD9, buff, 8);

}

/*-------------------------------------------------------------------
	FUNCTION :	Send18FF00D9()
	EFFECTS  :	opc18FF00D9送信処理
	NOTE	 :	コード番号送信
				2sec周期で呼ばれる前提
-------------------------------------------------------------------*/
void Send18FF00D9(void)
{
	static int timer_1min = 0;
	int			ret;
	uchar		data[8];
	char		c[16];


	if (timer_1min < 30) {				// 起動後1分（2x30=60sec）だけ送信する
		timer_1min++;

		ret = sscanf(TDN_TYPE, "%c%c", &c[0], &c[1]);								//2ﾊﾞｲﾄに対応
		if ('0' <= c[0] && c[0] <= '9') c[0] = c[0] - '0';							//数字
		if ('A' <= c[0] && c[0] <= 'F') c[0] = c[0] - 'A' + 0x0A;					//文字
		if ('0' <= c[1] && c[1] <= '9') c[1] = c[1] - '0';							//数字
		if ('A' <= c[1] && c[1] <= 'F') c[1] = c[1] - 'A' + 0x0A;					//文字
		if( ret == 1 )	data[7] = (c[0]);											//
		if( ret == 2 )	data[7] = (c[0]) << 4 | (c[1]);								//
																					//
		memset(c, 0, sizeof(c));													//
		ret = sscanf(TDN_REV, "%c%c", &c[0], &c[1]);								//2ﾊﾞｲﾄに対応
		if( ret == 1 )	data[6] = (c[0]-'0');										//
		if( ret == 2 )	data[6] = (c[0]-'0') << 4 | (c[1]-'0');						//
																					//
		memset(c, 0, sizeof(c));													//
		sscanf(TDN_VER, "%c%c%c-%c%c%c-%c%c%c%c%c", &c[0], &c[1], &c[2], &c[3],
									&c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10]);
		data[5] = (c[0]-'0') << 4 | (c[1]-'0');
		data[4] = (c[2]-'0') << 4 | (c[3]-'0');
		data[3] = (c[4]-'0') << 4 | (c[5]-'0');
		data[2] = (c[6]-'0') << 4 | (c[7]-'0');
		data[1] = (c[8]-'0') << 4 | (c[9]-'0');
		data[0] = (c[10]-'0') << 4;

		MakeMessage(0x18FF00D9, data, 8);
	}
}


/*-------------------------------------------------------------------
	FUNCTION :	Send18EFDFD9()
	EFFECTS  :	opc18EFDFD9送信処理
	NOTE	 :	18EFD900のAck
-------------------------------------------------------------------*/
void Send18EFDFD9(void)
{
	uchar		buff[8];

	memset(buff, 0xFF, sizeof(buff));
	buff[0] = 0x21;																// CAM設定変更応答を示すIndex
	buff[1] = 0x00;																// 問題あれば0x01を応答するが現状ナシ
	MakeMessage(0x18EFDFD9, buff, 8);
}


/*-------------------------------------------------------------------
	FUNCTION :	UpdateTimer()
	EFFECTS  :
	NOTE	 :
-------------------------------------------------------------------*/
int UpdateTimer(void){
	static double timer100 = 0.;                                                // 経過時間
	static double timer1000 = 0.;                                               // 経過時間
	static double timer2000 = 0.;                                               // 経過時間
	static double oldTime100 = GetTimeMsec();                                   // 前回呼び出された時刻
	const double nowTime100 = GetTimeMsec();                                    // 現在時刻
	static double oldTime1000 = GetTimeMsec();                                  // 前回呼び出された時刻
	const double nowTime1000 = GetTimeMsec();                                   // 現在時刻
	static double oldTime2000 = GetTimeMsec();                                  // 前回呼び出された時刻
	const double nowTime2000 = GetTimeMsec();                                   // 現在時刻
	int result = 0;

	timer100 = nowTime100 - oldTime100;											// 経過時間を計測
	timer1000 = nowTime1000 - oldTime1000;										// 経過時間を計測
	timer2000 = nowTime2000 - oldTime2000;										// 経過時間を計測

	if(timer100 >= TIMEOUT_CAN100) {											// 周期時間を迎えた場合は更新
		timer100 = 0.;
		result |= TIMER100;

		oldTime100 = nowTime100;												// 時刻更新
	}

	if(timer1000 >= TIMEOUT_CAN1000){											// 周期時間を迎えた場合は更新
		timer1000 = 0.;
		result |= TIMER1000;

		oldTime1000 = nowTime1000;												// 時刻更新
	}

	if(timer2000 >= TIMEOUT_CAN2000) {											// 周期時間を迎えた場合は更新
		timer2000 = 0.;
		result |= TIMER2000;

		oldTime2000 = nowTime2000;												// 時刻更新
	}

	return result;
}


/*-------------------------------------------------------------------
	FUNCTION :	CreateSendMessage()
	EFFECTS  :
	NOTE	 :
-------------------------------------------------------------------*/
void CreateSendMessage(void){

	int timeup;

	timeup = UpdateTimer();
	if (timeup & TIMER100) {													// 100ms
		Send18FF3BD9();															// ブザー、アイコン制御
	}

	if (timeup & TIMER1000) {													// 1000ms
		Send18FECAD9();															// エラーコード送信
	}

	if (timeup & TIMER2000) {													// 2000ms
		Send18FF00D9();															// Version
	}

	if (g_MfdButtonCtrl.ack_req.exchange(false))  {								// ACK応答要求
		Send18EFDFD9();
	}
}


/*-------------------------------------------------------------------
	FUNCTION :	ReadErrorHistory()
	EFFECTS  :
	NOTE	 :
-------------------------------------------------------------------*/
void ReadErrorHistory(void)
{
	// TODO: /home/share/logにCAN用のログをバイナリで保存する仕組みの実装が必要
}
