/********************************************************************************
;*                                                                              *
;*        Copyright (C) 2021 PAL Inc. All Rights Reserved                       *
;*                                                                              *
;********************************************************************************
;*                                                                              *
;*        開発製品名    :    HAS2                                               *
;*                                                                              *
;*        ﾓｼﾞｭｰﾙ名      :    transform.hpp                                      *
;*                                                                              *
;*        作　成　者    :    内之浦 伸治                                        *
;*                                                                              *
;********************************************************************************/

#pragma once

/*===    file include	 ===*/
#include <opencv2/core/core.hpp>
#include "opencv2/imgproc.hpp"
#include <opencv2/opencv.hpp>

/*===    user definition        ===*/


/*===    external variable    ===*/


/*===    external function    ===*/


/*===    public function prototypes    ===*/


/*===    static function prototypes    ===*/


/*===    static typedef variable        ===*/


/*===    static variable        ===*/


/*===    constant variable    ===*/


/*===    global variable        ===*/


/*===    implementation        ===*/


class TransForm
{
public:
	TransForm(void);
	~TransForm();
	void Init(std::string, bool);									// 初期設定
	void AffainConversion(const cv::Mat &, cv::Mat *);				// 視点変換
	bool SaveLUT(void);												// [PCAPP専用] LUTの保存
	void GetDummyImage(cv::Mat *);									// [デバッグ] 静止画読み出し
	void LoadLUT(void);												// LUTファイルの読み込み
	bool GetOutImageSizeFromLutFile(std::string file);				// 出力画像サイズのチェック
	void ReverseBBoxAffainConvert(const std::vector<cv::Point2f>&, std::vector<cv::Point2f>*);// BBOX情報を魚眼座標に逆射影

	std::vector<cv::Point2f>ReverseAffainConvert(std::vector<cv::Point2f>&);

	const cv::Mat mappingXCalling() const {
		return m_mapX;
	}
	const cv::Mat mappingYCalling() const {
		return m_mapY;
	}
private:

	bool FastLoadLUT(void);											// LUTファイルの読み込み(高速版)
	bool StrToFloat(std::string, float *);							// 文字列→小数への変換
	std::string m_lutFileName;										// LUTファイル名
	bool m_subpixMode;												// サブピクセル補間モード
	cv::Mat m_mapX;													// 変換テーブルX
	cv::Mat m_mapY;													// 変換テーブルY
	int m_imgWidth;													// 変換後の画像幅
	int m_imgHeigh;													// 変換後の画像長さ
};
