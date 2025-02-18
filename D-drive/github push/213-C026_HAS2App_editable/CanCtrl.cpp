/********************************************************************************
;*                                                                                *
;*        Copyright (C) 2024 PAL Inc. All Rights Reserved                         *
;*                                                                                *
;********************************************************************************
;*                                                                                *
;*        開発製品名    :    HAS2アプリケーション                                 *
;*                                                                                *
;*        ﾓｼﾞｭｰﾙ名      :    CanCtrl.cpp                                          *
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
extern ErrorDetailType errordetail;												// ｴﾗｰ詳細情報

/*===	external function	===*/
extern T_CANFD_FRAME *GetSendBuff(ulong id);
extern void InitCanCommand(void);
extern bool RecvCanCommand(T_CANFD_FRAME pac);
extern void CreateSendMessage(void);

/*===	public function prototypes	===*/

/*===	static function prototypes	===*/
bool PushSendData(T_CANFD_FRAME *p);
void AnalyseCANRecvMessage(void);
void *CANThreadFunc(void *targ);
void CloseCAN(int fd_epoll);
bool ConnetCAN(int fd_epoll);
void QueuingRecvMessage(int i,T_CANFD_INFO *obj);
void MakeMessage(ulong id,void *data, uint len);
ulong EncodePgn(uint edp, uint dp, uint pf, uint ps);
void DecodePgn(ulong pgn, uint *edp, uint *dp, uint *pf, uint *ps);
ulong EncodeArb(uint pri, uint edp, uint dp, uint pf, uint ps, uint sa);
int UpdateMFDTimer(int flg);
int UpdateVCUTimer(int flg);
bool GetCANState(void);

/*===	static typedef variable		===*/

/*===	static variable		===*/
pthread_t               m_can_thread;
int                     m_end_can_thread;

T_CANFD_FRAME           m_can_frame;
T_CANFD_INFO            m_can_fd_info;
TCanSendQueue           m_send_data_que;
TCanRecvQueue           m_recv_data_que;
msghdr                  m_message_handler;
iovec                   m_iovec;                         // readv() に対する構造体(入力)
pthread_mutex_t         m_pcan_thread_mutex;             // CANスレッドのミューテックス

/*===	constant variable	===*/

/*===	global variable		===*/

/*-------------------------------------------------------------------
	FUNCTION :	LinkupCAN()
	EFFECTS  :	CAN通信リンクアップ
	NOTE	 :
-------------------------------------------------------------------*/
int LinkupCAN(int ch)
{
	char command[100];
	int ret = -1;

	// CAN停止
	sprintf(command,"ip link set can%d down",ch);
	ret = system(command);
	if( ret != 0 ){
		return ret;
	}

	// ビットレート：250kbps　サンプルポイント0.875%
	sprintf(command,"ip link set can%d type can bitrate %d sample-point 0.875", ch, 250000);
	ret = system(command);
	if( ret != 0 ){
		return ret;
	}

	// 自動バスオフリカバリ設定
	sprintf(command,"ip link set can%d type can restart-ms 500", ch);
	ret = system(command);
	if( ret != 0 ){
		return ret;
	}

	// CAN起動
	sprintf(command,"ip link set can%d up",ch);
	ret = system(command);
	if( ret != 0 ){
		return ret;
	}

	return ret;
}


/*-------------------------------------------------------------------
	FUNCTION :	HookStopSignal()
	EFFECTS  :	特定のハンドル発生時の動作処理
	NOTE	 :
-------------------------------------------------------------------*/
static void HookStopSignal(int signo)
{
	m_end_can_thread = 0;
}


/*-------------------------------------------------------------------
	FUNCTION :	OutputCANMessage()
	EFFECTS  :	CAN通信伝文の出力処理
	NOTE	 :
-------------------------------------------------------------------*/
bool OutputCANMessage(void)
{
	T_CANFD_FRAME sframe;

	while(m_send_data_que.wp != m_send_data_que.rp){							// 送信リードポインタとライトポインタが違う場合
		sframe = m_send_data_que.buff[m_send_data_que.rp];						// 伝文取得

		if (m_send_data_que.rp+1 < MAX_TXPACKET) ++m_send_data_que.rp;			// ライトポインタ更新
		else									   m_send_data_que.rp = 0;

		if (write(m_can_fd_info.s, &sframe, CAN_MTU) != CAN_MTU) {				// 送信
			return false;
		}
	}

	return true;
}


