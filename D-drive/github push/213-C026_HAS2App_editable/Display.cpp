#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

#include "Sysdef.h"
#include "Display.hpp"
#include "Global.h"

/*===	SECTION NAME		===*/


/*===	user definition		===*/


/*===	pragma variable	===*/


/*===	external variable	===*/


/*===	external function	===*/


/*===	public function prototypes	===*/


/*===	static function prototypes	===*/
pthread_t disp0_thread;
int	end_disp0_thread;
pthread_t disp1_thread;
int	end_disp1_thread;


/*===	static typedef variable		===*/


/*===	static variable		===*/
void *disp0_thread_func(void *targ);
void *disp1_thread_func(void *targ);


/*===	constant variable	===*/


/*===	global variable		===*/
pthread_mutex_t  g_Disp0Mutex;											// Dispaly0 Mutex
DispControl *g_Disp0;
pthread_mutex_t  g_Disp1Mutex;											// Dispaly1 Mutex
DispControl *g_Disp1;


/*-------------------------------------------------------------------
  FUNCTION :	create_disp0_thread()
  EFFECTS  :	ディスプレイ（fb0）の映像出力スレッド生成
  NOTE	 :
-------------------------------------------------------------------*/
int create_disp0_thread(void)
{
	end_disp0_thread = 0;
	g_Disp0 = new DispControl();
	if (g_Disp0->Init("/dev/fb0")==false) {
		ERR("disp0 initialize failed");
		return(-1);
	}
	pthread_mutex_init(&g_Disp0Mutex, NULL);
	if(pthread_create(&disp0_thread, NULL, disp0_thread_func, NULL) != 0){
		ERR("disp0 thread create failed");
		return(-1);
	}
	return(0);
}


/*-------------------------------------------------------------------
  FUNCTION :	delete_disp0_thread()
  EFFECTS  :	ディスプレイ（fb0）の映像出力スレッド削除
  NOTE	 :
  -------------------------------------------------------------------*/
int delete_disp0_thread(void)
{
	int ret;
	end_disp0_thread = 1;
	ret = pthread_join(disp0_thread, NULL);
	if( ret != 0 ){
		ERR("delete_disp0_thread pthread_join failed");
	}
	delete(g_Disp0);
	return(0);
}


/*-------------------------------------------------------------------
  FUNCTION :	set_image_disp
  EFFECTS  :	映像受け渡し
  NOTE	 :
-------------------------------------------------------------------*/
bool set_image_disp0(cv::Mat &img)
{
	bool ret;

	ret = g_Disp0->set_image(img);
	if (ret == false) {
		end_disp0_thread = 1;
	}
	return ret;
}


/*-------------------------------------------------------------------
	FUNCTION :	disp0_thread_func()
	EFFECTS  :	ディスプレイ（fb0）スレッド関数
	NOTE	 :	MIPI-DSI側の画面表示
-------------------------------------------------------------------*/
void *disp0_thread_func(void *targ)
{
	while (!end_disp0_thread)
	{
		if (g_updatestatus != kUpdateStop) {
			g_Disp0->show_update_progress();
		}
		else {
			g_Disp0->show_image();
		}
		usleep(1000);
	}

	return NULL;
}


 /*-------------------------------------------------------------------
  FUNCTION :	create_disp1_thread()
  EFFECTS  :	ディスプレイ（fb1）の映像出力スレッド生成
  NOTE	 :
  -------------------------------------------------------------------*/
int create_disp1_thread(void)
{
	end_disp1_thread = 0;
	g_Disp1 = new DispControl();
	if (g_Disp1->Init("/dev/fb1")==false) {
		ERR("disp1 initialize failed");
		return(-1);
	}
	pthread_mutex_init(&g_Disp1Mutex, NULL);
	if(pthread_create(&disp1_thread, NULL, disp1_thread_func, NULL) != 0){
		ERR("disp1 thread create failed");
		return(-1);
	}
	return(0);
}


/*-------------------------------------------------------------------
  FUNCTION :	delete_disp1_thread()
  EFFECTS  :	ディスプレイ（fb1）の映像出力スレッド削除
  NOTE	 :
  -------------------------------------------------------------------*/
int delete_disp1_thread(void)
{
	int ret;
	end_disp1_thread = 1;
	ret = pthread_join(disp1_thread, NULL);
	if( ret != 0 ){
		ERR("delete_disp1_thread pthread_join failed");
	}
	delete(g_Disp1);
	return(0);
}


/*-------------------------------------------------------------------
  FUNCTION :	set_image_disp1
  EFFECTS  :	映像受け渡し
  NOTE	 :
-------------------------------------------------------------------*/
bool set_image_disp1(cv::Mat &img)
{
	bool ret;

	ret = g_Disp1->set_image(img);
	if (ret == false) {
		end_disp1_thread = 1;
	}
	return ret;
}


/*-------------------------------------------------------------------
	FUNCTION :	disp1_thread_func()
	EFFECTS  :	ディスプレイ（fb1）スレッド関数
	NOTE	 :	MIPI-DSI側の画面表示
-------------------------------------------------------------------*/
void *disp1_thread_func(void *targ)
{
	while (!end_disp1_thread)
	{
		if (g_updatestatus != kUpdateStop) {
			g_Disp1->show_update_progress();
		}
		else {
			g_Disp1->show_image();
		}
		usleep(1000);
	}

	return NULL;
}


