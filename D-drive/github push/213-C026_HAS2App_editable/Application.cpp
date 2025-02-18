/********************************************************************************
;*                                                                                *
;*        Copyright (C) 2024 PAL Inc. All Rights Reserved                         *
;*                                                                                *
;********************************************************************************
;*                                                                                *
;*        ﾓｼﾞｭｰﾙ名      :    Application.cpp	                                  *
;*                                                                                *
;*        作　成　者    :    内之浦　伸治                                         *
;*                                                                                *
;********************************************************************************/

#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <dirent.h>

#include "Sysdef.h"
#include "Application.h"
#include "LinuxUtility.hpp"
#include "Global.h"
#include "SerialCommand.h"
#include "Inference.hpp"
#include "Recoder.hpp"
#include "Display.hpp"
#include "Capture.hpp"
#include "TransForm.hpp"
#include "BoardCtrl.hpp"
#include "ExternalMemoryUtility.hpp"
#include "testing.hpp"


/*===	SECTION NAME		===*/

/*===	user definition		===*/

/*===	pragma variable	===*/

/*===	external variable	===*/
extern ErrorInfoType errorinfo;									// ｴﾗｰ情報
extern ErrorDetailType errordetail;								// ｴﾗｰ詳細情報


/*===	external function	===*/
extern void error_info_save(ErrorInfoType);						// utility.cpp
extern void error_info_send_req(void);
extern void get_bboxes(const vector<float> &tensor, vector<float> &boxes);
extern int get_bboxes_size(const vector<float> &boxes);
extern void CheckRTC(bool);
extern void CheckWattageOfSOM(void);
extern void UpdateWDTPulse(void);
extern int cust_getchar(void);
extern void	UpdateConfigFileforLutFavoriteTable(void);
extern int ReadFavariteLUTConf(void);


void get_image_static(cv::Mat &img); 

/*===	public function prototypes	===*/

/*===	static function prototypes	===*/

/*===	static typedef variable		===*/

/*===	static variable		===*/
CRecoder m_recoder;																// 動画保存用ｵﾌﾞｼﾞｪｸﾄ
CInference m_inference;															// 推論用ｵﾌﾞｼﾞｪｸﾄ
TransForm m_trans0, m_trans1, m_trans2;											// 視点変換用ｵﾌﾞｼﾞｪｸﾄ

cv::Mat m_black_image(IMAGE_HEIGHT_WIDE, IMAGE_WIDTH_WIDE, CV_8UC3, cv::Scalar::all(0));

bool m_debugena_flg;															// デバッグモード有効フラグ
bool m_drawbbox_flg;															// 物体認識BBOX表示フラグ
bool m_drawarea_flg;															// エリア表示フラグ
bool m_drawfps_flg;																// FPS表示フラグ

bool g_img_flag;
/*===	constant variable	===*/
const std::vector<std::string> m_coco_class_names = {
    "unlabeled",
    "person",        "bicycle",       "car",           "motorbike",
    "aeroplane",     "bus",           "train",         "truck",
    "boat",          "traffic light", "fire hydrant",  "stop sign",
    "parking meter", "bench",         "bird",          "cat",
    "dog",           "horse",         "sheep",         "cow",
    "elephant",      "bear",          "zebra",         "giraffe",
    "backpack",      "umbrella",      "handbag",       "tie",
    "suitcase",      "frisbee",       "skis",          "snowboard",
    "sports ball",   "kite",          "baseball bat",  "baseball glove",
    "skateboard",    "surfboard",     "tennis racket", "bottle",
    "wine glass",    "cup",           "fork",          "knife",
    "spoon",         "bowl",          "banana",        "apple",
    "sandwich",      "orange",        "broccoli",      "carrot",
    "hot dog",       "pizza",         "donut",         "cake",
    "chair",         "sofa",          "pottedplant",   "bed",
    "diningtable",   "toilet",        "tvmonitor",     "laptop",
    "mouse",         "remote",        "keyboard",      "cell phone",
    "microwave",     "oven",          "toaster",       "sink",
    "refrigerator",  "book",          "clock",         "vase",
    "scissors",      "teddy bear",    "hair drier",    "toothbrush",
};

const cv::Scalar m_danger_color = cv::Scalar(0,255,255);
const cv::Scalar m_detect_color = cv::Scalar(0,255,0);
const cv::Scalar m_normal_color = cv::Scalar(255,0,0);
const cv::Scalar m_point_color = cv::Scalar(0,0,255);


/*===	global variable		===*/


//--------------------------------------------------
//! @brief 温度異常確認
//!
//! ｴﾗｰ情報の保存まで行う
//--------------------------------------------------
void CheckTemp(void) {
	// 温度情報取得
	const auto [zone0temp, zone1temp] = GetInternalTemp();
	const auto externalTemp = GetExternalTemp();

	float hailots0_temp = 0.0;															//
	float hailots1_temp = 0.0;															//

	// 温度異常確認
	if(g_usInferEna){
		auto [ts0temp,ts1temp] = m_inference.GetHailoTemp();
		hailots0_temp = ts0temp;
		hailots1_temp = ts1temp;
	}

	// 温度取得失敗時は異常確認できない
	if(g_errorManager.IsErrorOr(
		{ErrorCode::GetInternalTempZone0,
		ErrorCode::GetInternalTempZone1,
		ErrorCode::GetExternalTemp})){

		ERR("Core Temp Get Err");
		errorinfo.err2.os = 3;													// OS異常(温度取得失敗)
		// ｴﾗｰ保存
		error_info_save(errorinfo);
		error_info_send_req();

		return;
	}

	// 環境試験用ﾛｸﾞ出力
	if(g_usTempMoni){
		DEBUG_LOG("[TEMP]%5.2f,%5.2f,%5.2f,%5.2f,%5.2f", zone0temp, zone1temp, externalTemp, hailots0_temp, hailots1_temp);
	}

	// どれか1つでも閾値を超えていれば温度異常
	if(static_cast<float>(max({zone0temp, zone1temp, externalTemp, hailots0_temp, hailots1_temp})) >= g_fCoreTempThresh){
		if( (errorinfo.err2.imgprc_temp & 0x02) == 0x00 ){						// 未検知状態
			ERR("Core Temp Rise Over Thermal0:%5.2f Thermal1:%5.2f External:%5.2f Hailots0:%5.2f Hailots1:%5.2f",zone0temp, zone1temp, externalTemp, hailots0_temp, hailots1_temp);
			errorinfo.err2.imgprc_temp = 3;										// 画像処理ﾌﾟﾛｾｯｻ温度異常
			// ｴﾗｰ保存
			error_info_save(errorinfo);
			sync_exec();
			error_info_send_req();
		}
	}else{
		if( (errorinfo.err2.imgprc_temp & 0x02) == 0x02 ){						// 異常検知解除
			LOG(LOG_COMMON_OUT,"Core Temp Rise Over return to Normal");
			errorinfo.err2.imgprc_temp = 1;										// 画像処理ﾌﾟﾛｾｯｻ温度異常
			sync_exec();
			error_info_send_req();
		}
	}
}