/*-------------------------------------------------------------------
	FUNCTION :	QueuingSendMessage()
	EFFECTS  :	CAN通信伝文送信処理
	NOTE	 :
-------------------------------------------------------------------*/
bool QueuingSendMessage(ulong id, void *data, uint len, uint tmout)
{
	uchar   		broadcast = 0;
	uint			edp;
	uint			dp;
	uint			pf;
	uint			ps;
	uchar   		*pdata;
	int 			paccnt;
	int 			i;
	uint			remain;
	int 			copysize;
	TCANFD_ARB  	*arb;
	T_CANFD_FRAME   *p;
	p = GetSendBuff(id);

	arb = 0;
	if( p == NULL ) {															//当該ID送信対象外または書込み禁止中
		return false;
	}

	memset(p, 0, sizeof(T_CANFD_FRAME));																									//
	if (len <= 8) {																//1ﾊﾟｹｯﾄで送信できる場合
		p[0].can_id = (id | CAN_EFF_FLAG);
		p[0].len = len;															//DLC
		memset(p[0].data, 0xFF, sizeof(p[0].data));
		memcpy(p[0].data, data, len);
		PushSendData(&p[0]);													//送信ﾊﾞｯﾌｧにﾃﾞｰﾀ格納
	}
	else {
		DecodePgn(arb->pgn, &edp, &dp, &pf, &ps);								//一旦、PGNを分解
		if( pf < 239 ) {														//PDU Formatが239未満(宛先指定)
			if( ps == DA_GLOBAL ) broadcast = 1;								//宛先ｱﾄﾞﾚｽ=Global→ﾌﾞﾛｰﾄﾞｷｬｽﾄ
		}
		else if( pf == 239 ) {													//PDU Formatが239(宛先指定、PS=ﾒｰｶｰ固有)
			if( ps == DA_GLOBAL ) broadcast = 1;								//宛先ｱﾄﾞﾚｽ=Global→ﾌﾞﾛｰﾄﾞｷｬｽﾄ
		}
		else if( 240 <= pf && pf < 255 ) {										//PDU Formatが240以上(宛先指定なし、PS=ｸﾞﾙｰﾌﾟ拡張)
			broadcast = 1;														//→ﾌﾞﾛｰﾄﾞｷｬｽﾄ
		}
		else {																	//PDU Formatが255(宛先指定なし、PS=ﾒｰｶｰ固有)
			broadcast = 1;														//→ﾌﾞﾛｰﾄﾞｷｬｽﾄ
		}
		if( broadcast ) {														//宛先はｸﾞﾛｰﾊﾞﾙ?
//			idx = 0;
			//ﾌﾞﾛｰﾄﾞｷｬｽﾄ
			//BAMﾊﾟｹｯﾄ生成
			//優先順位=7(TP.CM default=7)
			//edp=0, dp=0, pf=236(0xEC), ps=0xFF
			p[0].can_id = EncodeArb(7, 0, 0, (uint)PF_TP_BAM, (uint)DA_GLOBAL, (uint)arb->sa);
			p[0].can_id = (p[0].can_id | CAN_EFF_FLAG);
			p[0].len = 8;
			p[0].data[0] = 32;													//TP.BAM Control Byte
			p[0].data[1] = (len & 0x00FF);										//TP.BAM Total Message Size(bytes) L
			p[0].data[2] = (len & 0xFF00) >> 8;									//TP.BAM Total Message Size(bytes) H
			paccnt = len / (sizeof(p[0].data)-1);								//7ﾊﾞｲﾄで構成できるﾊﾟｹｯﾄ数を算出
			paccnt += (len % (sizeof(p[0].data)-1)) ? 1 : 0;					//端数ぶんを1ﾊﾟｹｯﾄ加算
			p[0].data[3] = paccnt;												//TP.BAM Total Number of Packets
			p[0].data[4] = 0xFF;												//TP.BAM fill 0xFF
			p[0].data[5] = (arb->pgn & 0x000000FF);								//TP.BAM PGN L
			p[0].data[6] = (arb->pgn & 0x0000FF00) >> 8;						//TP.BAM PGN M
			p[0].data[7] = (arb->pgn & 0x00FF0000) >> 16;						//TP.BAM PGN H
			//送信ﾊﾞｯﾌｧにBAM格納
			PushSendData(&p[0]);
			remain = len;
			pdata = (uchar *)data;
			for( i = 0; i < paccnt; i++ ) {
				memset(&p[i+1], 0, sizeof(T_CANFD_FRAME));
				//優先順位=7(TP.DT default=7)
				//edp=0, dp=0, pf=235(0xEB), ps
				p[i+1].can_id = EncodeArb(7, 0, 0, PF_TP_DT, (uint)DA_GLOBAL, arb->sa);
				p[i+1].can_id = (p[i+1].can_id | CAN_EFF_FLAG);
				memset(p[i+1].data, 0xFF, sizeof(p[i+1].data));
				p[i+1].data[0] = i+1;											//分割のｼｰｹﾝｽNo
				copysize = (sizeof(p[i+1].data) <= remain) ? sizeof(p[i+1].data)-1 : remain;
				p[i+1].len = 8;
				memcpy(&p[i+1].data[1], pdata, copysize);
				pdata += (copysize);
				remain -= (copysize);
				//送信ﾊﾞｯﾌｧにBAM格納
				PushSendData(&p[i+1]);
			}
		}
	}

	return true;
}


