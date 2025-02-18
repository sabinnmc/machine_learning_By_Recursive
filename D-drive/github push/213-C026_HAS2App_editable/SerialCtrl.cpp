//--------------------------------------------------
//! @file SerialCtrl.cpp
//! @brief ｼﾘｱﾙ通信制御ﾓｼﾞｭｰﾙ
//--------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include "Sysdef.h"
#include "Global.h"
#include "SerialCommand.h"

/*===	SECTION NAME		===*/


/*===	user definition		===*/
#define SERIAL_PORT "/dev/verdin-uart1"
//#define SERIAL_PORT "/dev/verdin-uart2" // for Verdin Development Board
//#define SERIAL_PORT "/dev/ttyPS1"
#define BAUDRATE B38400
#define SERIAL_SEND_WAIT	0

/*===	pragma variable	===*/


/*===	external variable	===*/


/*===	external function	===*/


/*===	public function prototypes	===*/


/*===	static function prototypes	===*/
void *serial_thread_func(void *targ);

/*===	static typedef variable		===*/
typedef struct {
	int id;
	int len;
} OpcIdInfo;

/*===	static variable		===*/
pthread_t serial_thread;
int	end_serial_thread;
SerialBuffType SerialBuff;
std::vector<OpcSendDataType> SendData;

/*===	constant variable	===*/
// 受信伝文ﾃｰﾌﾞﾙ
const OpcIdInfo OpcTable[] =
{
	{OPC_ID_RTC_SETREQ,		8},
	{OPC_ID_ERROR_DETECT,	4},
	{OPC_ID_ERROR_DETAIL,	6},
	{OPC_ID_SOUND_REQ,		1},
	{OPC_ID_SURVIVAL_REQ,	1},
	{OPC_ID_GETDATE_REQ,	1},
	{OPC_ID_VERSION_REQ,	1},
	{OPC_ID_SDCARD_TEST_REQ,	1},
	{OPC_ID_FPGA_VERSION_REQ,	1},
	{OPC_ID_ERROR_RESET,	1},
};

/*===	global variable		===*/

/*-------------------------------------------------------------------
	FUNCTION :	create_serial_thread()
	EFFECTS  :	ｼﾘｱﾙ通信ｽﾚｯﾄﾞ生成
	NOTE	 :	ｼﾘｱﾙ通信ｽﾚｯﾄﾞを生成する
-------------------------------------------------------------------*/
int create_serial_thread(void)
{
	SendData.clear();														// 送信伝文ﾍﾞｸﾀｰｸﾘｱ
	end_serial_thread = 0;
	pthread_mutex_init(&g_SerialMutex, NULL);
	if(pthread_create(&serial_thread, NULL, serial_thread_func, NULL) != 0){
		ERR("serial thread create failed");
		return(-1);
	}
	return(0);
}
/*-------------------------------------------------------------------
	FUNCTION :	delete_serial_thread()
	EFFECTS  :	ｼﾘｱﾙ通信ｽﾚｯﾄﾞ生成
	NOTE	 :	ｼﾘｱﾙ通信ｽﾚｯﾄﾞを生成する
-------------------------------------------------------------------*/
int delete_serial_thread(void)
{
	int ret;
	end_serial_thread = 1;
	ret = pthread_join(serial_thread, NULL);
	if( ret != 0 ){
		ERR("delete_serial_thread pthread_join failed %d",ret);
	}
	return(0);
}

/*-------------------------------------------------------------------
	FUNCTION :	serial_data_send()
	EFFECTS  :	ｼﾘｱﾙﾃﾞｰﾀ送信
	NOTE	 :	ｼﾘｱﾙﾃﾞｰﾀ送信関数
-------------------------------------------------------------------*/
int serial_data_send(int fd, char *data, int len)
{
#if SERIAL_SEND_WAIT
	int i;
#endif
	int result = 0;
	int ret;


#if SERIAL_SEND_WAIT
	for(i = 0; i < len; i++ ){
		ret = write(fd, &data[i], 1);
		if(ret != 1){
			result = -1;
			break;
		}
		usleep(100);
	}
#else
		ret = write(fd, data, len);
		if( ret != len ){
			result = -1;
		}
#endif

	return(result);
}

