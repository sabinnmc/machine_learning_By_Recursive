//--------------------------------------------------
//! @file Inference.hpp
//! @brief 推論ｸﾗｽ
//--------------------------------------------------

#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include "Sysdef.h"
#include "HailoDetect.h"
#include "LinuxUtility.hpp"

//--------------------------------------------------
//! @brief 推論ｸﾗｽ
//!
//! 画像を渡すことで別ｽﾚｯﾄﾞにて推論を実施する
//--------------------------------------------------
class CInference{
public:
	//--------------------------------------------------
	//! @brief 初期化用構造体
	//--------------------------------------------------
	struct InitParameter{
		std::string			m_modelFile="";						//!< 推論ﾓﾃﾞﾙのﾊﾟｽ
		std::string			m_modelBackupPath="";				//!< 推論ﾓﾃﾞﾙ（ﾊﾞｯｸｱｯﾌﾟ）のﾊﾟｽ
		double				m_scoreThreshold=-1.;				//!< ｽｺｱの閾値
		double				m_nmsThreshold=-1.;					//!< NMSの閾値
		std::vector<int>	m_targetLabels;						//!< 推論対象のﾗﾍﾞﾙ
		bool				m_isModelErrorCheck=true;			//!< ﾓﾃﾞﾙのｴﾗｰﾁｪｯｸを実施するか
		DetectSetting		m_detectSetting;					//!< 物体検出に使用する設定
	};

	//--------------------------------------------------
	//! @brief ｺﾝｽﾄﾗｸﾀ
	//--------------------------------------------------
	CInference();

	//--------------------------------------------------
	//! @brief ﾃﾞｽﾄﾗｸﾀ
	//--------------------------------------------------
	~CInference();

	/**
	 * @brief  初期化
	 *
	 * @param initParameter				初期化用ﾊﾟﾗﾒｰﾀ
	 * @retval			0				初期化成功
	 * @retval			1				weightエラー
	 * @retval			2				ﾃﾞﾊﾞｲｽエラー
	 * @retval			3				initエラー
	 */
	int Init(const InitParameter& initParameter);

	//--------------------------------------------------
	//! @brief 推論
	//!
	//! 別ｽﾚｯﾄﾞにて推論を開始する
	//! @param[in]		image			推論する対象の画像
	//--------------------------------------------------
	void Inference(const cv::Mat& image);

	//--------------------------------------------------
	//! @brief 推論
	//!
	//! 別ｽﾚｯﾄﾞにて推論を開始する
	//! @param[in]		image			推論する対象の画像
	//! @param[in]		image_1			推論する対象の画像
	//--------------------------------------------------
	void Inference_fisheye(const cv::Mat& image,const cv::Mat& image_1);

	//--------------------------------------------------
	//! @brief 推論中か？
	//!
	//! @retval			true			推論中
	//! @retval			false			推論完了or推論要求が発生していない
	//--------------------------------------------------
	bool IsInInference(void)const;

	//--------------------------------------------------
	//! @brief 推論が完了したか？
	//!
	//! 完了後はClearInferenceCompleteFlagを呼び出し、ﾌﾗｸﾞをｸﾘｱすること
	//! @retval			true			推論完了
	//! @retval			false			推論未完了
	//--------------------------------------------------
	bool IsInferenceComplete()const;

	//--------------------------------------------------
	//! @brief 推論完了ﾌﾗｸﾞをｸﾘｱ
	//--------------------------------------------------
	void ClearInferenceCompleteFlag();

	//--------------------------------------------------
	//! @brief 推論時にｴﾗｰが発生したか？
	//!
	//! @retval			true			ｴﾗｰ発生
	//! @retval			false			正常終了
	//--------------------------------------------------
	bool IsAppearError()const;

	//--------------------------------------------------
	//! @brief ﾊﾞｳﾝﾃﾞｨﾝｸﾞﾎﾞｯｸｽの情報を取得
	//!
	//! @param[out]		box				ﾊﾞｳﾝﾃﾞｨﾝｸﾞﾎﾞｯｸｽ情報の格納先
	//--------------------------------------------------
	void GetBoundingBox(std::vector<BboxInfo>*const box)const;

	//--------------------------------------------------
	//! @brief 推論ﾊﾟﾌｫｰﾏﾝｽ情報を取得
	//!
	//! @retval							推論ﾊﾟﾌｫｰﾏﾝｽ
	//--------------------------------------------------
	float GetinferencePerformance()const;

	//--------------------------------------------------
	//! @brief Hailoｺｱ温度情報を取得
	//!
	//! @retval							温度情報
	//--------------------------------------------------
	std::tuple <float, float> GetHailoTemp();

	//--------------------------------------------------
	//! @brief ﾓﾃﾞﾙﾃﾞｰﾀﾊﾞｰｼﾞｮﾝ取得
	//! @return	ﾓﾃﾞﾙﾃﾞｰﾀﾊﾞｰｼﾞｮﾝ
	//--------------------------------------------------
	inline const std::string&  GetDataVersion()const{return m_dataVersion;}