/*-------------------------------------------------------------------
	FUNCTION :	PopRecvMessage()
	EFFECTS  :
	NOTE	 :
-------------------------------------------------------------------*/
bool PopRecvMessage(T_CANFD_FRAME *pac)
{
	uint	wp;
    int mfdtimer,vcntimer;
	bool result = false;

	// MFD
	if ((pac->can_id & 0xFFU) == CAN_MFD_FLAG) {
    	mfdtimer = UpdateMFDTimer(0);
		errordetail.err8.mfd_disp = 0;
	}
	else {
		mfdtimer = UpdateMFDTimer(1);
    	if(mfdtimer & TIMER1000) errordetail.err8.mfd_disp = 1;
	}

	// VCU
	if ((pac->can_id & 0xFFU) == CAN_VCU_FLAG) {
    	vcntimer = UpdateVCUTimer(0);
		errordetail.err8.vcu_disp = 0;
	}
	else {
		vcntimer = UpdateVCUTimer(1);
    	if(vcntimer & TIMER1000) errordetail.err8.vcu_disp = 1;
	}

	wp = m_recv_data_que.wp;														//wpは割り込み内のPushRecvData()で更新される可能性があるので扱い注意！
	if (wp != m_recv_data_que.rp) {												//
		memcpy(pac, &m_recv_data_que.buff[m_recv_data_que.rp], sizeof(T_CANFD_FRAME));
		memset(&m_recv_data_que.buff[m_recv_data_que.rp], 0, sizeof(T_CANFD_FRAME));
		m_recv_data_que.rp++;
		m_recv_data_que.rp = (m_recv_data_que.rp)%MAX_RXPACKET;
		result = true;
	}

	return result;
}


