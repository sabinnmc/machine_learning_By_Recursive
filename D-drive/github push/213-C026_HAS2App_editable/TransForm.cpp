/********************************************************************************
;*                                                                              *
;*        Copyright (C) 2021 PAL Inc. All Rights Reserved                       *
;*                                                                              *
;********************************************************************************
;*                                                                              *
;*        開発製品名    :    HAS2                                               *
;*                                                                              *
;*        ﾓｼﾞｭｰﾙ名      :    transform.cpp                                      *
;*                                                                              *
;*        作　成　者    :    内之浦 伸治                                        *
;*                                                                              *
;********************************************************************************/

/*===    file include	 ===*/
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdint.h>

#include "TransForm.hpp"
#include "Sysdef.h"

/*===    user definition        ===*/

/*===    external variable    ===*/


/*===    external function    ===*/
extern int update_check(void);

/*===    public function prototypes    ===*/


/*===    static function prototypes    ===*/


/*===    static typedef variable        ===*/


/*===    static variable        ===*/


/*===    constant variable    ===*/


/*===    global variable        ===*/


/*===    implementation        ===*/

/*-------------------------------------------------------------------
  FUNCTION :    TransForm()
  EFFECTS  :    コンストラクタ
  NOTE     :
-------------------------------------------------------------------*/
TransForm::TransForm(void)
{
	// メンバ変数の初期化
	m_lutFileName = "";															// LUTファイル名取得
	m_subpixMode = false;														// サブピクセル補間有効/無効
	m_imgHeigh = 0;																// 変換後の画像サイズ長さ
	m_imgWidth = 0;																// 変換後の画像サイズ幅
}


/*-------------------------------------------------------------------
  FUNCTION :    ~TransForm()
  EFFECTS  :    デストラクタ
  NOTE     :
  -------------------------------------------------------------------*/
TransForm::~TransForm()
{

}


/*-------------------------------------------------------------------
  FUNCTION :    Init()
  EFFECTS  :    初期設定
  NOTE     :	LUTファイルのロード
  -------------------------------------------------------------------*/
void TransForm::Init(std::string filename, bool subpix)
{
	bool ret;
	// 初期設定
	m_lutFileName = filename;													// LUTファイル名取得
	m_subpixMode = subpix;														// サブピクセル補間有効/無効
	if(m_mapX.empty())
		printf("inside init function , m_mapX is empty . \n");
	// LUTファイルのチェック
	ret = FastLoadLUT();														// バイナリファイルとしてリード
	
	if (ret==false) {															// 正常にファイルを読み出せたら
		OutputErrorLog("[ERR] " + m_lutFileName + " bin could not find.\n");
	}
	std::cout << "LUT change to " << m_lutFileName << std::endl;
}


/*-------------------------------------------------------------------
  FUNCTION :    AffainConversion()
  EFFECTS  :    視点変換処理
  NOTE     :
  -------------------------------------------------------------------*/
void TransForm::AffainConversion(const cv::Mat &src, cv::Mat *dst)
{

	if (m_subpixMode == true) {													// サブピクセル補間が有効の場合
		cv::remap(src, *dst, m_mapX, m_mapY, cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0) );
	}
	else {																		// サブピクセル補間が無効の場合
		cv::remap(src, *dst, m_mapX, m_mapY, cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0) );
	}
}


/*-------------------------------------------------------------------
  FUNCTION :    GetDummyImage()
  EFFECTS  :    初期設定
  NOTE     :	LUTファイルのロード
  -------------------------------------------------------------------*/
void TransForm::GetDummyImage(cv::Mat *img)
{
	*img = cv::imread("data/tadano.jpg");
}