/*-------------------------------------------------------------------
  FUNCTION :	InitHailo
  EFFECTS  :	Hailoの初期化
  NOTE	   :
  -------------------------------------------------------------------*/
bool InitHailo(void)
{
	string init_fail_str = "";				// 初期化失敗時の表示コメント
	bool ret = true;

	if(g_usInferEna){
		const auto& inferenceTypeData = g_configData.GetInferenceTypeData(static_cast<eInferenceType>(0));
		CInference::InitParameter iparam;

		iparam.m_modelFile = inferenceTypeData.m_modelPath;
		iparam.m_modelBackupPath = inferenceTypeData.m_modelBackupPath;
		iparam.m_scoreThreshold = inferenceTypeData.m_scoreThreshold;
		iparam.m_nmsThreshold = inferenceTypeData.m_nmsThreshold;
		iparam.m_targetLabels = vector<int>{1};					// 人物のみ検知
		iparam.m_detectSetting = inferenceTypeData.m_detectSetting;
		const int infInitRtn = m_inference.Init(iparam);
		if(infInitRtn){
			if(infInitRtn == -1){											// 推論ｽﾚｯﾄﾞ生成の失敗
				errorinfo.err2.application = 3;								// ｱﾌﾟﾘｹｰｼｮﾝ異常
				init_fail_str = "create inf thread err.";					// ｽﾚｯﾄﾞ生成の失敗
				error_info_save(errorinfo);
			}
			else if(infInitRtn == 1){										// weightエラー
				errorinfo.err2.application = 3;								// ｱﾌﾟﾘｹｰｼｮﾝ異常
				init_fail_str = "weight err, chk model.";					// modelﾃﾞｰﾀの確認
				error_info_save(errorinfo);
			}
			else if(infInitRtn == 2){										// ﾃﾞﾊﾞｲｽエラー
				errorinfo.err2.os = 3;										// OS異常
				init_fail_str = "dev err, reboot...";
			}
			else if(infInitRtn == 3){										// GyrfalconInitエラー
				errorinfo.err2.imgprc = 3;									// 画像処理ﾌﾟﾛｾｯｻ異常
				init_fail_str = "init err, reboot...";
			}
			ERR("Network Initialize failed. %s", init_fail_str.c_str());
			ret = false;
		}
	}
	return ret;
}


/*-------------------------------------------------------------------
  FUNCTION :	InitRecoder
  EFFECTS  :	録画の初期化
  NOTE	   :
  -------------------------------------------------------------------*/
bool InitRecorder(void)
{
	bool ret = true;

	if(g_usRecoderEna){
		CRecoder::InitParameter rparam;
		char fname[100];
		sprintf(fname,"%s%02d",ADD_FILE_NAME,1);
		rparam.m_addName = fname;
		rparam.m_recode_minutes = ( g_usRecoderMinutes > 0 ) ? g_usRecoderMinutes : 10;
		rparam.m_recode_fps = ( g_usRecoderFps > 0) ? g_usRecoderFps : 1;
		ret = m_recoder.Init(rparam);
	}
	return ret;
}


/*-------------------------------------------------------------------
  FUNCTION :	InitSDcard
  EFFECTS  :	SDカードの初期化
  NOTE	   :
  -------------------------------------------------------------------*/
bool InitSDcard(void)
{
	if(CheckSSDMount()){
		g_sd_mount = 1;
	}
	else{
		g_sd_mount = 0;
		errorinfo.err3.data_save = 3;
		// ｴﾗｰ保存
		error_info_save(errorinfo);
		error_info_send_req();
	}
	return true; // SDカードなくても良しとする
}


/*-------------------------------------------------------------------
  FUNCTION :	InitRTC
  EFFECTS  :	RTCの初期設定
  NOTE	   :
  -------------------------------------------------------------------*/
bool InitRTC(void)
{
	if(IsCompleteInitRTC() == false){
		errorinfo.err3.rtc = 3;													// RTC異常とする
		error_info_save(errorinfo);
	}
	else {																		// RTC初期設定ずみであれば
		CheckRTC(true);															// 電池残量等の異常チェック
	}
	return true;	// RTCが未設定でも良しとする
}

/*-------------------------------------------------------------------
  FUNCTION :	SetupDevices
  EFFECTS  :	周辺デバイスの初期化
  NOTE	   :
  -------------------------------------------------------------------*/
bool SetupDevices(void)
{
	// FPGAﾘｾｯﾄ解除
	if (StartUpFPGA() == false)
		return false;
	// 拡張IO
	if (I2cSelect_Ext(1) == false)
		return false;
	// Hailo
	if (InitHailo() == false)
		return false;
	// SDカード
	if (InitSDcard() == false)
		return false;
	// 録画モジュール
	if (InitRecorder() == false)
		return false;

	// RTC設定ﾁｪｯｸ
	if (InitRTC() == false)
		return false;

	return true;
}

/*-------------------------------------------------------------------
  FUNCTION :	OutputParam
  EFFECTS  :	パラメータおよびバージョン情報出力
  NOTE	   :
  -------------------------------------------------------------------*/
void OutputParam(void)
{
	std::string log_fpga_ver;
	char	s[128];

	// バージョン情報収集
	// アプリケーション
	g_app_ver = VERSION;
	// FPGA
	GetFPGAVersion(&g_fpga_ver, &log_fpga_ver);
	// 学習データ
	if(errorinfo.err2.application == 3){
		g_train_model_ver = "--.--.--";
	}
	else{
		g_train_model_ver = m_inference.GetDataVersion();
	}

	// 出力
	sprintf(s, "================ %s ================", PRODUCT);
	STD_LOG("%s", s);
	sprintf(s, " Software       Ver.%s %s %s",
			VERSION, RELEASE, RELEASE1);
	STD_LOG("%s", s);

	sprintf(s, " Hardware       Ver.%s", log_fpga_ver.c_str());
	STD_LOG("%s", s);

	sprintf(s, " Model          Ver.%s", g_train_model_ver.c_str());
	STD_LOG("%s", s);

	sprintf(s, " CODE No.       %s (%s)", TDN_VER, TDN_REV);
	STD_LOG("%s", s);
	STD_LOG("%s", "--------------------------------------------");
}


/*-------------------------------------------------------------------
  FUNCTION :	ChangeAppStateToInitialized
  EFFECTS  :	初期化完了の通知
  NOTE	   :
  -------------------------------------------------------------------*/