//==================================================================
// DispControlクラス
//==================================================================
/*-------------------------------------------------------------------
  FUNCTION :	コンストラクタ
  EFFECTS  :
  NOTE	 :
-------------------------------------------------------------------*/
DispControl::DispControl()
{
	m_img[0] = cv::Mat();
	m_img[1] = cv::Mat();
	m_delay = 0.0;
	m_rp = 0;
	m_wp = 0;
}


/*-------------------------------------------------------------------
  FUNCTION :	コンストラクタ
  EFFECTS  :
  NOTE	 :
-------------------------------------------------------------------*/
bool DispControl::Init(std::string name)
{
	bool ret = true;
	int fbfd;
	uint8_t *fbdata;

	m_size = IMAGE_WIDTH_WIDE*IMAGE_HEIGHT_WIDE*4;
	m_disp = cv::Mat(IMAGE_HEIGHT_WIDE, IMAGE_WIDTH_WIDE, CV_8UC4);
	m_update_icon = 0;

	m_dev = name;
	m_meter.reset();

	// CHECK
	// device
	fbfd = open (m_dev.c_str(), O_RDWR);
	if(fbfd<0){
		ERR("open failed %s", name.c_str());
		ret = false;
	}
	else {
		// mmap
		fbdata = (uint8_t *)mmap (0, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, (off_t)0);
		if (fbdata == MAP_FAILED) {
			ERR("mmap failed %s", name.c_str());
			ret = false;
		}
		else {
			munmap(fbdata, m_size);
		}
		close(fbfd);
	}
	return ret;
}


/*-------------------------------------------------------------------
  FUNCTION :	デストラクタ
  EFFECTS  :
  NOTE	 :
  -------------------------------------------------------------------*/
DispControl::~DispControl()
{
	m_disp.release();
}


/*-------------------------------------------------------------------
  FUNCTION :	set_image
  EFFECTS  :	画像の受け渡し
  NOTE	 :
-------------------------------------------------------------------*/
bool DispControl::set_image(const cv::Mat &input)
{
	bool ret = true;

	if (m_wp == m_rp) {
		m_img[m_wp] = input.clone();
		m_wp = (m_wp+1)%2;
		m_delay = 0.0;
	}
	else {
		//ERR("%s rp did not make it!!!", m_dev.c_str());
		//		ret = false;
	}
	return ret;
}

/*-------------------------------------------------------------------
  FUNCTION :	show_image
  EFFECTS  :	画像の表示
  NOTE	 :
-------------------------------------------------------------------*/
void DispControl::show_image(void)
{
	if (m_wp != m_rp) {
		int fbfd = open (m_dev.c_str(), O_RDWR);
		if(fbfd<0){
			ERR("open failed %s", m_dev.c_str());
			return;
		}

		uint8_t *fbdata = (uint8_t *)mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, (off_t)0);
		if(fbdata){
			ioctl(fbfd, FBIO_WAITFORVSYNC, 0);
			m_disp.data = fbdata;
			cv::cvtColor(m_img[m_rp], m_disp, cv::COLOR_BGR2BGRA);
			munmap (fbdata, m_size);
			// m_meter.stop();
			// m_meter.start();
			// if (m_meter.getCounter () > 100 ) {
			// 	std::cout << m_dev << ": " << m_meter.getAvgTimeMilli() << std::endl;
			// 	m_meter.reset();
			// }
		}
		close (fbfd);
		m_rp = (m_rp+1)%2;
	}
}

/*-------------------------------------------------------------------
  FUNCTION :	show_update_progress
  EFFECTS  :	アップデート画像の表示
  NOTE	 :
-------------------------------------------------------------------*/
void DispControl::show_update_progress(void)
{
	int fbfd = open (m_dev.c_str(), O_RDWR);
	if(fbfd<0){
		ERR("open failed %s", m_dev.c_str());
		return;
	}

	uint8_t *fbdata = (uint8_t *)mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, (off_t)0);
	if(fbdata){
		ioctl(fbfd, FBIO_WAITFORVSYNC, 0);
		m_disp.data = fbdata;
		switch (g_updatestatus) {
			case kUpdateWait: {
				cv::Mat update_img = cv::Mat::zeros(IMAGE_HEIGHT_WIDE, IMAGE_WIDTH_WIDE, CV_8UC3);
				int start_angle = m_update_icon;
				int end_angle = (m_update_icon+270)%360;
				m_update_icon = (m_update_icon+10) % 360;
				cv::Point center(IMAGE_WIDTH_WIDE / 2, IMAGE_HEIGHT_WIDE / 2);
				cv::ellipse(update_img, center, cv::Size(100, 100), 0, start_angle, end_angle, cv::Scalar(0, 255, 0), 40);
				cv::cvtColor(update_img, m_disp, cv::COLOR_BGR2BGRA);
				break;
			}

			case kUpdateFinsh: {
				cv::Mat update_img = cv::imread("update_success.jpg");
				resize(update_img, update_img, cv::Size(IMAGE_WIDTH_WIDE, IMAGE_HEIGHT_WIDE));
				cv::cvtColor(update_img, m_disp, cv::COLOR_BGR2BGRA);
				break;
			}

			default: {
				cv::Mat update_img = cv::imread("update_fail.jpg");
				resize(update_img, update_img, cv::Size(IMAGE_WIDTH_WIDE, IMAGE_HEIGHT_WIDE));
				cv::cvtColor(update_img, m_disp, cv::COLOR_BGR2BGRA);
				break;
			}
		}
		munmap (fbdata, m_size);
	}
	close (fbfd);
}
