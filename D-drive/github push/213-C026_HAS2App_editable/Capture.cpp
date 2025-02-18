#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <chrono>

#include "Sysdef.h"
#include "Capture.hpp"
#include "BoardCtrl.hpp"
#include "Global.h"
#include "Param.hpp"

/*===	SECTION NAME		===*/


/*===	user definition		===*/


/*===	pragma variable	===*/


/*===	external variable	===*/
extern ErrorInfoType errorinfo;													// ｴﾗｰ情報
extern ErrorDetailType errordetail;												// ｴﾗｰ詳細情報

/*===	external function	===*/


/*===	public function prototypes	===*/


/*===	static function prototypes	===*/
pthread_t cap0_thread;
int	end_cap0_thread;
pthread_t cap1_thread;
int	end_cap1_thread;

/*===	static typedef variable		===*/


/*===	static variable		===*/
void *cap0_thread_func(void *targ);
void *cap1_thread_func(void *targ);


/*===	constant variable	===*/


/*===	global variable		===*/
pthread_mutex_t  g_Cap0Mutex;											// Capture0 Mutex
CapControl *g_Cap0;														// class CapControl pointer object = *g_cap0
pthread_mutex_t  g_Cap1Mutex;											// Catpure1 Mutex
CapControl *g_Cap1;
const auto& inferenceTypeData = g_configData.GetInferenceTypeData(static_cast<eInferenceType>(0));

/*-------------------------------------------------------------------
  FUNCTION :	create_cap0_thread()
  EFFECTS  :	ディスプレイ（fb0）の映像出力スレッド生成
  NOTE	 :
-------------------------------------------------------------------*/
int create_cap0_thread(void)
{
	end_cap0_thread = 0;
	g_Cap0 = new CapControl();
	if (g_Cap0->Init()==false) {
		ERR("cap0 initialize failed");
		return(-1);
	}
	pthread_mutex_init(&g_Cap0Mutex, NULL);
	if(pthread_create(&cap0_thread, NULL, cap0_thread_func, NULL) != 0){
		printf("cap0 thread create failed");
		return(-1);
	}
	return(0);
}


/*-------------------------------------------------------------------
  FUNCTION :	delete_cap0_thread()
  EFFECTS  :	ディスプレイ（fb0）の映像出力スレッド削除
  NOTE	 :
  -------------------------------------------------------------------*/
int delete_cap0_thread(void)
{
	int ret;
	end_cap0_thread = 1;
	ret = pthread_join(cap0_thread, NULL);
	if( ret != 0 ){
		ERR("delete_cap0_thread pthread_join failed");
	}
	delete(g_Cap0);
	return(0);
}

/*-------------------------------------------------------------------
  FUNCTION :	get_image_cap
  EFFECTS  :	映像受け渡し
  NOTE	 :
-------------------------------------------------------------------*/
CaptureResult get_image_cap0(cv::Mat &img)
{
	bool ret;
	
	ret = g_Cap0->capture(&img);
	if (ret == false) {
		end_cap0_thread = 1;
	}

	return g_Cap0->GetCaptureStatus();
}


/*-------------------------------------------------------------------
	FUNCTION :	cap0_thread_func()
	EFFECTS  :	ディスプレイ（fb0）スレッド関数
	NOTE	 :	MIPI-DSI側の画面表示
-------------------------------------------------------------------*/
void *cap0_thread_func(void *targ)
{
	
	cv::VideoCapture image1;
	cv::Mat image;
	while (!end_cap0_thread)
	{
		g_Cap0->set_image(&image,&image1);
		usleep(1000);
	}

	return NULL;
}


 /*-------------------------------------------------------------------
  FUNCTION :	create_cap1_thread()
  EFFECTS  :	ディスプレイ（fb1）の映像出力スレッド生成
  NOTE	 :
  -------------------------------------------------------------------*/