void ChangeAppStateToInitialized(void)
{
	g_app_state = APPSTATE_APP_STARTUP;
	som_status_send_req(g_app_state, g_gb_mode, UPDSTATE_NONE);
	OutputAppWakeup(true);
	error_info_send_req();														// ｴﾗｰ情報通知伝文送信要求
	waittime_info_send_req(g_usAccOffTimer);									// 待機時間設定通知伝文送信要求
}


/*-------------------------------------------------------------------
  FUNCTION :	LoadAreaMap()
  EFFECTS  :	エリアマップの読み込み
  NOTE	 :
  -------------------------------------------------------------------*/
void LoadAreaMap(void)
{
	FILE *pFile;

	//read binary
	pFile = fopen(g_areabinFilePath.c_str(), "rb");
	if(fread(g_areadata, sizeof(g_areadata),1, pFile) < 1) exit(EXIT_FAILURE);
    fclose(pFile);

}


/*-------------------------------------------------------------------
  FUNCTION :	JudgeByDetectedPosition()
  EFFECTS  :	検出位置から検出結果を精査
  NOTE	   :
-------------------------------------------------------------------*/
int JudgeByDetectedPosition(vector<BboxInfo> &boxinfo, vector<BboxJudge> &boxjudge)
{
	int i;
	cv::Point bp;
	int judge, judge_info,area_judge;
	//int w_flg;
	BboxJudge bjudge;

	//int offset = 0;

	boxjudge.clear();															// 結果格納用のBBOX情報初期化
	judge_info = 0;																// 判定情報初期化

	for( i = 0; i < (int)boxinfo.size(); i++ ){									// 検出したBBOX分ﾙｰﾌﾟ
#if 0
		judge = 0;																// 判定情報初期化									// Initialize judgment information
		w_flg = boxinfo[i].flg;													// 前側方 or 後側方の情報取得 						// Acquisition of front-side or rear-side information
		if(w_flg == 1) offset = FISHEYE_RCOL_OFFSET;							// detect_imageの切り出しオフセットを加味			// Add crop offset of detect_image

		bp.x = (boxinfo[i].x2 + boxinfo[i].x1)/2 + offset;						// BBOX下辺中心X座標
		bp.y = boxinfo[i].y2 + FISHEYE_ROW_OFFSET;								// BBOX下辺中心Y座標
		// g_areadata is a cv::scalar class used for color data   cv::Scalar s(0,0 255)
		area_judge = g_areadata[bp.y][bp.x][0];									// BBOX下端中央の座標からエリア判定値を取得
	#endif
		judge = YOLO_PERSON_DANGER_JUDGE;
		area_judge = YOLO_PERSON_DANGER_JUDGE;
		judge |= area_judge;													// 各BBOXの情報を更新							// update each bbox information
		judge_info |= area_judge;												// 画像全体の検出情報を更新						// update detection information for the entire image

		bjudge.judge = judge;													//update judge  information 
		bjudge.box = boxinfo[i];												// BBOX判定情報へBBOX設定
		boxjudge.push_back(bjudge);												// BBOX判定情報ﾘｽﾄへ追加
	}
	boxinfo.clear();															// BBOX情報をｸﾘｱ

	return(judge_info);

}


/*-------------------------------------------------------------------
  FUNCTION :	FilteringJudgmentByVehicleSignal()
  EFFECTS  :	車両情報を加味して最終結果を判定
  NOTE	   :　　struct T_CAN_RECV_DATA T_CAN_RECV_DATA;
-------------------------------------------------------------------*/
int FilteringJudgmentByVehicleSignal(int detect_state)
{
	if (g_CanRecvData.left==0) {												// if there is no lft turning signal
	    detect_state &= ~(YOLO_PERSON_DETECT_JUDGE);// 0b 11111101 =0xfe		// 警報のみにマスク（注意は発報しない）  //mask only for alarms (no warning are issued)
	}
	//車速情報
	if((g_CanRecvData.vehicle_speed == 0x00) ||									// at stationary position  there is no detection of object
	   (g_CanRecvData.adjust == 0x01)){
		detect_state |= YOLO_DETECTION_STOP;
	}
	return detect_state;
}


/*-------------------------------------------------------------------
  FUNCTION :	UpdateRecorder()
  EFFECTS  :	録画画像を録画モジュールへ渡す
  NOTE	   :	レコーダモジュール側で捨てながらFPSを調整する
  -------------------------------------------------------------------*/
void UpdateRecorder(cv::Mat &img, vector<BboxJudge> &bbox)
{
	if(g_usRecoderEna){															// 録画が有効で
		if(g_sd_mount){															// SDカードがマウント済みであれば
			m_recoder.AddFrame(img, 0, 0, bbox);								// 画像を追加
			if( m_recoder.IsErrorOccurred() ){									// 異常発生時は
				errorinfo.err3.data_save = 3;									// 異常を通知
				error_info_save(errorinfo);
				error_info_send_req();
			}
			else{																// 正常に保存出来たのに
				if( errorinfo.err3.data_save == 3 ){							// 過去に異常があれば
					errorinfo.err3.data_save = 1;								// 異常解除
					error_info_send_req();
				}
			}
		}
	}
}


/*-------------------------------------------------------------------
  FUNCTION :	JudgeErrorcode()
  EFFECTS  :
  NOTE	 :
  -------------------------------------------------------------------*/
std::string JudgeErrorcode(void)
{
	std::string result = "";

	if(errordetail.err8.vcu_disp == 1) result = "E203";

	if(errordetail.err8.mfd_disp == 1) result = "E202";

	if(errordetail.err7.psv_ch0 == 1) result = "E201";

	if(errordetail.err7.bof_ch0 == 1) result = "E200";

	if(errordetail.err8.cap_stat == 1) result = "E100";

	if(g_wdt_occerd)                   result = "E007";

	return result;

}


/*-------------------------------------------------------------------
  FUNCTION :	OverlayErrorCodeImage()
  EFFECTS  :	エラーコードの画像表示
  NOTE	 :
  -------------------------------------------------------------------*/
void OverlayErrorCodeImage(cv::Mat &overlay, string can_errorcode)
{
	int image_w = overlay.cols;
	int image_h = overlay.rows;
	int x0,x1,y0,y1,x2,y2;
	const int fontFace = cv::FONT_HERSHEY_SIMPLEX;

	x0 = image_w / 2 - 50;
	y0 = image_h / 2 - 50;
	x1 = image_w / 2 + 50;
	y1 = image_h / 2 + 50;
	x2 = image_w / 2 - 80;
	y2 = image_h / 2 + 150;

	cv::rectangle(overlay, cv::Point(x0,y0), cv::Point(x1, y1), cv::Scalar(0,0,255), 8, 8);					// ERRORBOX描画
	cv::line(overlay, cv::Point(x0,y0), cv::Point(x1, y1), cv::Scalar(0,0,255), 2, 8);						// ERRORBOX描画
	cv::line(overlay, cv::Point(x1,y0), cv::Point(x0, y1), cv::Scalar(0,0,255), 2, 8);						// ERRORBOX描画
	cv::putText(overlay, can_errorcode, cv::Point(x2, y2), fontFace, 2, cv::Scalar(0, 0, 255), 8);
}


