//--------------------------------------------------
//! @file Inference.cpp
//! @brief 推論ｸﾗｽ
//--------------------------------------------------

#include "Inference.hpp"

#include <iostream>
#include <iterator>
#include <sha256.h>
#include <unistd.h>
#include <map>
#include <sys/stat.h>

#include "Sysdef.h"
#include "LinuxUtility.hpp"

#include "boost/filesystem.hpp"
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/algorithm/string/classification.hpp>

CInference::CInference():m_threadData(m_inference_mutex, m_end_inference_thread, m_detect, m_temp_req, m_hailo_ts0, m_hailo_ts1, m_image, m_image_1, m_boundingbox, m_infer_req, m_infer_comp, m_infer_fps, m_hailo_error),
						 m_end_inference_thread(1), m_is_run_thread(false){

}

CInference::~CInference(){
	DeleteInferenceThread();
}

#define DMP_SHA256_DIGEST_STRING_LENGTH 64	//!< ﾊｯｼｭ部分のｻｲｽﾞ
#define INFER_FPS_FRAME_AVG				10	//!< 推論ﾊﾟﾌｫｰﾏﾝｽ算出平均ﾌﾚｰﾑ数

int CInference::Init(const InitParameter& initParameter){
	// ｽﾚｯﾄﾞ用ﾃﾞｰﾀ作成
	m_threadData.SetTargetLabels(initParameter.m_targetLabels);
	m_threadData.SetDetectSetting(initParameter.m_detectSetting);

	// 推論ｽﾚｯﾄﾞ起動
	if(CreateInferenceThread() != 0){
		ERR("CInference::Init CreateInferenceThread failed.");
		return -1;															// ｽﾚｯﾄﾞ作成エラー
	}

	// ﾓﾃﾞﾙﾃﾞｰﾀ読み込み
	CInference::LoadModelData data;
	if(LoadModel(&data, initParameter.m_modelFile, initParameter.m_isModelErrorCheck) == false){
		if(LoadModel(&data, initParameter.m_modelBackupPath, initParameter.m_isModelErrorCheck) == false){
			ERR("CInference::Init LoadModel failed.");
			return 1;														// weightエラー
		}
	}
	m_dataVersion = data.dataVersion;

	if(CheckDevice() == false)
	{
		ERR("CInference::Init CheckDevice failed.");
		return 2;															// ﾃﾞﾊﾞｲｽ接続・認識エラー
	}

	try{
		if( m_detect.hailoinit(data.modelType, data.modelData.data(), data.modelData.size()) == false ){
			ERR("CInference::Init Hailo failed.");
			return 3;															// GyrfalconのInitエラー
		}
	}
	catch (exception & rE)
	{
		ERR("CInference::Init Hailo Errow Chatch. : %s", rE.what());
		return 3;															// GyrfalconのInitエラー
	}

	return 0;
}

void CInference::Inference(const cv::Mat& image){
	if(IsInInference()){
		return;
	}
	pthread_mutex_lock(&m_inference_mutex);
	m_image = image.clone();
	m_infer_req = 1;													// 推論ﾘｸｴｽﾄﾌﾗｸﾞON
	pthread_mutex_unlock(&m_inference_mutex);
	m_timeStartedInference = getTime();
}
void CInference::Inference_fisheye(const cv::Mat& image,const cv::Mat& image_1){
	if(IsInInference()){
		return;
	}
	pthread_mutex_lock(&m_inference_mutex);
	cv::cvtColor(image, m_image, cv::COLOR_BGR2RGB);					// clone()が不要になる
	cv::cvtColor(image_1, m_image_1, cv::COLOR_BGR2RGB);				// clone()が不要になる
	m_infer_req = 1;													// 推論ﾘｸｴｽﾄﾌﾗｸﾞON
	pthread_mutex_unlock(&m_inference_mutex);
	m_timeStartedInference = getTime();
}
bool CInference::IsInInference(void)const{
	pthread_mutex_lock(&m_inference_mutex);
	int infer_req = m_infer_req;
	pthread_mutex_unlock(&m_inference_mutex);
	return infer_req == 1;
}