/*-------------------------------------------------------------------
	FUNCTION :	MainLoopForCAN()
	EFFECTS  :	CAN送受信処理
	NOTE	 :
-------------------------------------------------------------------*/
int MainLoopForCAN(void)
{
	int fd_epoll;																// epoll_create()戻り値
	struct epoll_event events_pending;
	struct sockaddr_can addr;													// bind用構造体

	char ctrlmsg[CMSG_SPACE(sizeof(struct timeval) + 3 * sizeof(struct timespec) + sizeof(__u32))];
	int i;
	int num_events;

	int timeout_ms = 0;															// タイムアウト設定(「-1」の場合はタイムアウトなし)

	memset(&m_send_data_que, 0, sizeof(m_send_data_que));						// 送信バッファ初期化
	memset(&m_recv_data_que, 0, sizeof(m_recv_data_que));						// 受信バッファ初期化

	signal(SIGTERM, HookStopSignal);											// シグナルハンドラ(終了シグナル)
	signal(SIGHUP, HookStopSignal);												// シグナルハンドラ(ハングアップ)
	signal(SIGINT, HookStopSignal);												// シグナルハンドラ(キーボードからの割り込みシグナルCTRL+C)

	fd_epoll = epoll_create(1);													// epoll生成
																				// epollは複数のファイルディスクリプタを監視しその中のいずれかが入出力可能な状態であるかを確認する
	if (fd_epoll < 0) {															// エラーチェック
		LOG(LOG_COMMON_OUT,"epoll_create err");
		return -1;
	}

	if (ConnetCAN(fd_epoll) == false) {											// ソケット生成失敗
		CloseCAN(fd_epoll);
	}
	num_events = epoll_wait(fd_epoll, &events_pending, CAN_MAXSOCK, timeout_ms);

	// these settings are static and can be held out of the hot path
	m_iovec.iov_base = &m_can_frame;
	m_message_handler.msg_name = &addr;
	m_message_handler.msg_iov = &m_iovec;
	m_message_handler.msg_iovlen = 1;
	m_message_handler.msg_control = &ctrlmsg;

	while (!m_end_can_thread) {
		usleep(1000);
		// シグナルが発生するまで待機
		// 準備ができているファイルディスクリプターの数を返す
		if ((num_events = epoll_wait(fd_epoll, &events_pending, CAN_MAXSOCK, timeout_ms)) < 0) {
			LOG(LOG_COMMON_OUT,"epoll_wait err");
			m_end_can_thread = 0;
			continue;
		}

		for (i = 0; i < num_events; i++) {										// 発生イベントチェック
			T_CANFD_INFO *obj = (T_CANFD_INFO *)events_pending.data.ptr;
			QueuingRecvMessage(i,obj);											// CAN受信
		}
		AnalyseCANRecvMessage();												// 伝文解析
		CreateSendMessage();													// 送信伝文作成
		OutputCANMessage();														// CAN送信
	}
	CloseCAN(fd_epoll);

	return 0;
}


// /*-------------------------------------------------------------------
// 	FUNCTION :	CloseCAN()
// 	EFFECTS  :
// 	NOTE	 :
// -------------------------------------------------------------------*/
void CloseCAN(int fd_epoll)
{
	close(m_can_fd_info.s);
	close(fd_epoll);
}


/*-------------------------------------------------------------------
	FUNCTION :	ConnetCAN()
	EFFECTS  :
	NOTE	 :
-------------------------------------------------------------------*/
bool ConnetCAN(int fd_epoll)
{
	struct epoll_event event_setup = {
		.events = EPOLLIN, /* prepare the common part */
	};

	struct sockaddr_can addr;                                                   // bind用構造体
	char canif[10]=CAN_IFNAME_CH;
	struct ifreq ifr;                                                           // ソケットインターフェイス
	int ret;
	can_err_mask_t err_mask = CAN_ERR_MASK;
	m_can_fd_info.s = socket(PF_CAN, SOCK_RAW, CAN_RAW);						// ソケット生成
	if (m_can_fd_info.s < 0) {
		LOG(LOG_COMMON_OUT,"socket err");
		return false;
	}
	event_setup.data.ptr = &m_can_fd_info;										// remember the instance as private data
	if (epoll_ctl(fd_epoll, EPOLL_CTL_ADD, m_can_fd_info.s, &event_setup)) {
		LOG(LOG_COMMON_OUT,"failed to add socket to epoll");
		return false;
	}

	m_can_fd_info.ifname = &canif[0];											// can I/F name of this socket 	ソケット名割り振り
	memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
	strcpy(ifr.ifr_name, &canif[0]);
	ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);                             // ネットワークインターフェースのインデックス番号

	if (!ifr.ifr_ifindex) {                                                     // インデックス番号が取得できなかった場合
		LOG(LOG_COMMON_OUT,"if_nametoindex ERROR");
		return false;
	}
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	// can0/1のエラー設定
	setsockopt(m_can_fd_info.s, SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
				&err_mask, sizeof(err_mask));
	ret = bind(m_can_fd_info.s, (struct sockaddr *)&addr, sizeof(addr));

	if (ret < 0) {
		LOG(LOG_COMMON_OUT,"bind error %d %s",errno,strerror(errno));
		return false;
	}
	return true;
}