/*-------------------------------------------------------------------
  FUNCTION :	DrawErrorCode()
  EFFECTS  :	エラーコード表示処理
  NOTE	 :
  -------------------------------------------------------------------*/
void DrawErrorCode(cv::Mat *out_img0,cv::Mat *out_img1)
{
	std::string can_errorcode;

	can_errorcode = JudgeErrorcode();
	if(can_errorcode != ""){
		OverlayErrorCodeImage(*out_img0, can_errorcode);
		OverlayErrorCodeImage(*out_img1, can_errorcode);
	}
}



/*-------------------------------------------------------------------
  FUNCTION :	DrawAreaMap()
  EFFECTS  :	エリアマップ描画
  NOTE	 :      エリアマップをオーバーレイ表示する . this function define R,G,B channel filled with zeros and set the color and addweight the image 
  -------------------------------------------------------------------*/
void DrawAreaMap(cv::Mat *frame)
{
    // area_mapのサイズがframeと一致しているか確認
    if (frame->rows != IMAGE_HEIGHT_FHD || frame->cols != IMAGE_WIDTH_FHD) {
        std::cerr << "Error: frame size and area_map size do not match." << std::endl;
        return;
    }

    cv::Mat area_map(IMAGE_HEIGHT_FHD, IMAGE_WIDTH_FHD, CV_8UC1, g_areadata);

    // オーバーレイ用のマスク画像を作成
    cv::Mat red_channel = cv::Mat::zeros(frame->rows, frame->cols, CV_8UC1);   // 行と列を明示的に指定  //explicitly specify rows and column . here matrix object are filled with zeros
    cv::Mat green_channel = cv::Mat::zeros(frame->rows, frame->cols, CV_8UC1); // 行と列を明示的に指定
    cv::Mat blue_channel = cv::Mat::zeros(frame->rows, frame->cols, CV_8UC1);  // 行と列を明示的に指定

    // エリア設定値によって色を設定 (0: 完全透過, 1: 黄色, 2: 赤) 		Set color according to area setting value (0: fully transparent, 1: yellow, 2: red)
    for (int y = 0; y < area_map.rows; ++y) {
        for (int x = 0; x < area_map.cols; ++x) {
            if (y >= red_channel.rows || x >= red_channel.cols) {
                std::cerr << "Error: area_map index out of bounds." <<
					" area_map:" << area_map.size() << "red_channel:" << red_channel.size() << std::endl;
                return;
            }
            uint8_t value = area_map.at<uint8_t>(y, x);
			if (value == YOLO_PERSON_DETECT_JUDGE) {
                red_channel.at<uint8_t>(y, x) = 255;
                green_channel.at<uint8_t>(y, x) = 255;
            }
            else if (value == YOLO_PERSON_DANGER_JUDGE) {
                red_channel.at<uint8_t>(y, x) = 255;
            }
        }
    }

    // チャンネルをマージしてマスク画像を作成
    std::vector<cv::Mat> mask_map = { blue_channel, green_channel, red_channel };
    cv::Mat mask_img;
    cv::merge(mask_map, mask_img);

    // フレームに重畳表示
    cv::Mat base_img = frame->clone();
    cv::addWeighted(base_img, 1.0, mask_img, 0.3, 0.0, *frame);
}


/*-------------------------------------------------------------------
	FUNCTION :	DrawGuideLine()
	EFFECTS  :	bmp画像からガイド線生成
	NOTE	 :	取り込んだ画像からガイド線を生成する
-------------------------------------------------------------------*/
int DrawGuideLine(cv::Mat &img)
{
#if 0
    vector<vector<cv::Point>> contours,approx;
	vector<cv::Vec4i> hierarchy;
	cv::Mat src = cv::imread(g_areasourceFilePath,0);
	if(!src.data) return 0;

	// 指定した色に基づいたマスク画像の生成
	cv::findContours(src,contours,hierarchy,cv::RETR_TREE,cv::CHAIN_APPROX_SIMPLE);
	cv::polylines(img, vector<cv::Point>{contours[1][0],contours[1][20]}, false, cv::Scalar(255, 0, 0), 3);
#endif
	return 1;
}

// read 2
/*-------------------------------------------------------------------
	FUNCTION :	DrawBBox()
	EFFECTS  :	Drawing BBOX information for fisheye lenses
	NOTE	 :	魚眼レンズ用のBBOX判定情報の描画を行う  : Draws BBOX information for fisheye lenses
-------------------------------------------------------------------*/
void DrawBBox(cv::Mat &img,vector<BboxJudge> &boxjudge, const vector<string>& classNames)
{
	int i;
	int image_w = img.cols;
	int image_h = img.rows;
	int x0,x1,y0,y1;
	cv::Scalar color;
	int offset = 0;
	printf("img width = %d , height = %d \n", image_w, image_h);
	for( i = 0; i < (int)boxjudge.size(); i++ ){								// BBOX判定情報分ﾙｰﾌﾟ
		string s;
		int w_flg;

		/*  for confirmation purpose */
		printf("Draw Bbox x1:%d, y1:%d x2:%d y2:%d /n",boxjudge[i].box.x1,boxjudge[i].box.y1,boxjudge[i].box.x2,boxjudge[i].box.y2);


		w_flg = boxjudge[i].box.flg;
		if(w_flg == 0) offset = FISHEYE_FCOL_OFFSET_DETECT;
		if(w_flg == 1) offset = 0;

		if( boxjudge[i].judge & YOLO_PERSON_DANGER_JUDGE )				color = m_danger_color; // 危険判定
		else if( boxjudge[i].judge & YOLO_PERSON_DETECT_JUDGE )			color = m_detect_color; // 注意判定
		else															color = m_normal_color; // 検知
		x0 = (boxjudge[i].box.x1 + offset) * (IMAGE_WIDTH_FHD/(YOLO_IMAGE_WIDTH*2));
		x0 = (x0 > image_w - 1) ? (image_w - 1) : ((x0 < 0) ? 0 : x0);
		y0 = boxjudge[i].box.y1 * (IMAGE_HEIGHT_FHD/YOLO_IMAGE_HEIGHT);
		y0 = (y0 > image_h - 1) ? (image_h - 1) : ((y0 < 0) ? 0 : y0);
		x1 = (boxjudge[i].box.x2 + offset) * (IMAGE_WIDTH_FHD/(YOLO_IMAGE_WIDTH*2));
		x1 = (x1 > image_w - 1) ? (image_w - 1) : ((x1 < 0) ? 0 : x1);
		y1 = boxjudge[i].box.y2 * (IMAGE_HEIGHT_FHD/YOLO_IMAGE_HEIGHT);
		y1 = (y1 > image_h - 1) ? (image_h - 1) : ((y1 < 0) ? 0 : y1);

		s = classNames[boxjudge[i].box.cls_idx];
		LOG(LOG_YOLO_INFO,"Draw Bbox index:%d x1:%d, y1:%d x2:%d y2:%d",i,x0,y0,x1,y1);
		//if( boxjudge[i].judge & YOLO_PERSON_DANGER_JUDGE )	{
			cv::rectangle(img, cv::Point(x0,y0), cv::Point(x1, y1), color, 10, 8);					// BBOX描画
			cv::circle(img, cv::Point((x0 + (x1 - x0) / 2),y1), 3, m_point_color, -1, cv::LINE_AA); // 判定座標描画
		//}
		//printf("Draw Bbox index:%d x1:%d, y1:%d x2:%d y2:%d",i,x0,y0,x1,y1);
	
	}
}