bool CInference::IsInferenceComplete()const{
	pthread_mutex_lock(&m_inference_mutex);
	int infer_comp = m_infer_comp;
	pthread_mutex_unlock(&m_inference_mutex);
	return infer_comp == 1;
}

void CInference::ClearInferenceCompleteFlag(){
	pthread_mutex_lock(&m_inference_mutex);
	m_infer_comp = 0;
	pthread_mutex_unlock(&m_inference_mutex);
}

bool CInference::IsAppearError()const{
	// 推論中でなければ特にｴﾗｰなし			// if no interface , no error
	if(IsInInference()==false){
		return false;
	}

	if( m_hailo_error ){
		return true;
	}
	// 一定時間以内に推論が終わらなければｴﾗｰ発生と判定する
	auto diff = getTimeDiff(m_timeStartedInference, getTime());
	if(diff >= INFERENCE_ERROR_TIME){
		ERR("CInference::InferenceThreadFunc InferenceError.  time : %ld", diff);
		return true;
	}
	return false;
}

void CInference::GetBoundingBox(std::vector<BboxInfo>*const box)const{
	if(box == nullptr){return;}
	pthread_mutex_lock(&m_inference_mutex);
	*box = m_boundingbox;
	pthread_mutex_unlock(&m_inference_mutex);
}

float CInference::GetinferencePerformance()const{
	pthread_mutex_lock(&m_inference_mutex);
	float infer_fps = m_infer_fps;
	pthread_mutex_unlock(&m_inference_mutex);
	return infer_fps;
}
std::tuple <float, float> CInference::GetHailoTemp(){
	pthread_mutex_lock(&m_inference_mutex);
	float hailots0 = m_hailo_ts0;
	float hailots1 = m_hailo_ts1;
	m_temp_req = 1;
	pthread_mutex_unlock(&m_inference_mutex);
	return {hailots0,hailots1};
}

int CInference::CreateInferenceThread(void){
	// Create時にｽﾚｯﾄﾞ生成済みならｽｷｯﾌﾟ
	if(m_is_run_thread){return 0;}

	m_end_inference_thread = 0;
	m_infer_req = 0;
	m_infer_comp = 0;
	m_infer_fps = 0.0f;
	m_temp_req = 0;
	m_hailo_ts0 = 0.0f;
	m_hailo_ts1 = 0.0f;
	m_hailo_error = 0;

	pthread_mutex_init(&m_inference_mutex, NULL);
	int ret = pthread_create(&m_inference_thread, NULL, InferenceThreadFunc_fisheye, (void*)(&m_threadData));
	if(ret == 0){
		m_is_run_thread = true;
	}
	else{
		ERR("CInference::CreateInferenceThread pthread_create failed %d", ret);
	}
	return ret;
}

