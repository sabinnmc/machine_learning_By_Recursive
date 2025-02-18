#pragma once
#include <opencv2/opencv.hpp>

// 画像取得関連
enum class CaptureResult{
	Success = 0,	//!< 成功
	FailedOpen,		//!< ｵｰﾌﾟﾝ失敗
	FailedRead,		//!< 読み込み失敗
	InvalidDeviceStatus,		//!< デコーダのステータス異常
};

CaptureResult get_image_cap0(cv::Mat &img);
CaptureResult get_image_cap1(cv::Mat &img);

//--------------------------------------------------
// 映像出力管理クラス
// DualBufferで/dev/videoXとのデータ制御を行う
//--------------------------------------------------
class CapControl {
  public:
	CapControl();
	~CapControl();
	bool Init();
	void set_image(cv::Mat*const &image, cv::VideoCapture*const &capture);
	bool capture(cv::Mat*const input);
	CaptureResult GetCaptureStatus(void);

  private:
	cv::Mat m_img[2];
    int m_wp;
    int m_rp;
    CaptureResult m_cap_state;
	CaptureResult m_img_state;
};