/*-------------------------------------------------------------------
  FUNCTION :	DrawBBoxQuad()
  EFFECTS  :	Drawing BBOX information in fisheye image  1920*1080  
  DETAIL   :	cv::polylines() used instead of cv::rectangle() function
  				because the detection is done in applie 
-------------------------------------------------------------------*/
void DrawBBoxQuad(cv::Mat &img,vector<BboxJudge> &boxjudges, const vector<string>& classNames)
{
	//TransForm retracking;
	std::vector<cv::Point2f> remapped_conor_points;
	cv::Mat m_mapX, m_mapY;

	// calculating offset value wheather it is at rear end or front end 
	int offset = 0; 
	for(const auto& boxjudge: boxjudges){
		const auto& box_obj = boxjudge.box;
		int w_flg;

		printf("before offset (x0,y0) = (%d, %d),  (x1,y1) = (%d , %d) \n", box_obj.x1, box_obj.y1, box_obj.x2, box_obj.y2);
		w_flg = box_obj.flg;
		if(w_flg == 0)		offset = FISHEYE_FCOL_OFFSET_DETECT; 
		if(w_flg == 1)		offset = 0;


		// adding all four  rectangular point pair inside a vector for the purpose of seach of each point
		// here purpose is to draw polylines rather then curve beacuse of which all four rectangualr point
		// are necessary
		int width  = box_obj.x2 - box_obj.x1 +1 ;
		int height = box_obj.y2 - box_obj.y1 +1 ;
		//printf("416 X 416 frame point x1 = %d, y1 = %d \n",box_obj.x1, box_obj.y1);
		remapped_conor_points.emplace_back(box_obj.x1 + offset, box_obj.y1);
		remapped_conor_points.emplace_back(box_obj.x1 + width + offset, box_obj.y1);
		remapped_conor_points.emplace_back(box_obj.x2 + offset, box_obj.y2 );
		remapped_conor_points.emplace_back(box_obj.x1 + offset, box_obj.y1 + height);
	}
	// here vector data is written in a row wise style . please be careful don't take this as mistake
	printf("++++++++++++++++++++++++++++++points after reverse affain tranformation.\n");
	for(const auto& point : remapped_conor_points){
		printf("after offset (x,y) = (%f, %f)\n", point.x, point.y);
	}
	std::vector<cv::Point2f> quadbbox_sets = m_trans0.ReverseAffainConvert(remapped_conor_points);
	printf("------------------------------points after reverse affain tranformation.\n");
	for(const auto& point : quadbbox_sets){
		printf(" (x,y) = (%f, %f)\n", point.x, point.y);
	}

	//converting cv::Point2f coordinates into point for polyline function
	std::vector<cv::Point2f> quad_points;
	for(const auto& quadbbox_set : quadbbox_sets){
			int x_axis = std::round(quadbbox_set.x);
			int y_axis = std::round(quadbbox_set.y);
			quad_points.push_back(cv::Point(x_axis , y_axis));
			//printf("quad x_axis = %d, y = %d \n", x_axis , y_axis);
	}
	for (size_t i = 0; i < quad_points.size(); i+=4){
		//making four pair points for polylines
		std::vector<cv::Point> quad_edge = { quad_points[i], quad_points[i+1], quad_points[i+2], quad_points[i+3] };
	
		//drawing 4 side polygon in original image
		cv::polylines(img, quad_edge, true, cv::Scalar(0,0,255), THICKNESS);	
	}

	//drawing 4 side polygon in original image
	//cv::polylines(img, quad_edge, true, cv::Scalar(0,0,255), 2);
	cv::imwrite("polylines.jpg", img);
}

/*-------------------------------------------------------------------
  FUNCTION :	DrawPerformance()
  EFFECTS  :	パフォーマンス情報描画
  NOTE	 :
  -------------------------------------------------------------------*/
void DrawPerformance(cv::Mat &img,float fps1,float fps2)
{
	const int fontFace = cv::FONT_HERSHEY_SIMPLEX;
	const double fontScale = 0.5;
	const int thickness = 2;
	char fps_str[100];
	string str;

	sprintf(fps_str,"%3.3fFPS",fps1);
	str = fps_str;
	cv::putText(img, str, cv::Point(30, 30), fontFace, fontScale, cv::Scalar(0, 0, 255), thickness);
	sprintf(fps_str,"%3.3fFPS",fps2);
	str = fps_str;
	cv::putText(img, str, cv::Point(30, 70), fontFace, fontScale, cv::Scalar(0, 0, 255), thickness);
}


/*-------------------------------------------------------------------
  FUNCTION :	OverlayOnOutputVideo
  EFFECTS  :	色々と重畳表示
  NOTE	   :	- 検知枠
				- 調整用ガイドライン
				- 検知エリア（デバッグ）
				- パフォーマンス（デバッグ）
  -------------------------------------------------------------------*/
void OverlayOnOutputVideo(cv::Mat &cap_img, vector<BboxJudge> &bbox) {

	// エリア表示
	if (m_drawarea_flg) {
		DrawAreaMap(&cap_img);
	}
	// BBOX判定情報描画
	if (m_drawbbox_flg) {
		//DrawBBox(cap_img, bbox, m_coco_class_names);
		DrawBBoxQuad(cap_img, bbox, m_coco_class_names);
	}
	// パフォーマンス情報描画
	if (m_drawfps_flg) {
		DrawPerformance(cap_img, m_inference.GetinferencePerformance(),
						 m_inference.GetinferencePerformance());
	}
	// 調整モード
	if (g_CanRecvData.adjust) {
		DrawGuideLine(cap_img);
	}
}


/*-------------------------------------------------------------------
  FUNCTION :	TransformFishEyeImage
  EFFECTS  :	視点変換処理
  -------------------------------------------------------------------*/