/*-------------------------------------------------------------------
  FUNCTION :     GetOutImageSize()
  EFFECTS  :    LUTファイルの存在確認および出力画像サイズの取得
  NOTE     :
-------------------------------------------------------------------*/
bool TransForm:: GetOutImageSizeFromLutFile(std::string file)
{
	std::string data;
	std::string line;
	int img_w = 0;
	int img_h = 0;

	m_lutFileName = LUT_FLDR + file;

	// ファイル解析
	std::ifstream lutfile(m_lutFileName);
	if (!lutfile) {
		OutputErrorLog("[ERR] " + m_lutFileName + " could not find.\n");
		return false;
	}
	while (getline(lutfile, line)) {												// 1行ずつ読み込み
		std::istringstream stream(line);
		if (line.size() ==0) img_h++;
		if (img_h == 0) img_w++;
	}
	m_imgHeigh = img_h + 1;
	m_imgWidth = img_w;

	std::cout << m_lutFileName << ": w=" << m_imgWidth << ", h=" << m_imgHeigh << std::endl;
	return true;
}


/*-------------------------------------------------------------------
  FUNCTION :    LoadLUT()
  EFFECTS  :    LUTファイルの読み込み
  NOTE     :
-------------------------------------------------------------------*/
void TransForm::LoadLUT(void)
{
	std::string data;
	std::string line;
	float coef;
	int count = 0;
	cv::Mat input_x, input_y;
	// メモリ確保
	if (m_subpixMode == true) {														// サブピクセル補間が有効の場合
		input_x = cv::Mat(cv::Size(m_imgWidth, m_imgHeigh), CV_32FC1);				// LUTデータ格納vector(X)
		input_y = cv::Mat(cv::Size(m_imgWidth, m_imgHeigh), CV_32FC1);				// LUTデータ格納vector(Y)
	}
	else {																			// サブピクセル補間が無効の場合
		input_x = cv::Mat(cv::Size(m_imgWidth, m_imgHeigh), CV_32FC1);				// LUTデータ格納vector(X)
		input_y = cv::Mat(cv::Size(m_imgWidth, m_imgHeigh), CV_32FC1);				// LUTデータ格納vector(Y)
	}

	// ファイル解析
	std::ifstream lutfile(m_lutFileName);
	while (getline(lutfile, line)) {												// 1行ずつ読み込み
		std::replace(line.begin(), line.end(), '[', ' ');							// "["が邪魔なので削除
		std::replace(line.begin(), line.end(), ']', ' ');							// "]"が邪魔なので削除
		std::istringstream stream(line);
		while (getline(stream, data, ' ')) {										// スペース区切りで分割
			if (StrToFloat(data, &coef)) {											// 小数かどうか判定
				int x = (count/2) % m_imgWidth;
				int y = (count/2) / m_imgWidth;

				if (count % 2 == 0) input_x.at<float>(y,x) = (float)coef;
				else				input_y.at<float>(y,x) = (float)coef;

				if (++count > m_imgHeigh*m_imgWidth*2) {
					OutputErrorLog("[ERR] LUT file size is too large.\n");
					break;
				}
			}
		}
	}

	cv::convertMaps(input_x,input_y,m_mapX,m_mapY,11,true);
	if(update_check()) SaveLUT();													// update only
}


/*-------------------------------------------------------------------
  FUNCTION :    StrToFloat()
  EFFECTS  :    文字列から小数に変換
  NOTE     :	変換できない場合はfalseを返す
  -------------------------------------------------------------------*/
bool TransForm::StrToFloat(std::string s, float *val)
{
	bool res;

	try {
		*val = std::stof(s);
		res = true;
	}
	catch (...) {
		*val = -1;
		res = false;
	}
	return res;
}


/*-------------------------------------------------------------------
  FUNCTION :    FastLoadLUT()
  EFFECTS  :    LUTをMatのバイナリ形式で読み出し
  NOTE     :	高速化のため
  -------------------------------------------------------------------*/