/*-------------------------------------------------------------------
	FUNCTION :	QueuingRecvMessage()
	EFFECTS  :
	NOTE	 :
-------------------------------------------------------------------*/
void QueuingRecvMessage(int i,T_CANFD_INFO *obj){

	int nbytes;
	struct cmsghdr *cmsg;
	struct sockaddr_can addr;													// bind用構造体

	char ctrlmsg[CMSG_SPACE(sizeof(struct timeval) + 3 * sizeof(struct timespec) + sizeof(__u32))];
	m_iovec.iov_len = sizeof(m_can_frame);
	m_message_handler.msg_namelen = sizeof(addr);
	m_message_handler.msg_controllen = sizeof(ctrlmsg);
	m_message_handler.msg_flags = 0;											//
	nbytes = recvmsg(obj->s, &m_message_handler, 0);							// ソケットからメッセージ取得
	if ((size_t)nbytes == CAN_MTU){												// CANインターフェイス確認
		//	maxdlen = CAN_MAX_DLEN;
	}
	for (cmsg = CMSG_FIRSTHDR(&m_message_handler);
		 cmsg && (cmsg->cmsg_level == SOL_SOCKET);
		 cmsg = CMSG_NXTHDR(&m_message_handler,cmsg)) {
		if (cmsg->cmsg_type == SO_RXQ_OVFL){									// ドロップパケット
			memcpy(&obj->dropcnt, CMSG_DATA(cmsg), sizeof(__u32));
		}
	}
	if (obj->dropcnt != obj->last_dropcnt) {									// ドロップパケットの比較
		__u32 frames = obj->dropcnt - obj->last_dropcnt;
		LOG(LOG_COMMON_OUT,"DROPCOUNT: dropped %d CAN frame%s on '%s' socket (total drops %d)\n",
				frames, (frames > 1)?"s":"", obj->ifname, obj->dropcnt);
		obj->last_dropcnt = obj->dropcnt;
	}
	m_recv_data_que.buff[m_recv_data_que.wp] = m_can_frame;						// 伝文取得

	if (m_recv_data_que.wp+1 < MAX_RXPACKET) ++m_recv_data_que.wp;				// ライトポインタ更新
	else									   m_recv_data_que.wp = 0;
}


/*-------------------------------------------------------------------
	FUNCTION :	GetCountSendQue()
	EFFECTS  :	バッファフル確認処理
	NOTE	 :
-------------------------------------------------------------------*/
uint GetCountSendQue(void)
{
	uint	result;

	result = 0;
	if( m_send_data_que.wp == m_send_data_que.rp ) {
		result = 0;
	}
	else if( m_send_data_que.wp > m_send_data_que.rp ) {
		result = m_send_data_que.wp - m_send_data_que.rp;
	}
	else {
		result = MAX_TXPACKET + m_send_data_que.wp - m_send_data_que.rp;
	}
	return result;
}


/*-------------------------------------------------------------------
	FUNCTION :	PushSendData()
	EFFECTS  :	//送信ﾊﾞｯﾌｧへの送信ﾃﾞｰﾀ退避(ﾒｲﾝﾀｽｸから呼び出すこと!)
	NOTE	 :
-------------------------------------------------------------------*/
bool PushSendData(T_CANFD_FRAME *p)
{
	bool ret = false;
	if( GetCountSendQue() < MAX_TXPACKET-1 ) {								//ｵｰﾊﾞｰﾗｲﾄ保護
		m_send_data_que.buff[m_send_data_que.wp] = *p;
		m_send_data_que.wp++;
		m_send_data_que.wp = (m_send_data_que.wp)%MAX_TXPACKET;
		ret = true;
	}
	return ret;
}