/*-------------------------------------------------------------------
	FUNCTION :	serial_thread_func()
	EFFECTS  :	ｼﾘｱﾙ通信ｽﾚｯﾄﾞ関数
	NOTE	 :	ｼﾘｱﾙ通信ｽﾚｯﾄﾞ関数
-------------------------------------------------------------------*/
void *serial_thread_func(void *targ)
{
	int fd;
	int state = 0;
	char buf[SERIAL_BUF_SIZE];
	struct termios oldtio,newtio;
	int len,ret;
	int available_size = 0;
	int free_size;
	int tp;
	int i,j;
	OpcDataType opcrcv;
	uchar sum;
	OpcSendDataType send_data;

	memset((void *)&SerialBuff, 0x00, sizeof(SerialBuffType));
	SerialBuff.wp = SERIAL_BUF_SIZE - 1;
	SerialBuff.rp = SERIAL_BUF_SIZE - 1;

	while (!end_serial_thread)
	{
		usleep(100000);

		if( state == 0 ){													// ｼﾘｱﾙ未ｵｰﾌﾟﾝ状態
			fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY );						// ｼﾘｱﾙｵｰﾌﾟﾝ
			if( fd < 0 ){													// ｵｰﾌﾟﾝ失敗
				usleep(100000);												// 100msｽﾘｰﾌﾟ
				continue;
			}
			tcgetattr(fd,&oldtio);											// 現在のﾎﾟｰﾄ設定を退避
			bzero(&newtio, sizeof(newtio));									// ﾎﾟｰﾄ設定初期化
			newtio.c_cflag = BAUDRATE| CS8 | CLOCAL | CREAD | PARENB;		// 38400pbs | ﾃﾞｰﾀﾋﾞｯﾄ:8bit | ｽﾄｯﾌﾟﾋﾞｯﾄ:1bit | ﾊﾟﾘﾃｨ EVEN
			newtio.c_cc[VTIME] = 0;											// ｷｬﾗｸﾀ間ﾀｲﾏ未使用
			newtio.c_cc[VMIN] = 0;											// 受信ﾌﾞﾛｯｸなし
			tcsetattr( fd, TCSANOW, &newtio );								// ﾃﾞﾊﾞｲｽへ設定を行う
			ioctl(fd, TCSETS, &newtio);										// ﾎﾞｰﾚｰﾄの設定を有効にする
			state = 1;														// ｼﾘｱﾙ通信有効
			LOG(LOG_COMMON_OUT,"[UART] Serial Open");
		}
		else if(state == 1){												// ｼﾘｱﾙ受信及び送信要求受付
			ioctl(fd, FIONREAD, &available_size);							// ｶｰﾈﾙﾊﾞｯﾌｧの受信ｻｲｽﾞ取得
			if( available_size >= OPC_SIZE_MIN ){
				// ｼﾘｱﾙﾊﾞｯﾌｧ空きｻｲｽﾞ算出
				free_size = SERIAL_BUF_SIZE - SerialBuff.cnt;
				if( available_size > free_size ){
					available_size = free_size;
				}
				len = read(fd, buf, available_size);

				if( len > 0){
					tp = SerialBuff.wp+1;
					if( tp >  SERIAL_BUF_SIZE - 1 ) tp = 0;

					if( tp + len <= SERIAL_BUF_SIZE ){
						memcpy(&SerialBuff.buff[tp] , buf, len );
						SerialBuff.wp += len;
						if( SerialBuff.wp > SERIAL_BUF_SIZE - 1 )	SerialBuff.wp -= SERIAL_BUF_SIZE;
					}
					else{
						int bufpos;
						memcpy(&SerialBuff.buff[tp] , buf, SERIAL_BUF_SIZE - tp );
						bufpos = SERIAL_BUF_SIZE - tp;
						memcpy(&SerialBuff.buff[0] , &buf[bufpos], len - (SERIAL_BUF_SIZE - tp) );
						SerialBuff.wp += len;
						if( SerialBuff.wp > SERIAL_BUF_SIZE - 1 )	SerialBuff.wp -= SERIAL_BUF_SIZE;
					}
					SerialBuff.cnt += len;
				}
				else{
					WARNING("[UART]read failed : %d",len);
				}
			}

			while(SerialBuff.cnt >= OPC_SIZE_MIN){
				int data_pos = 0;
				int opc_num;
				int opc_check_flg = 0;
				tp = SerialBuff.rp;
				// 伝文ﾍｯﾀﾞ取り出し
				for( i = 0 ; i < OPC_HEADER_SIZE; i++ ){
					tp++;
					if( tp > SERIAL_BUF_SIZE -1 ) tp = 0;
					opcrcv.data[data_pos] = SerialBuff.buff[tp];
					data_pos++;
				}
				opc_num = sizeof(OpcTable)/sizeof(OpcIdInfo);
				for( i = 0; i < opc_num; i++ ){
					if( OpcTable[i].id == opcrcv.header.id ){
						if( opcrcv.header.len == OpcTable[i].len ){
							opc_check_flg = 1;
						}
						else{
							WARNING("[UART]OPC Check Len NG ID:%02X len:%02X opclen:%02X",opcrcv.header.id,opcrcv.header.len,OpcTable[i].len);
						}
						break;
					}
				}
				if( i >= opc_num ){
					WARNING("[UART]Unknown OPC ID:%02X",opcrcv.header.id);
				}
				if( !opc_check_flg ){
					tp = SerialBuff.rp;
					tp++;
					if( tp > SERIAL_BUF_SIZE -1 ) tp = 0;
					SerialBuff.rp = tp;																			// ﾘｰﾄﾞﾎﾟｲﾝﾀ更新
					SerialBuff.cnt -= 1;
					continue;
				}
				opc_check_flg = 0;


				if( OPC_HEADER_SIZE + OPC_CRC_SIZE + opcrcv.header.len > SerialBuff.cnt ){				// 該当伝文のﾃﾞｰﾀがそろってない場合
					break;
				}
				// 残りの伝文情報取り出し
				for( i = 0 ; i < OPC_CRC_SIZE + opcrcv.header.len; i++ ){
					tp++;
					if( tp > SERIAL_BUF_SIZE -1 ) tp = 0;
					opcrcv.data[data_pos] = SerialBuff.buff[tp];
					data_pos++;
				}

				sum = 0;
				for(j = 0; j < opcrcv.header.len; j++ ){
					sum += opcrcv.data[OPC_HEADER_SIZE+j];
				}
				sum = ~sum & 0xFF;
				if( sum == opcrcv.data[OPC_HEADER_SIZE+opcrcv.header.len] ){
					rcvopc_notice(opcrcv);
					LOG(LOG_SERIAL_DEBUG,"[UART] Opc %02X Rcv",opcrcv.header.id);
					opc_check_flg = 1;
				}
				else{
					WARNING("[UART]OPC Check Sum NG ID:%02X sum:%02X opcsum:%02X",opcrcv.header.id,sum,opcrcv.data[OPC_HEADER_SIZE+opcrcv.header.len]);
				}
				if( opc_check_flg ){
					SerialBuff.rp = tp;																			// ﾘｰﾄﾞﾎﾟｲﾝﾀ更新
					SerialBuff.cnt -= OPC_HEADER_SIZE + OPC_CRC_SIZE + opcrcv.header.len;					// 受信ﾊﾞｯﾌｧｶｳﾝﾄ更新
				}
				else{
					tp = SerialBuff.rp;
					tp++;
					if( tp > SERIAL_BUF_SIZE -1 ) tp = 0;
 					SerialBuff.rp = tp;																			// ﾘｰﾄﾞﾎﾟｲﾝﾀ更新
					SerialBuff.cnt -= 1;
				}

			}

			while(SendData.size()){											// 送信要求ﾃﾞｰﾀ分ﾙｰﾌﾟ
				pthread_mutex_lock(&g_SerialMutex);
				send_data = SendData[0];									// 先頭のﾃﾞｰﾀ取り出し
				SendData.erase(SendData.begin());
				pthread_mutex_unlock(&g_SerialMutex);
				len = OPC_HEADER_SIZE+OPC_CRC_SIZE+send_data.header.len;	// 送信ｻｲｽﾞ算出
				ret = serial_data_send(fd, (char *)send_data.data, len);			// ﾃﾞｰﾀ送信
				if( ret == -1 ){
					ERR("[UART]Opc Send Failed");
				}
			}
		}
	}
	if( state != 0 ){
		tcsetattr(fd,TCSANOW,&oldtio);										// 元の設定に戻す
		ioctl(fd, TCSETS, &newtio);											// ﾎﾞｰﾚｰﾄの設定を有効にする
		close(fd);															// ｼﾘｱﾙｸﾛｰｽﾞ
	}

	return NULL;
}

/*-------------------------------------------------------------------
	FUNCTION :	serial_send_req()
	EFFECTS  :	ｼﾘｱﾙ通信伝文送信要求
	NOTE	 :	ｼﾘｱﾙ通信伝文送信要求
-------------------------------------------------------------------*/
int serial_send_req(OpcSendDataType req_data)
{
	pthread_mutex_lock(&g_SerialMutex);
	if( SendData.size() <= OPC_SEND_QUEUE_NUM ){
		SendData.push_back(req_data);
	}
	else{
		ERR("Send Opc Buff FULL");
	}
	pthread_mutex_unlock(&g_SerialMutex);
	return 0;
}