void TransformFishEyeImage(cv::Mat &in_img, cv::Mat *out_img0,cv::Mat *out_img1)
{
	static 	uchar old_lut[MONITOR_MAX] = {0xFF, 0xFF};
	uchar lut1,lut2;

#if 1
	// 最新のLUT情報取得
	lut1 = 0;
	lut2 = 3;
#else
	int pto;
	int fid;

	if (g_CanRecvData.mfd_button_state_recv) {									// MDFから表示指示を受信済みであれば
		if (g_CanRecvData.mfd_change_favorite != 0) {							// お気に入り編集中の場合
			fid = g_CanRecvData.mfd_change_favorite;
			pto = g_CanRecvData.mfd_change_pto;
			// お気に入り変更ありの場合
			if (g_MfdButtonCtrl.monitor1.exchange(false))  {					// 出力1の変更要求の場合
				g_lut_favorite_table[pto][fid][0] =								// 変更先のLUTを算出
					(g_lut_favorite_table[pto][fid][0] + 1) % LUT_DATA_MAX;
			}
			if (g_MfdButtonCtrl.monitor2.exchange(false))  {					// 出力1の変更要求の場合
				g_lut_favorite_table[pto][fid][1] =								// 変更先のLUTを算出
					(g_lut_favorite_table[pto][fid][1] + 1) % LUT_DATA_MAX;
			}
			lut1 = g_lut_favorite_table[pto][fid][0];
			lut2 = g_lut_favorite_table[pto][fid][1];
		}
		else {																	// 通常表示の場合
			pto = g_CanRecvData.pto;											// 現状のPTO状態ラッチ
			if (pto) fid = g_CanRecvData.mfd_favorite1;							// お気に入り番号取得 モニタ1
			else	 fid = g_CanRecvData.mfd_favorite0;							// お気に入り番号取得 モニタ2
			lut1 = g_lut_favorite_table[pto][fid][0];
			lut2 = g_lut_favorite_table[pto][fid][1];
		}
	}
	else {
		lut1 = 0xFF;
		lut2 = 0xFF;
	}
	// LUT変更の確定処理
	if (g_MfdButtonCtrl.cancel_req.exchange(false))  {							// LUTのお気に入りキャンセル要求の場合
		ReadFavariteLUTConf();													// 元のLUTに戻す
		g_MfdButtonCtrl.ack_req.store(true);									// ACK要求
		LOG(LOG_COMMON_OUT, "Editting of the favorite LUTs is canceled.");
	}
	else if (g_MfdButtonCtrl.save_req.exchange(false))  {						// LUTのお気に入り保存要求の場合
		UpdateConfigFileforLutFavoriteTable();									// LUTの保存処理
		g_MfdButtonCtrl.ack_req.store(true);									// ACK要求
		LOG(LOG_COMMON_OUT, "Update Favorite LUTs.");
	}

#endif
	// LUTの変更確認
	if(lut1 != old_lut[0]) {
		m_trans1.Init(g_LutConfig[lut1][0].filename, false);    				// filename is a string where
		old_lut[0] = lut1;
	}
	if(lut2 != old_lut[1]) {
		m_trans2.Init(g_LutConfig[lut2][1].filename, false);
		old_lut[1] = lut2;
	}
	// 描画
	if (lut1 < LUT_DATA_MAX){
		m_trans1.AffainConversion(in_img, out_img0);
		printf(" m_trans1 object affain conversion is taking place.\n");
	}
	else					 cv::resize(m_black_image, *out_img0, cv::Size(IMAGE_WIDTH_WIDE, IMAGE_HEIGHT_WIDE));
	if (lut2 < LUT_DATA_MAX){
		m_trans2.AffainConversion(in_img, out_img1);
		printf(" ______m_trans2 object affain conversion is taking place.\n");
	} 
	else					 cv::resize(m_black_image, *out_img1, cv::Size(IMAGE_WIDTH_WIDE, IMAGE_HEIGHT_WIDE));
}


/*-------------------------------------------------------------------
  FUNCTION :	SetFrameBuffer()
  EFFECTS  :	OS上のフレームバッファに出力映像を格納
  NOTE	   :
  -------------------------------------------------------------------*/
bool SetFrameBuffer(cv::Mat &out_img0, cv::Mat &out_img1)
{
	//	cv::TickMeter meter0;

	bool ret = true;
	if (set_image_disp0(out_img0) == false) {
		ret = false;
		LOG(LOG_COMMON_OUT, "failed to set image disp0");
	}
	if (set_image_disp1(out_img1) == false) {
		ret = false;
		LOG(LOG_COMMON_OUT, "failed to set image disp1");
	}

	// meter0.stop();
	// meter0.start();
	// if (meter0.getCounter () > 100 ) {
	// 	std::cout << "avg time : " << meter0.getAvgTimeMilli() << std::endl;
	// 	meter0.reset();
	// }
	return ret;
}


/*-------------------------------------------------------------------
  FUNCTION :	DebugFunction()
  EFFECTS  :	各種デバッグ機能の処理
  NOTE	   :
  -------------------------------------------------------------------*/
bool DebugFunction(int key, cv::Mat &cap_image)
{
	static bool servival_flg = true;											// 生存応答有効ﾌﾗｸﾞ
	bool quite_request = true;

	if( key == 'q'){
		quite_request = false;														// ｱﾌﾟﾘ終了
		LOG(LOG_COMMON_OUT, "escape Q");
	}
	else if( key == 'C'){
		if(servival_flg == false){
			servival_flg = true;
			LOG(LOG_COMMON_OUT,"Servival Check Enable");
		}
		else{
			servival_flg = false;
			LOG(LOG_COMMON_OUT,"Servival Check Disable");
		}
		servival_setting_send_req(servival_flg);
	}
	else if(m_debugena_flg ){
		if( key == 'e' ){
			m_drawarea_flg = !m_drawarea_flg;
		}
		else if( key == 'j' ){
			m_drawbbox_flg = !m_drawbbox_flg;
		}
		else if( key == 'p'){
			m_drawfps_flg = !m_drawfps_flg;
		}
		else if( key == 'S' ){
			cv::Mat ss = cap_image;
			cv::imwrite("screenshot.png", ss);									// 実行フォルダ下にPNGフォーマットで上書き保存
		}
		else if( key == 'I'){
			// 温度取得失敗時のOS異常発生の検証用
			DeleteExternalTempPath();											// 温度ｾﾝｻｰの取得パスを削除
		}
	}
	return quite_request;
}


/*-------------------------------------------------------------------
  FUNCTION :	Timer1secFunction()
  EFFECTS  :	1秒周期の処理
  NOTE	   :
  -------------------------------------------------------------------*/
