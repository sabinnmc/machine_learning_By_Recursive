#pragma once
#include <opencv2/opencv.hpp>

bool set_image_disp0(cv::Mat &img);
bool set_image_disp1(cv::Mat &img);

//--------------------------------------------------
// 映像出力管理クラス
// DualBufferで/dev/fbxとのデータ制御を行う
//--------------------------------------------------
class DispControl {
  public:
	DispControl();
	~DispControl();
	bool Init(std::string);
	bool set_image(const cv::Mat &img);
	void show_image(void);
	void show_update_progress(void);

  private:
	std::string m_dev;
	double m_delay;
	cv::Mat m_img[2];
	cv::Mat m_disp;
	cv::TickMeter m_meter;
	int m_wp;
	int m_rp;
	int m_size;
	int m_update_icon;
};