int create_cap1_thread(void)
{
	end_cap1_thread = 0;
	g_Cap1 = new CapControl();
	if (g_Cap1->Init()==false) {
		ERR("cap1 initialize failed");
		return(-1);
	}
	pthread_mutex_init(&g_Cap1Mutex, NULL);
	if(pthread_create(&cap1_thread, NULL, cap1_thread_func, NULL) != 0){
		ERR("cap1 thread create failed");
		return(-1);
	}
	return(0);
}


/*-------------------------------------------------------------------
  FUNCTION :	delete_cap1_thread()
  EFFECTS  :	ディスプレイ（fb1）の映像出力スレッド削除
  NOTE	 :
  -------------------------------------------------------------------*/
int delete_cap1_thread(void)
{
	int ret;
	end_cap1_thread = 1;
	ret = pthread_join(cap1_thread, NULL);
	if( ret != 0 ){
		ERR("delete_cap1_thread pthread_join failed");
	}
	delete(g_Cap1);
	return(0);
}


/*-------------------------------------------------------------------
  FUNCTION :	set_image_cap1
  EFFECTS  :	映像受け渡し
  NOTE	 :
-------------------------------------------------------------------*/
CaptureResult get_image_cap1(cv::Mat &img)
{
	bool ret;

	ret = g_Cap1->capture(&img);
	if (ret == false) {
		end_cap1_thread = 1;
	}

	return g_Cap1->GetCaptureStatus();
}




/*-------------------------------------------------------------------
	FUNCTION :	cap1_thread_func()
	EFFECTS  :	read single static fisheye image 
	NOTE	 :	MIPI-DSI側の画面表示
-------------------------------------------------------------------*/
void *cap1_thread_func(void *targ)
{
	cv::Mat image;
	cv::VideoCapture image1;
	

	while (!end_cap1_thread)
	{
		g_Cap1->set_image(&image,&image1);
		usleep(1000);
	}

	return NULL;
}


//==================================================================
// CapControlクラス
//==================================================================
/*-------------------------------------------------------------------
  FUNCTION :	コンストラクタ
  EFFECTS  :
  NOTE	 :
-------------------------------------------------------------------*/
CapControl::CapControl()
{
	m_img[0] = cv::Mat();
	m_img[1] = cv::Mat();
	m_rp = 0;
	m_wp = 0;
}


/*-------------------------------------------------------------------
  FUNCTION :	コンストラクタ
  EFFECTS  :
  NOTE	 :
-------------------------------------------------------------------*/
bool CapControl::Init()
{
	return true;
}


/*-------------------------------------------------------------------
  FUNCTION :	デストラクタ
  EFFECTS  :
  NOTE	 :
  -------------------------------------------------------------------*/
CapControl::~CapControl()
{

}