int Timer1secFunction(void)
{
	static 	ulong elapsedTimeAfterBoot = 0;				// 起動後の経過時間（秒）
	static 	int runtime_counter = 0;					// 駆動時間
	int exit_request = 0;

	if(g_checktimer_flg2){	// 時間が掛かるメモリ監視を分割して負荷を平滑化
		g_checktimer_flg2 = 0;

		// 温度チェック
		CheckTemp();
		// レコーダ空き容量チェック
		if((runtime_counter % 60)==0){											// 60秒毎に確認
			if(g_sd_mount){
				if(SSDSecureFreeSpace(SSD_DEL_CHECK_FREE_SIZE)==0){
					create_remove_thread();
				}
			}
		}

		if(g_remove_comp){
			delete_remove_thread();						// ｽﾅｯﾌﾟｼｮｯﾄ保存ｽﾚｯﾄﾞ削除
			g_remove_comp = 0;
			g_remove_req = 0;
			if( g_remove_result != 0 ){
				errorinfo.err3.data_save = 3;			// ﾃﾞｰﾀ保存異常
				// ｴﾗｰ保存
				error_info_save(errorinfo);
				error_info_send_req();
			}
			else{
				if( errorinfo.err3.data_save == 3 ){
					errorinfo.err3.data_save = 1;			// ﾃﾞｰﾀ保存異常解除
					error_info_send_req();
				}
			}
		}

#ifdef SD_LOG_ENABLE
		// 調査用ログ
		if(g_sd_mount){
			GetFPGAStatus();
			get_tp2912_status();
			get_exio_status();
			logStatus();
		}
#endif
	}
	else if(g_checktimer_flg) {
		g_checktimer_flg = 0;

		g_survival_cnt++;
		// 起動後の経過時間を計測
		++elapsedTimeAfterBoot;
		runtime_counter++;

		// 自動再起動の時間を満了すればﾌﾗｸﾞを立てる
		if(const auto rebootSec = g_configData.GetDebugAutoRebootSeconds();
		   rebootSec != 0 && elapsedTimeAfterBoot >= rebootSec){
			exit_request = 2;
			LOG(LOG_COMMON_OUT, "Auto reboot Timing");
		}

		// ｼｬｯﾄﾀﾞｳﾝ要求確認
		if(IsShutdownReq() && g_errorManager.IsError(ErrorCode::IsShutdownReq)==false){
			exit_request = 1;
			LOG(LOG_COMMON_OUT, "Shutdown");
		}

		// 生存確認伝文受信確認
		if( g_survival_cnt >= 5 && g_survival_flg == 0){										// 生存確認ﾀｲﾑｱｯﾌﾟ
			g_survival_flg = 1;
			ERR("BaseBoard Survival Req Timeout!");
		}

		if(runtime_counter >= 60*60*24){									// 1日経過でｸﾘｱ
			runtime_counter = 0;
		}
	}
	return exit_request;
}