/*-------------------------------------------------------------------
	FUNCTION :	AnalyseCANRecvMessage()
	EFFECTS  :	CAN通信受信データ解析処理
	NOTE	 :
-------------------------------------------------------------------*/
void AnalyseCANRecvMessage(void)
{
	T_CANFD_FRAME	pac;
	int          	configflg = 0;
	int          	canclass;

	while(PopRecvMessage(&pac) ) {
		if(pac.can_id & CAN_ERR_FLAG){											// エラーの場合
			canclass = pac.can_id & CAN_EFF_MASK;
			if(canclass & 0x40){												// バスオフ検知
				errordetail.err7.bof_ch0 = 1;									// バスオフセット
			}
			if((canclass & 0x04) && ((pac.data[1] & 0x10) || (pac.data[1] & 0x20))){	// エラーパッシブ検知
				errordetail.err7.psv_ch0 = 1;											// エラーパッシブセット
			}
			else {
				errordetail.err7.psv_ch0 = 0;									// エラーパッシブクリア
			}
		}

		pthread_mutex_lock(&m_pcan_thread_mutex);
		if(RecvCanCommand(pac)) configflg =1;
		pthread_mutex_unlock(&m_pcan_thread_mutex);
		if(configflg==1){
			errordetail.err7.bof_ch0 = 0;										// エラーバスオフ解除
		}
	}
}


/*-------------------------------------------------------------------
	FUNCTION :	CreateCANThread()
	EFFECTS  :	CAN通信ｽﾚｯﾄﾞ生成
	NOTE	 :	CAN通信ｽﾚｯﾄﾞを生成する
-------------------------------------------------------------------*/
bool CreateCANThread(void)
{
	int resu_mutex;
	int resu_create;

	m_end_can_thread = 0;
	resu_mutex = pthread_mutex_init(&m_pcan_thread_mutex, NULL);
	resu_create = pthread_create(&m_can_thread, NULL, CANThreadFunc, NULL);

	 // ミューテックスとスレッドの生成が両方成功の場合
	if((resu_mutex==0) && (resu_create==0)){
		return true;
	}
	else{
		return false;
	}
}


/*-------------------------------------------------------------------
	FUNCTION :	DeleteCANThread()
	EFFECTS  :	CAN通信ｽﾚｯﾄﾞ削除
	NOTE	 :	CAN通信ｽﾚｯﾄﾞを削除する
-------------------------------------------------------------------*/
void DeleteCANThread(void)
{
	m_end_can_thread = 1;
	pthread_join(m_can_thread, NULL);
}


/*-------------------------------------------------------------------
	FUNCTION :	CANThreadFunc()
	EFFECTS  :	ｼﾘｱﾙ通信ｽﾚｯﾄﾞ関数
	NOTE	 :	ｼﾘｱﾙ通信ｽﾚｯﾄﾞ関数
-------------------------------------------------------------------*/
void *CANThreadFunc(void *targ)
{
	cpu_set_t cpu_set;
	int result;
	int ret;

	// Can_ThreadのCPUｱﾌｨﾆﾃｨ設定
	CPU_ZERO(&cpu_set);
	CPU_SET(1, &cpu_set);
	result = pthread_setaffinity_np(m_can_thread, sizeof(cpu_set_t), &cpu_set);
	if (result != 0) {

	}

	InitCanCommand();

	ret = LinkupCAN(CAN_CH);													// CANリンクアップ
	if(ret != 0){
		LOG(LOG_COMMON_OUT,"CAN Link Up Failed");
	}

    // 初期化直後の安定化時間を待つ
    sleep(2);

	LOG(LOG_COMMON_OUT,"CAN SetUp complet");

	MainLoopForCAN();															// CAN通信メイン処理

	return NULL;
}