int CInference::DeleteInferenceThread(void){
	// Delete時にｽﾚｯﾄﾞ生成前ならｽｷｯﾌﾟ
	if(!m_is_run_thread){return 0;}

	pthread_mutex_lock(&m_inference_mutex);
	m_end_inference_thread = 1;
	pthread_mutex_unlock(&m_inference_mutex);

	int ret = pthread_join(m_inference_thread, NULL);
	if(ret == 0){
		m_is_run_thread = false;
	}
	else{
		ERR("CInference::DeleteInferenceThread pthread_join failed %d",ret);
	}
	return ret;
}
void* CInference::InferenceThreadFunc_fisheye(void *targ){
	int inferreq;
	int tempreq;
	int end_thread_request;
	int infer_frame = 0;
	int hailoerror;
	TP st_time = getTime();
	TP end_time;
	float inferfps;
	cv::Mat image_front;
	cv::Mat image_rear;
	ThreadData*	threadData = (ThreadData*)targ;
	int w_flg;
	while (true)
	{
		usleep(1000);

		pthread_mutex_lock(&threadData->m_inference_mutex);
		inferreq = threadData->m_infer_req;
		tempreq = threadData->m_temp_req;
		threadData->m_temp_req = 0;
		end_thread_request = threadData->m_end_inference_thread;
		hailoerror = threadData->m_hailo_error;
		pthread_mutex_unlock(&threadData->m_inference_mutex);
		// 推論終了
		if(end_thread_request){
			break;
		}
		if(tempreq){
			auto [hailots0, hailots1] = threadData->m_detect.GetHailoTemp();
			pthread_mutex_lock(&threadData->m_inference_mutex);
			threadData->m_hailo_ts0 = hailots0;
			threadData->m_hailo_ts1 = hailots1;
			pthread_mutex_unlock(&threadData->m_inference_mutex);
		}
		// 推論要望がなければ次のループまで待機
		if(!inferreq){
			continue;
		}
		if( hailoerror ){
			continue;
		}

		// 画像受信
		pthread_mutex_lock(&threadData->m_inference_mutex);
		image_front = threadData->m_image.clone();
		image_rear = threadData->m_image_1.clone();
		threadData->m_infer_req = 0;
		pthread_mutex_unlock(&threadData->m_inference_mutex);
		if(infer_frame == 0){
			st_time = getTime();
		}
		infer_frame++;

		for(w_flg = 0;w_flg<2;w_flg++){            //2枚の画像を順番に推論処理
			// 推論
			while(true){
				// 推論終了命令が届いたら終了
				pthread_mutex_lock(&threadData->m_inference_mutex);
				end_thread_request = threadData->m_end_inference_thread;
				pthread_mutex_unlock(&threadData->m_inference_mutex);
				if(end_thread_request){
					break;
				}

				// 推論
				try{
					if(threadData->m_detect.CheckDevice() == false){
						pthread_mutex_lock(&threadData->m_inference_mutex);
						hailoerror = 1;
						threadData->m_hailo_error = hailoerror;
						pthread_mutex_unlock(&threadData->m_inference_mutex);
						break;
					}
					bool process_ret = false;
					if(w_flg == 0) process_ret = threadData->m_detect.processImage(image_front);
					if(w_flg == 1) process_ret = threadData->m_detect.processImage(image_rear);
					if(process_ret == false){
						pthread_mutex_lock(&threadData->m_inference_mutex);
						hailoerror = 1;
						threadData->m_hailo_error = hailoerror;
						pthread_mutex_unlock(&threadData->m_inference_mutex);
						break;
					}
					break;
				}
				catch (exception & rE)
				{
					ERR("CInference::InferenceThreadFunc processImage failed. : %s", rE.what());
					// 失敗する限りやり直す
					usleep(1000);
					continue;
				}
			}
			if( hailoerror ){
				continue;
			}
			// 推論終了
			if(end_thread_request){
				break;
			}
			// 推論正解時の結果送信
			pthread_mutex_lock(&threadData->m_inference_mutex);
			if(w_flg == 0) threadData->m_boundingbox.clear();
			const auto& targetLabelsBegin = threadData->m_targetLabels.begin();
			const auto& targetLabelsEnd = threadData->m_targetLabels.end();
			for(const auto& detection : threadData->m_detect.GetBboxesNms()){
				boundingbox bbox;
				if(w_flg == 0) {
					bbox.label = detection->get_class_id();
					bbox.score = detection->get_confidence();
				    HailoBBox hbbox = detection->get_bbox();
					bbox.x1 = hbbox.xmin() * image_front.cols;
					bbox.x2 = hbbox.xmax() * image_front.cols;
					bbox.y1 = hbbox.ymin() * image_front.rows;
					bbox.y2 = hbbox.ymax() * image_front.rows;
					bbox.flg = 0;
				}else{
					bbox.label = detection->get_class_id();
					bbox.score = detection->get_confidence();
				    HailoBBox hbbox = detection->get_bbox();
					bbox.x1 = hbbox.xmin() * image_rear.cols;
					bbox.x2 = hbbox.xmax() * image_rear.cols;
					bbox.y1 = hbbox.ymin() * image_rear.rows;
					bbox.y2 = hbbox.ymax() * image_rear.rows;
					bbox.flg = 1;
				}
				if(std::find(targetLabelsBegin, targetLabelsEnd, bbox.label) == targetLabelsEnd){
					continue;
				}

				// ﾎﾞｯｸｽｻｲｽﾞが一定範囲内でなければ排除
				float w = bbox.x2 - bbox.x1;
				float h = bbox.y2 - bbox.y1;
				if(w < threadData->m_detectSetting.m_minBoxW || threadData->m_detectSetting.m_maxBoxW < w ){
					continue;
				}
				if(h < threadData->m_detectSetting.m_minBoxH || threadData->m_detectSetting.m_maxBoxH < h ){
					continue;
				}
				threadData->m_boundingbox.push_back(bbox);

			}
			// 推論完了
			threadData->m_infer_comp = 1;
			if(infer_frame >= INFER_FPS_FRAME_AVG){
				infer_frame = 0;
				end_time = getTime();
				inferfps = getFPS(st_time,end_time,INFER_FPS_FRAME_AVG);
				threadData->m_infer_fps = inferfps;
			}
			pthread_mutex_unlock(&threadData->m_inference_mutex);
		}
	}

	return NULL;
}
void* CInference::InferenceThreadFunc(void *targ){
	int inferreq;
	int tempreq;
	int end_thread_request;
	int infer_frame = 0;
	TP st_time = getTime();
	TP end_time;
	float inferfps;
	cv::Mat image;
	ThreadData*	threadData = (ThreadData*)targ;

	while (true)
	{
		usleep(1000);

		pthread_mutex_lock(&threadData->m_inference_mutex);
		inferreq = threadData->m_infer_req;

		tempreq = threadData->m_temp_req;
		threadData->m_temp_req = 0;
		end_thread_request = threadData->m_end_inference_thread;
		pthread_mutex_unlock(&threadData->m_inference_mutex);
		// 推論終了
		if(end_thread_request){
			break;
		}
		if(tempreq){
			auto [hailots0, hailots1] = threadData->m_detect.GetHailoTemp();
			pthread_mutex_lock(&threadData->m_inference_mutex);
			threadData->m_hailo_ts0 = hailots0;
			threadData->m_hailo_ts1 = hailots1;
			pthread_mutex_unlock(&threadData->m_inference_mutex);
		}
		// 推論要望がなければ次のループまで待機
		if(!inferreq){
			continue;
		}

		// 画像受信
		pthread_mutex_lock(&threadData->m_inference_mutex);
		image = threadData->m_image.clone();
		threadData->m_infer_req = 0;
		pthread_mutex_unlock(&threadData->m_inference_mutex);
		if(infer_frame == 0){
			st_time = getTime();
		}
		infer_frame++;

		// 推論
		while(true){
			// 推論終了命令が届いたら終了
			pthread_mutex_lock(&threadData->m_inference_mutex);
			end_thread_request = threadData->m_end_inference_thread;
			pthread_mutex_unlock(&threadData->m_inference_mutex);
			if(end_thread_request){
				break;
			}
			// 推論
			try{
				threadData->m_detect.processImage(image);
				break;
			}
			catch (exception & rE)
			{
				ERR("CInference::InferenceThreadFunc processImage failed. : %s", rE.what());
				// 失敗する限りやり直す
				usleep(1000);
				continue;
			}
		}
		// 推論終了
		if(end_thread_request){
			break;
		}
		// 推論正解時の結果送信
		pthread_mutex_lock(&threadData->m_inference_mutex);
		threadData->m_boundingbox.clear();
		const auto& targetLabelsBegin = threadData->m_targetLabels.begin();
		const auto& targetLabelsEnd = threadData->m_targetLabels.end();
		for(const auto& detection : threadData->m_detect.GetBboxesNms()){
			boundingbox bbox;
			bbox.label = detection->get_class_id();
			bbox.score = detection->get_confidence();
		    HailoBBox hbbox = detection->get_bbox();
			bbox.x1 = hbbox.xmin() * image.cols;
			bbox.x2 = hbbox.xmax() * image.cols;
			bbox.y1 = hbbox.ymin() * image.rows;
			bbox.y2 = hbbox.ymax() * image.rows;

			if(std::find(targetLabelsBegin, targetLabelsEnd, bbox.label) == targetLabelsEnd){
				continue;
			}
			// ﾎﾞｯｸｽｻｲｽﾞが一定範囲内でなければ排除
			float w = bbox.x2 - bbox.x1;
			float h = bbox.y2 - bbox.y1;
			if(w < threadData->m_detectSetting.m_minBoxW || threadData->m_detectSetting.m_maxBoxW < w ){
				continue;
			}
			if(h < threadData->m_detectSetting.m_minBoxH || threadData->m_detectSetting.m_maxBoxH < h ){
				continue;
			}
			threadData->m_boundingbox.push_back(bbox);

		}
		// 推論完了
		threadData->m_infer_comp = 1;
		if(infer_frame >= INFER_FPS_FRAME_AVG){
			infer_frame = 0;
			end_time = getTime();
			inferfps = getFPS(st_time,end_time,INFER_FPS_FRAME_AVG);
			threadData->m_infer_fps = inferfps;
		}
		pthread_mutex_unlock(&threadData->m_inference_mutex);
	}

	return NULL;
}