bool TransForm::FastLoadLUT(void)
{
	size_t lastindex = m_lutFileName.find_last_of(".");
	std::string rawname = m_lutFileName.substr(0, lastindex);
	std::string file_x = rawname + "_x.binlut";
	std::string file_y = rawname + "_y.binlut";

	std::ifstream ifs_x(file_x, std::ios::binary);
	if(!ifs_x.is_open()){
		return false;
	}
	std::ifstream ifs_y(file_y, std::ios::binary);
	if(!ifs_y.is_open()){
		return false;
	}

	// map X
	int rows, cols, type;
	ifs_x.read((char*)(&rows), sizeof(int));
	ifs_x.read((char*)(&cols), sizeof(int));
	ifs_x.read((char*)(&type), sizeof(int));
	m_mapX.release();
	m_mapX.create(rows, cols, type);
	ifs_x.read((char*)(m_mapX.data), m_mapX.elemSize() * m_mapX.total());
	// map Y
	ifs_y.read((char*)(&rows), sizeof(int));
	ifs_y.read((char*)(&cols), sizeof(int));
	ifs_y.read((char*)(&type), sizeof(int));
	m_mapY.release();
	m_mapY.create(rows, cols, type);
	ifs_y.read((char*)(m_mapY.data), m_mapY.elemSize() * m_mapY.total());
	// info
	m_imgHeigh = rows;
	m_imgWidth = cols;
	//cv::convertMaps(m_mapX,m_mapY,m_mapX,m_mapY,11,true);
	return true;
}


/*-------------------------------------------------------------------
  FUNCTION :    SaveLUT()
  EFFECTS  :    LUTをMatのバイナリ形式で保存
  NOTE     :	高速化のため
  -------------------------------------------------------------------*/
bool TransForm::SaveLUT(void)
{
	size_t lastindex = m_lutFileName.find_last_of(".");
	std::string rawname = m_lutFileName.substr(0, lastindex);
	std::string file_x = rawname + "_x.binlut";
	std::string file_y = rawname + "_y.binlut";
	std::ofstream ofs_x(file_x, std::ios::binary);
	if(!ofs_x.is_open()){
		return false;
	}
	std::ofstream ofs_y(file_y, std::ios::binary);
	if(!ofs_y.is_open()){
		return false;
	}
	// X map
	int type_x = m_mapX.type();
	ofs_x.write((const char*)(&m_mapX.rows), sizeof(int));
	ofs_x.write((const char*)(&m_mapX.cols), sizeof(int));
	ofs_x.write((const char*)(&type_x), sizeof(int));
	ofs_x.write((const char*)(m_mapX.data), m_mapX.elemSize() * m_mapX.total());
	// Y map
	int type_y = m_mapY.type();
	ofs_y.write((const char*)(&m_mapY.rows), sizeof(int));
	ofs_y.write((const char*)(&m_mapY.cols), sizeof(int));
	ofs_y.write((const char*)(&type_y), sizeof(int));
	ofs_y.write((const char*)(m_mapY.data), m_mapY.elemSize() * m_mapY.total());

	return true;
}


/*-------------------------------------------------------------------
  FUNCTION :    ReverseBBoxAffainConvert
  EFFECTS  :    BBOX情報を魚眼座標に逆射影
  NOTE     :
  -------------------------------------------------------------------*/
void TransForm::ReverseBBoxAffainConvert(const std::vector<cv::Point2f>& detect_bbox, std::vector<cv::Point2f>* fish_bbox)
{
	int x0 = IMAGE_WIDTH_FHD;
	int y0 = IMAGE_HEIGHT_FHD;
	int x1 = 0;
	int y1 = 0;

	for (const auto& point : detect_bbox) {
		int x = std::round(point.x);
		int y = std::round(point.y);

		int fish_x = m_mapX.at<float>(y, x);
		int fish_y = m_mapY.at<float>(y, x);
		if (fish_x < 0) fish_x = 0;
		if (fish_x >= IMAGE_WIDTH_FHD) fish_x = IMAGE_WIDTH_FHD-1;
		if (fish_y < 0) fish_y = 0;
		if (fish_y >= IMAGE_HEIGHT_FHD) fish_y = IMAGE_HEIGHT_FHD-1;

		if (x0 < fish_x) x0 = fish_x;
		if (y0 < fish_y) y0 = fish_y;
		if (x1 > fish_x) x1 = fish_x;
		if (y1 > fish_y) y1 = fish_y;
	}

	fish_bbox->clear();
	fish_bbox->emplace_back(x0, y0);
	fish_bbox->emplace_back(x1, y1);
}