	//--------------------------------------------------
	//! @brief 推論に使用した画像を取得取得
	//!
	//! 推論後の枠などは描画されていない
	//! @return	推論に使用した画像
	//--------------------------------------------------
	inline cv::Mat	GetSrcImage()const{return m_image.clone();}
private:
	//--------------------------------------------------
	//! @brief ﾓﾃﾞﾙﾃﾞｰﾀ
	//--------------------------------------------------
	struct LoadModelData{
		int					convertVersion;				//!< ｺﾝﾊﾞｰﾄﾊﾞｰｼﾞｮﾝ
		std::string			projectCode;				//!< ﾌﾟﾛｼﾞｪｸﾄｺｰﾄﾞ
		eAiType				aiType;						//!< AI分類
		eEngineType			engineType;					//!< 推論ｴﾝｼﾞﾝﾀｲﾌﾟ
		eModelType			modelType;					//!< ﾓﾃﾞﾙﾀｲﾌﾟ
		std::string			dataVersion;				//!< ﾓﾃﾞﾙﾃﾞｰﾀﾊﾞｰｼﾞｮﾝ
		std::vector<char>	buffer;						//!< ﾍｯﾀﾞｰﾊﾞｯﾌｧ
		std::vector<char>	modelData;					//!< ﾓﾃﾞﾙﾃﾞｰﾀ
	};

	//--------------------------------------------------
	//! @brief 推論ｽﾚｯﾄﾞ生成
	//!
	//! @retval	0		成功
	//! @retval	0以外	失敗。pthread_createの戻り値
	//--------------------------------------------------
	int CreateInferenceThread(void);

	//--------------------------------------------------
	//! @brief 推論ｽﾚｯﾄﾞ終了
	//!
	//! @retval	0		成功
	//! @retval	0以外	失敗。pthread_joinの戻り値
	//--------------------------------------------------
	int DeleteInferenceThread(void);

	//--------------------------------------------------
	//! @brief 推論ｽﾚｯﾄﾞ
	//!
	//! @param[in]		targ			未使用
	//! @return	NULL固定
	//--------------------------------------------------
	static void* InferenceThreadFunc(void *targ);

	//--------------------------------------------------
	//! @brief 推論ｽﾚｯﾄﾞ
	//!
	//! @param[in]		targ			未使用
	//! @return	NULL固定
	//--------------------------------------------------
	static void* InferenceThreadFunc_fisheye(void *targ);

	//--------------------------------------------------
	//! @brief 推論ﾓﾃﾞﾙﾌｧｲﾙ読み込み
	//!
	//! isModelErrorCheckがtrueであればﾍｯﾀﾞｰ情報のﾌﾟﾛｼﾞｪｸﾄｺｰﾄﾞやAI分類などが正しいか確認し、
	//! 誤りがあれば読み込み失敗を返す
	//! ﾓﾃﾞﾙが読み込めない・ｺﾝﾊﾞｰﾄﾊﾞｰｼﾞｮﾝ不一致・ﾊｯｼｭ不一致などはﾌﾗｸﾞに関係なく確認する
	//! @param[out]		modelData			読み込んだﾓﾃﾞﾙのﾃﾞｰﾀ
	//! @param[in]		modelPath			ﾓﾃﾞﾙﾊﾟｽ
	//! @param[in]		isModelErrorCheck	ﾓﾃﾞﾙのｴﾗｰﾁｪｯｸをするか
	//! @retval			true				読み込み成功
	//! @retval			false				読み込み失敗
	//--------------------------------------------------
	bool LoadModel(LoadModelData*const modelData, const std::string& modelPath, bool isModelErrorCheck);

	//--------------------------------------------------
	//! @brief 推論ﾃﾞﾊﾞｲｽ接続確認
	//!
	//! 推論ﾃﾞﾊﾞｲｽが接続されているか確認
	//!
	//!
	//! @retval			true				確認成功
	//! @retval			false				確認失敗
	//--------------------------------------------------
	bool CheckDevice(void);