/*-------------------------------------------------------------------
	FUNCTION :	MakeMessage()
	EFFECTS  :
	NOTE	 :
-------------------------------------------------------------------*/
void MakeMessage(ulong id,void *data, uint len)
{
	pthread_mutex_lock(&m_pcan_thread_mutex);									// ミューテックス確保
	QueuingSendMessage(id, data, len, 0);
	pthread_mutex_unlock(&m_pcan_thread_mutex);									// ミューテックス解除
}


/*-------------------------------------------------------------------
	FUNCTION :	EncodePgn()
	EFFECTS  :
	NOTE	 :
-------------------------------------------------------------------*/
ulong EncodePgn(uint edp, uint dp, uint pf, uint ps)
{
	return  ((edp & 0x00000001) << 17) |
			((dp  & 0x00000001) << 16) |
			((pf  & 0x000000FF) <<  8) |
			((ps  & 0x000000FF) <<  0);
}


/*-------------------------------------------------------------------
	FUNCTION :	DecodePgn()
	EFFECTS  :
	NOTE	 :
-------------------------------------------------------------------*/
void DecodePgn(ulong pgn, uint *edp, uint *dp, uint *pf, uint *ps)
{
	*edp	= (pgn >> 17) & 0x00000001;
	*dp		= (pgn >> 16) & 0x00000001;
	*pf		= (pgn >>  8) & 0x000000FF;
	*ps		= (pgn >>  0) & 0x000000FF;
}


/*-------------------------------------------------------------------
	FUNCTION :	EncodeArb()
	EFFECTS  :
	NOTE	 :
-------------------------------------------------------------------*/
ulong EncodeArb(uint pri, uint edp, uint dp, uint pf, uint ps, uint sa)
{
	ulong		result = 0;
	TCANFD_ARB	*arb;

	arb = (TCANFD_ARB *)&result;
	arb->p = pri;																//優先順位
	arb->pgn = EncodePgn(edp, dp, pf, ps);										//PGN
	arb->sa = sa;																//SA
	return result;
}


/*-------------------------------------------------------------------
	FUNCTION :	UpdateMFDTimer()
	EFFECTS  :	MFD接続確認関数
	NOTE	 :	MFD接続確認
-------------------------------------------------------------------*/
int UpdateMFDTimer(int flg){
	static double timer1000 = 0.;												// 経過時間
	static double oldTime1000 = GetTimeMsec();									// 前回呼び出された時刻
	const double nowTime1000 = GetTimeMsec();									// 現在時刻
	int result = 0;

	timer1000 = nowTime1000 - oldTime1000;										// 経過時間を計測
	if(flg == 0){
		timer1000 = 0.;
		oldTime1000 = nowTime1000;												// 時刻更新
	}
	else{
		if(timer1000 >= TIMEOUT_MFD){											// 周期時間を迎えた場合は更新
			timer1000 = 0.;
			result |= TIMER1000;
			oldTime1000 = nowTime1000;											// 時刻更新
		}
	}
	return result;
}


/*-------------------------------------------------------------------
	FUNCTION :	UpdateVCUTimer()
	EFFECTS  :	VCU接続確認関数
	NOTE	 :	VCU接続確認
-------------------------------------------------------------------*/
int UpdateVCUTimer(int flg){
	static double timer1000 = 0.;												// 経過時間
	static double oldTime1000 = GetTimeMsec();									// 前回呼び出された時刻
	const double nowTime1000 = GetTimeMsec();									// 現在時刻
	int result = 0;

	timer1000 = nowTime1000 - oldTime1000;										// 経過時間を計測
	if(flg == 0){
		timer1000 = 0.;
		oldTime1000 = nowTime1000;												// 時刻更新
	}
	else{
		if(timer1000 >= TIMEOUT_VCU){											// 周期時間を迎えた場合は更新
			timer1000 = 0.;
			result |= TIMER1000;
			oldTime1000 = nowTime1000;											// 時刻更新
		}
	}
	return result;
}