/*-------------------------------------------------------------------
  FUNCTION :	ReverseAffainConvert()
  EFFECTS  :	New vector<cv::Point2f> variable populated with original data  
  DETAIL   :	Searching a point in a original image matrix and returing it to DrawBBoxQuad()
  WARNING  :	Floating value is roundoff while searching for finding nearest value rather 
				real and exact value
-------------------------------------------------------------------*/
std::vector<cv::Point2f> TransForm::ReverseAffainConvert(std::vector<cv::Point2f>& corner_points)
{
	std::vector<cv::Point2f> original_coordinates;
	
	printf("reverse affain conversion points \n");
	for(const auto& corner_point: corner_points){
		// rounding off the cordinates value for fast search and always finding value
		int x_axis = std::round(corner_point.x);
		int y_axis = std::round(corner_point.y);
		printf("after offset_______________________ (x,y) = (%f, %f)\n", corner_point.x, corner_point.y);

		if(m_mapX.empty()){
			std::cout <<  "m_mapX matrix is empty" << std::endl;
		}
		if(m_mapY.empty()){
			std::cout << "m_mapY matrix is empty" << std::endl;
		}
		/*
		if(m_mapX.type() == CV_32F)  				printf("CV_32F m_mapX\n");
		else if(m_mapX.type() == CV_64F)			printf("CV_64F m_mapX\n");
		else if(m_mapX.type() == CV_16U)            printf("CV_16F m_mapX\n");
		else 										printf("unknown type for m_mapX\n");
		*/
		//printf("mapX (column) = ( %d) and mapY (rows) = (%d) \n",m_mapX.cols, m_mapY.rows);
		float original_x = 0;
		float original_y = 0;
		if(x_axis >=0 && x_axis < m_mapX.cols && y_axis >=0 && y_axis < m_mapY.rows){
			original_x = m_mapX.at<float>(y_axis, x_axis);
			original_y = m_mapY.at<float>(y_axis, x_axis);
			//printf("original x = %f , original y= %f \n", original_x, original_y);
			//original_coordinates.emplace_back(original_x, original_y);
		}
		if ((x_axis < 0 && y_axis < 0) ) {

			original_x = m_mapX.at<float>(0, 0);
			original_y = m_mapX.at<float>(0, 0);
		}

		if(x_axis > m_mapX.cols && y_axis > m_mapY.rows){
			original_x = IMAGE_WIDTH -1;
			original_y = IMAGE_HEIGHT -1;
		}
		else if(x_axis >= m_mapX.cols ){
			original_x = m_mapX.at<float>(y_axis, IMAGE_WIDTH -1);
			original_y = m_mapY.at<float>(y_axis, x_axis);
			printf("#######################original x = %f , original y= %f \n", original_x, original_y);
		}
		else if(y_axis >= m_mapY.rows){
			original_x = m_mapX.at<float>(IMAGE_HEIGHT -1, x_axis);
			original_y = m_mapY.at<float>(IMAGE_HEIGHT -1, x_axis);
			printf("***********************original x = %f , original y= %f \n", original_x, original_y); 
		}
		original_coordinates.emplace_back(original_x, original_y);	
	}
	std::cout << "real fisheye coordinates are listed as " << std::endl;
	for(const auto& point : original_coordinates){
		printf("new-->> (x,y) = (%f, %f)\n", point.x, point.y);
	}
	return original_coordinates;
}