bool CInference::LoadModel(LoadModelData*const modelData, const std::string& modelPath, bool isModelErrorCheck){
	assert(modelData);

	static const int lenConvertVersion = 2;
	static const int lenProjectCode = 20;
	static const int lenAiType = 1;
	static const int lenEngineType = 1;
	static const int lenModelType = 1;
	static const int lenDataVersion = 20;
	static const int lenBuffer = 10;
	static const int headerSize = lenConvertVersion + lenProjectCode + lenAiType + lenEngineType + lenModelType + lenDataVersion + lenBuffer + DMP_SHA256_DIGEST_STRING_LENGTH;
	// ﾓﾃﾞﾙ読み込み
	ifstream model(modelPath, ios::in|ios::binary);
	// 読み込み失敗検出
	if(!model){
		OutputErrorLog("failed model read. " + modelPath);
		return false;
	}
	// ﾊﾞｲﾅﾘﾃﾞｰﾀ取得
	vector<char> inputData{(istreambuf_iterator<char>(model)), (istreambuf_iterator<char>())};

	// ﾓﾃﾞﾙｻｲｽﾞがﾍｯﾀﾞｰ+ﾃﾞｰﾀ部1ﾊﾞｲﾄ以上ないとおかしい
	if(inputData.size() < headerSize + 1){
		OutputErrorLog("model size is wrong. " + modelPath);
		return false;
	}

	int index = 0;																//!< 読み込むｱﾄﾞﾚｽ
	// ｺﾝﾊﾞｰﾄﾊﾞｰｼﾞｮﾝ
	modelData->convertVersion = (static_cast<int>(inputData[index]) << 8) + (static_cast<int>(inputData[index+1]) << 0);
	if(modelData->convertVersion != 1){	// 現在は1のみ対応
		OutputErrorLog("invalid value convertVersion. " + modelPath);
		return false;
	}
	index += lenConvertVersion;

	// ﾌﾟﾛｼﾞｪｸﾄｺｰﾄﾞ
	{
		modelData->projectCode = "";
		for(int i=0;i<lenProjectCode;++i){
			auto t = *(inputData.begin()+index + i);
			if(t == 0){
				break;
			}
			modelData->projectCode += t;
		}
		index += lenProjectCode;
	}

	// AI分類
	modelData->aiType = static_cast<eAiType>(inputData[index]);
	index += lenAiType;

	// 推論ｴﾝｼﾞﾝﾀｲﾌﾟ
	modelData->engineType = static_cast<eEngineType>(inputData[index]);
	index += lenEngineType;

	// ﾓﾃﾞﾙﾀｲﾌﾟ
	modelData->modelType = static_cast<eModelType>(inputData[index]);
	index += lenModelType;

	// ﾃﾞｰﾀﾊﾞｰｼﾞｮﾝ
	{
		for(int i=0;i<lenDataVersion;++i){
			auto t = *(inputData.begin()+index + i);
			if(t == 0){
				break;
			}
			modelData->dataVersion += t;
		}
		index += lenDataVersion;
	}

	// ﾍｯﾀﾞｰﾊﾞｯﾌｧ
	{
		modelData->buffer.clear();
		modelData->buffer.insert(modelData->buffer.end(), inputData.begin()+index, inputData.begin()+index+lenBuffer);
		index += lenBuffer;
	}

	// ﾓﾃﾞﾙﾃﾞｰﾀの場所を計算
	int modelIndex = index + DMP_SHA256_DIGEST_STRING_LENGTH;	// ﾍｯﾀﾞｰ+ﾁｪｯｸｻﾑ分進んだところにﾓﾃﾞﾙﾃﾞｰﾀがある
	int modelSize = static_cast<int>(inputData.size() - modelIndex);

	// ﾓﾃﾞﾙのｴﾗｰﾁｪｯｸ
	if(isModelErrorCheck){
		static const map<eEngineType, vector<eModelType>> MODEL_TYPE_TABLE				//!< ﾓﾃﾞﾙﾀｲﾌﾟの設定組み合わせﾃｰﾌﾞﾙ
		{
			{ENGINE_TYPE_HAILO, vector<eModelType>{MODEL_TYPE_YOLOV5, MODEL_TYPE_YOLOX_S, MODEL_TYPE_YOLOV4_TINY, MODEL_TYPE_YOLOX_TINY}}	// HAILO
		};
		// AI分類
		if(modelData->aiType < 0 || modelData->aiType == AI_TYPE_INVALID || modelData->aiType >= MAX_AI_TYPE){
			OutputErrorLog("invalid value aiType. " + modelPath);
			return false;
		}
		// 推論ｴﾝｼﾞﾝ
		if(MODEL_TYPE_TABLE.find(modelData->engineType) == MODEL_TYPE_TABLE.end()){
			OutputErrorLog("invalid value engineType. " + modelPath);
			return false;
		}
		// ﾓﾃﾞﾙﾀｲﾌﾟ
		const auto& v = MODEL_TYPE_TABLE.at(modelData->engineType);
		if(std::find(v.begin(), v.end(), modelData->modelType) == v.end()){
			OutputErrorLog("invalid value modelType. " + modelPath);
			return false;
		}
	}
	// ﾊｯｼｭ値ﾁｪｯｸ
	{
		SHA256_CTX ctx;
		unsigned char hash[DMP_SHA256_DIGEST_STRING_LENGTH+1] = {0};
		SHA256_Init(&ctx);													// 初期化
		SHA256_Update(&ctx, reinterpret_cast<uint8_t*>(&inputData[modelIndex]), modelSize);
		SHA256_End(&ctx, (char *)hash);
		if(memcmp(&inputData[index], hash, DMP_SHA256_DIGEST_STRING_LENGTH) != 0){
			OutputErrorLog("hash mismatch. " + modelPath);
			return false;
		}
	}
	modelData->modelData.clear();
	modelData->modelData.insert(modelData->modelData.end(), inputData.begin()+modelIndex, inputData.end());
	return true;
}

bool CInference::CheckDevice(){
	const std::ifstream ifs(HAILO_DEVICE);								//!< ﾌｧｲﾙの存在確認用
	if(ifs.is_open() == false){
		ERR("Hailo Device not found %s", MOUNT_SDCARD_SRC_PATH);
		return false;
	}
	return true;
}


CInference::ThreadData::ThreadData(	pthread_mutex_t&		inference_mutex,
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
									int&					hailo_error)
									:m_inference_mutex(inference_mutex),
									m_end_inference_thread(end_inference_thread),
									m_detect(detect),
									m_temp_req(temp_req),
									m_hailo_ts0(hailo_ts0),
									m_hailo_ts1(hailo_ts1),
									m_image(image),
									m_image_1(image_1),
									m_boundingbox(boundingbox),
									m_infer_req(infer_req),
									m_infer_comp(infer_comp),
									m_infer_fps(infer_fps),
									m_hailo_error(hailo_error){

}