/*-------------------------------------------------------------------
	FUNCTION :	detect_main()
	EFFECTS  :	危険動作検知ﾒｲﾝﾙｰﾌﾟ関数
	NOTE	 :	危険動作判定ｱﾌﾟﾘﾒｲﾝ処理
-------------------------------------------------------------------*/
int detect_main(void)
{
	struct stat st;							// ﾃﾞｨﾚｸﾄﾘ/ﾌｧｲﾙ属性
	int key;								// ｷｰﾎﾞｰﾄﾞからの入力ｷｰ
	int exit_code = -1;						// 終了ﾌﾗｸﾞ
	#if !TESTING_MODE
	int tmp_detect_state;
	#endif
	int timer_status;
	bool ret;

	CaptureResult capture_result;			// 画像取得結果
	vector<BboxJudge> boxjudge;				// BBOX判定情報 Vector
	cv::Mat cap_image;						//
	cv::Mat front_image;					//
	cv::Mat rear_image;						//
	cv::Mat rec_image;						//
	cv::Mat detect_image;					//
	cv::Mat disp0_image;					// monitor 1 display
	cv::Mat disp1_image;					// monitor 2 display

	g_capcomp = 0;
	g_remove_req = 0;						// 最古日付ﾌｫﾙﾀﾞ削除要求初期化		// Oldest date folder deletion request initialization
	g_remove_comp = 0;						// 最古日付ﾌｫﾙﾀﾞ削除完了初期化 		// Delete the oldst folder and initialize

	g_img_flag = true;							// 

	if(stat("./DebugEna", &st) == 0){		// Debug機能有効				 // Debug function enabled
		LOG(LOG_COMMON_OUT,"DebugEnable");
		m_debugena_flg = true;
	}
	else{
		m_debugena_flg = false;
	}

	// 周辺デバイスの初期化
	if (SetupDevices() == false) {
		ERR("Devices setup failed!!");
		sync_exec();
	}

	// バージョン取得 & 表示
	OutputParam();

	// ローカル変数の初期化
	m_drawbbox_flg = true;					// 物体認識論結果ﾌﾗｸﾞ初期化
	m_drawarea_flg = false;					// ｴﾘｱ表示ﾌﾗｸﾞ初期化
	m_drawfps_flg = false;					// FPS表示ﾌﾗｸﾞ初期化
	std::vector<float> boxes;				// BBOXｵﾌﾞｼﾞｪｸﾄ Vector

	// 検知系の初期化
	g_DetectState = 0;
	LoadAreaMap();
	m_trans0.Init(g_LutConfigDETECT[0].filename, false);			//TransForm m_trans0 -> it is object

	// RL78へ初期化完了の通知
	ChangeAppStateToInitialized();

	while(exit_code == -1){   			// exit_code is predefined in above case . so this loop alawys continue and value will be change by the end of program
		// WDTﾊﾟﾙｽ更新
		UpdateWDTPulse();

		// RL78のシリアルコマンド処理
		ExecRL78Command();


		/*if(g_img_flag == true)
		{
			get_image_static(cap_image);	
			capture_result = CaptureResult::Success;
		}*/
		//void  TestColorBand(cv::Mat frame)  calling this function out
		// TestColorBand(cap_image);
		// capture_result = CaptureResult::Success;
		
#if VIDEO_FLAG
		// 画像取得
		if(g_capcomp == 1) {													// 次のフレーム画像を取得済みであれば		// reading next frame for image retrival
			capture_result = get_image_cap0(cap_image);							// 画像取得	
		}
		else {																	// 次のフレーム待ちであれば
			usleep(1000);														// 少し休憩
			continue;
		}
		// 取得失敗
		if(capture_result != CaptureResult::Success) {
			cap_image = m_black_image.clone();
			

		}
		// may not be needed here either just for emoving bug of unused variable
		//capture_result = CaptureResult::Success;
#else 
		get_image_static(cap_image);	
#endif

#if !TESTING_MODE
		// 推論前処理
		m_trans0.AffainConversion(cap_image, &detect_image);
		
		// read 1
		front_image  = detect_image(cv::Rect(FISHEYE_FCOL_OFFSET_DETECT,FISHEYE_ROW_OFFSET_DETECT,FISHEYE_COL_CUT_DETECT,FISHEYE_ROW_CUT_DETECT));   	//(416,0) w = h = 416
		rear_image   = detect_image(cv::Rect(FISHEYE_RCOL_OFFSET_DETECT,FISHEYE_ROW_OFFSET_DETECT,FISHEYE_COL_CUT_DETECT,FISHEYE_ROW_CUT_DETECT));		//(0,0)   w = h = 416
		resize(cap_image, rec_image, cv::Size(IMAGE_WIDTH_WIDE, IMAGE_HEIGHT_WIDE),0,0,cv::INTER_NEAREST);
		
		cv::imwrite("front_image.jpg", front_image);
		cv::imwrite("rear_image.jpg", rear_image);
		// here output image after affain conversion is used instead of source
		//resize(detect_image, rec_image, cv::Size(IMAGE_WIDTH_WIDE, IMAGE_HEIGHT_WIDE),0,0,cv::INTER_NEAREST);

		// 推論(前側方と後ろ側方の画像を同時にHailoに渡す)							//Inference (passing front-side and back-side images to Hailo simultaneously)
		m_inference.Inference_fisheye(front_image,rear_image);
		// 推論結果の判定
		bool isAppearError = m_inference.IsAppearError();						// 推論モジュールのエラー状態取得    // return false in normal and return true when condtion are not meet
		if(isAppearError == true){												// 推論モジュールに異常が発生した場合
			exit_code = 3;														// Reboot
			LOG(LOG_COMMON_OUT, "Hailo Error Occurred System Reboot");
			servival_setting_send_req(false);
			usleep(200000);
		}

		else if(m_inference.IsInferenceComplete() ){							// 推論モジュールが正常に推論完了した場合   		// always true and thread is used
			m_inference.ClearInferenceCompleteFlag();							// 完了を認識した旨を推論モジュールに通知

			vector<BboxInfo> bboxInfo;											//
			m_inference.GetBoundingBox(&bboxInfo);								// 検出結果取得									// m_boundingbox value is copied as return through bboxinfo here.
			// for(auto const& box:bboxInfo){
			// 	std::cout<<box.score <<std::endl;
			// 	std::cout << box.x1 << "\t" << box.y1 << "\t" << box.x2 << "\t" << box.y2 << "\t flag = " << box.flg << std::endl;
			// }
			tmp_detect_state = JudgeByDetectedPosition(bboxInfo, boxjudge);		// 検出結果の精査
			g_DetectState = FilteringJudgmentByVehicleSignal(tmp_detect_state);	// 車両信号を元に最終判定 						// final decison based on vechicle signal
		}

		// 出力
		if(capture_result == CaptureResult::Success) {							// 映像取得成功時のみ  // CaptureResult is a enum class where value 0
			UpdateRecorder(rec_image, boxjudge);								// 動画保存
		}
		OverlayOnOutputVideo(cap_image, boxjudge);								// 検知枠やデバッグ情報の重畳表示

#endif
		//TransformFishEyeImage(cap_image, &disp0_image, &disp1_image);			// 魚眼レンズ歪み補正 & 視点変換

		// read 1 done
#if !TESTING_MODE
		DrawErrorCode(&disp0_image, &disp1_image);								// エラーコード重畳
#endif
		get_image_static(disp0_image);
		// get_image_static(disp1_image);
		/** disp0_image  -> empty matrix where frame is passed and called at 30fps rate
		 * 	cap_image 	 -> cap_image is next frame for video which will get its value from disp0_img for passing and loop inside it
		 */

		// // for reference
		cv::imwrite("Display_frame1.bmp", disp0_image);
		cv::imwrite("Capture_output.bmp", cap_image);

		
		ret = SetFrameBuffer(cap_image, disp0_image);							// 映像出力
		getimageComparator(cap_image, disp0_image);
		if (ret == false) {														// 出力に失敗した場合
			exit_code = 0;														// アプリ終了
		}


		// 1秒周期
		timer_status = Timer1secFunction();										// 1秒周期の処理
		if (timer_status != 0) {												// Exit要求があった場合
			exit_code = timer_status;											// Exitフラグを更新
		}

		// デバッグ機能
		key = cust_getchar();													// キー入力取得
		ret = DebugFunction(key, cap_image);									// デバッグ処理
		if (ret == false) {														// Quit要求があれば
			exit_code = 0;														// アプリ終了
		}
	}

	m_recoder.Finalize();
	OutputAppWakeup(false);

	// 終了前処理
	LOG(LOG_COMMON_OUT,"END Preprocessing");
	warning_output_send_req(0);												// 警告信号出力指示伝文送信要求

	usleep(200000);															// 200mｳｪｲﾄ

	return exit_code;
}


/*-------------------------------------------------------------------
	FUNCTION :	get_image_static()
	EFFECTS  :	read a static images one at a time rather then a video
	NOTE	 :	videocapture is not　used be careful
-------------------------------------------------------------------*/
void get_image_static(cv::Mat &frame)
{
    int width = IMAGE_WIDTH_FHD;
    int height = IMAGE_HEIGHT_FHD;
    // creating a black frame
    frame = cv::Mat::zeros(height, width, CV_8UC3);
    //frame = cv::Mat(height, width, CV_8UC3, cv::Scalar(255, 0, 0));

	std::vector<cv::Scalar> colors = {
        cv::Scalar(255, 0, 0),              //blue 
		cv::Scalar(255, 0, 255),            //magenta
        cv::Scalar(0, 255, 0),              //green
        cv::Scalar(255, 255, 255),          //white
        cv::Scalar(0, 255, 255),            //yellow   
		cv::Scalar(255, 255, 0),			//cyan
        cv::Scalar(0, 0, 0),				// black
		cv::Scalar(0, 0, 255)               //red
        };
     //equal distribution of color in frame
    int strip = width / colors.size();

    //filling up the strip
    for(size_t i = 0; i < colors.size(); ++i){
        int pos1 = i * strip;
        int pos2 = pos1 + strip;
        frame(cv::Range::all(), cv::Range(pos1, pos2)) = colors[i];
    } 

	
	cv::circle(frame, cv::Point(40,40), 20, cv::Scalar(0, 0, 0), -1);
	

	
    //cv::resize(frame,frame,cv::Size(IMAGE_WIDTH_FHD,IMAGE_HEIGHT_FHD),0,0,cv::INTER_NEAREST);
	cv::resize(frame,frame,cv::Size(IMAGE_WIDTH_WIDE, IMAGE_HEIGHT_WIDE),0,0,cv::INTER_NEAREST);
}