//--------------------------------------------------
//! @brief 画像取得
//!
//! ｷｬﾌﾟﾁｬ用ｵﾌﾞｼﾞｪｸﾄがｵｰﾌﾟﾝされていなかった場合、ｵｰﾌﾟﾝ処理も行う
//! @param[out]		image			取得した画像
//! @param[in,out]	capture			ｷｬﾌﾟﾁｬ用ｵﾌﾞｼﾞｪｸﾄ
//! @param[in]		dev				推論ﾀｲﾌﾟごとの設定ﾃﾞｰﾀ
//! @return			画像取得結果
//! @details		edit n1
//--------------------------------------------------
CaptureResult capture_image(cv::Mat*const image, cv::VideoCapture*const capture){
	if(inferenceTypeData.m_sourceType == ConfigData::InferenceTypeData::eSourceType::DEVICE){
		if(capture_state() == false) {
			errordetail.err8.cap_stat = 1;
			return CaptureResult::InvalidDeviceStatus;
		}
	}
	
	if(g_chsize == 1 ) {
		capture->release();
		usleep(5000);
	}
	
	// 画像取得 -> がぞう　しゅとく　ー＞ image acquisition

	(*capture) >> *image; 

	// 取得成功
	if(image->empty() == false){
		errordetail.err8.cap_stat = 0;
#ifdef INPUT_TVI720P
		//resize(*image, *image, cv::Size(IMAGE_WIDTH_FHD, IMAGE_HEIGHT_FHD),0,0,cv::INTER_NEAREST);
#endif
		return CaptureResult::Success;
	}

	// 取得に失敗したのでｵｰﾌﾟﾝ
	if(inferenceTypeData.m_sourceType == ConfigData::InferenceTypeData::eSourceType::DEVICE){
		capture->open(inferenceTypeData.m_sourceDeviceNo);
	}else{
		capture->open(inferenceTypeData.m_sourceFilePath);
	}
	// ｵｰﾌﾟﾝ失敗
	if(!capture->isOpened()){
		if(errorinfo.err3.input_img != 3){
		}
		errordetail.err8.cap_stat = 1;
		return CaptureResult::FailedOpen;
	}
#ifdef INPUT_TVI720P
	capture->set(cv::CAP_PROP_BUFFERSIZE,3);	// 2が設定出来る最小値(映像遅延100ms程度)
	capture->set(cv::CAP_PROP_FRAME_WIDTH,IMAGE_WIDTH_WIDE);
	capture->set(cv::CAP_PROP_FRAME_HEIGHT,IMAGE_HEIGHT_WIDE);
	capture->set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y','U','Y','V'));
#else
	capture->set(cv::CAP_PROP_BUFFERSIZE,3);	// 2が設定出来る最小値(映像遅延100ms程度)
	//capture->set(cv::CAP_PROP_CONVERT_RGB,false);
	capture->set(cv::CAP_PROP_FRAME_WIDTH,IMAGE_WIDTH_FHD);
	capture->set(cv::CAP_PROP_FRAME_HEIGHT,IMAGE_HEIGHT_FHD);
	capture->set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y','U','Y','V'));
	//capture->set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('N','V','1','2'));
#endif
	// ｵｰﾌﾟﾝしたので画像取得し直し
	(*capture) >> *image;
	// 取得成功
	if(image->empty() == false){
		(*capture) >> *image;	// dummy read
		(*capture) >> *image;	// dummy read
		errordetail.err8.cap_stat = 0;
#ifdef INPUT_TVI720P
		//resize(*image, *image, cv::Size(IMAGE_WIDTH_FHD, IMAGE_HEIGHT_FHD),0,0,cv::INTER_NEAREST);
#endif
		return CaptureResult::Success;
	}

	return CaptureResult::FailedRead;
}

/*-------------------------------------------------------------------
  FUNCTION :	set_image
  EFFECTS  :	画像の受け渡し
  NOTE	 :
-------------------------------------------------------------------*/
void CapControl::set_image(cv::Mat*const &image, cv::VideoCapture*const &capture)
{
	m_cap_state = capture_image(image,capture);

	if(m_cap_state == CaptureResult::Success){
		if (m_wp == m_rp) {
			m_img[m_wp] = image->clone();
			m_wp = (m_wp+1)%2;
			g_capcomp = 1;
		}
	}
	if(m_cap_state == CaptureResult::InvalidDeviceStatus){						// デバイスの状態異常であれば
		usleep(66666);															// 15fps待つ
		g_capcomp = 1;															// エラー表示のために取得できたことにする
	}
}

/*-------------------------------------------------------------------
  FUNCTION :	show_image
  EFFECTS  :	画像の表示
  NOTE	 :
-------------------------------------------------------------------*/
bool CapControl::capture(cv::Mat*const input)
{
	if (m_wp != m_rp) {															// 新しいフレームを取得済みであれば
		g_capcomp = 0;															// 次のキャプチャ要求   def: cv::Mat m_img[2];
		*input = m_img[m_rp];													// 取得カメラ映像をコピー   //copy captured camera footage
		m_rp = (m_rp+1)%2;
	}
	return true;
}


/*-------------------------------------------------------------------
  FUNCTION :	GetCaptureStatus
  EFFECTS  :	画像取得状態のリード
  NOTE	 :
  -------------------------------------------------------------------*/
CaptureResult CapControl::GetCaptureStatus(void)
{
	return m_cap_state;
}