	//--------------------------------------------------
	//! @brief ｽﾚｯﾄﾞ用ﾃﾞｰﾀ
	//--------------------------------------------------
	struct ThreadData{
		//--------------------------------------------------
		//! @brief ｺﾝｽﾄﾗｸﾀ
		//!
		//! ﾒﾝﾊﾞ変数に参照ﾃﾞｰﾀがあるためｺﾝｽﾄﾗｸﾀにて設定
		//! 推論結果に応じてﾃﾞｰﾀの更新をするため、ここで渡した参照は排他処理無しに外部で触らないこと
		//! @param[in,out]	inference_mutex			推論Mutex
		//! @param[in,out]	end_inference_thread	推論ｽﾚｯﾄﾞ終了ﾌﾗｸﾞ
		//! @param[in,out]	detect					推論ｸﾗｽ
		//! @param[in,out]	image					推論対象画像
		//! @param[in,out]	image_1					推論対象画像
		//! @param[in,out]	boundingbox				ﾊﾞｳﾝﾃﾞｨﾝｸﾞﾎﾞｯｸｽ
		//! @param[in,out]	infer_req				推論ﾘｸｴｽﾄ
		//! @param[in,out]	infer_comp				推論完了ﾌﾗｸﾞ
		//! @param[in,out]	infer_fps				推論ﾊﾟﾌｫｰﾏﾝｽ
		//--------------------------------------------------
		ThreadData(	pthread_mutex_t&		inference_mutex,
					int&					end_inference_thread,
					HAILODetection&			detect,
					int&					temp_req,
					float&					hailo_ts0,
					float&					hailo_ts1,
					cv::Mat&				image,
					cv::Mat&				image_1,
					std::vector<BboxInfo>&	boundingbox,
					int&					infer_req,
					int&					infer_comp,
					float&					infer_fps,
					int&					hailo_error);
		//--------------------------------------------------
		//! @brief 推論対象のﾗﾍﾞﾙを設定
		//! @param[in]		targetLabels		推論対象のﾗﾍﾞﾙ
		//--------------------------------------------------
		inline void SetTargetLabels(const std::vector<int>& targetLabels){
			m_targetLabels = targetLabels;
		}
		//--------------------------------------------------
		//! @brief 物体検出に使用する設定を設定
		//! @param[in]		detectSetting		推論対象のﾗﾍﾞﾙ
		//--------------------------------------------------
		inline void SetDetectSetting(const DetectSetting& detectSetting){
			m_detectSetting = detectSetting;
		}

		// 参照変数群						// reference variable
		// ｽﾚｯﾄﾞ管理用ｵﾌﾞｼﾞｪｸﾄ				// object for thread management
		pthread_mutex_t&		m_inference_mutex;			//!< 推論Mutex
		int&					m_end_inference_thread;		//!< 推論ｽﾚｯﾄﾞ終了要求
		// 推論ｵﾌﾞｼﾞｪｸﾄ
		HAILODetection&			m_detect;					//!< 推論ｸﾗｽ
		int&					m_temp_req;					//!< 温度計測ﾘｸｴｽﾄ
		float&					m_hailo_ts0;				//!< TS0温度
		float&					m_hailo_ts1;				//!< TS1温度
		cv::Mat&				m_image;					//!< 推論対象画像
		cv::Mat&				m_image_1;  				//!< 推論対象画像
		std::vector<BboxInfo>&	m_boundingbox;				//!< ﾊﾞｳﾝﾃﾞｨﾝｸﾞﾎﾞｯｸｽ
		int&					m_infer_req;				//!< 推論ﾘｸｴｽﾄ
		int&					m_infer_comp;				//!< 推論完了ﾌﾗｸﾞ
		float&					m_infer_fps;				//!< 推論ﾊﾟﾌｫｰﾏﾝｽ
		int&					m_hailo_error;				//!< HAILOデバイス異常

		// 内部ﾊﾟﾗﾒｰﾀ
		std::vector<int>		m_targetLabels;				//!< 推論対象のﾗﾍﾞﾙ
		DetectSetting			m_detectSetting;			//!< 物体検出に使用する設定
	};

	// 推論ｽﾚｯﾄﾞｵﾌﾞｼﾞｪｸﾄ
	mutable pthread_t		m_inference_thread;
	// ｽﾚｯﾄﾞ用ﾃﾞｰﾀ
	ThreadData				m_threadData;
	// ｽﾚｯﾄﾞから参照するｵﾌﾞｼﾞｪｸﾄ
	mutable pthread_mutex_t	m_inference_mutex;			//!< 推論ｽﾚｯﾄﾞMutex
	int						m_end_inference_thread;		//!< 推論ｽﾚｯﾄﾞ終了要求
	HAILODetection			m_detect;					//!< 推論ｸﾗｽ
	int						m_temp_req;					//!< 温度計測ﾘｸｴｽﾄ
	float					m_hailo_ts0;				//!< TS0温度
	float					m_hailo_ts1;				//!< TS1温度
	cv::Mat					m_image;					//!< 推論対象画像
	cv::Mat					m_image_1;  				//!< 推論対象画像
	std::vector<BboxInfo>	m_boundingbox;				//!< ﾊﾞｳﾝﾃﾞｨﾝｸﾞﾎﾞｯｸｽ
	int						m_infer_req;				//!< 推論ﾘｸｴｽﾄ
	int						m_infer_comp;				//!< 推論完了ﾌﾗｸﾞ
	float					m_infer_fps;				//!< 推論ﾊﾟﾌｫｰﾏﾝｽ
	int						m_hailo_error;				//!< HAILOデバイス異常

	// 内部ﾊﾟﾗﾒｰﾀ
	bool					m_is_run_thread;			//!< 推論ｽﾚｯﾄﾞ実行中
	std::string				m_dataVersion;				//!< ﾓﾃﾞﾙﾃﾞｰﾀﾊﾞｰｼﾞｮﾝ
	TP						m_timeStartedInference;		//!< 推論を開始した時刻
};
