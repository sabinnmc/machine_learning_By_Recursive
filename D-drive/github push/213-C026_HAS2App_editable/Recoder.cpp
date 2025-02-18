//--------------------------------------------------
//! @file Recoder.cpp
//! @brief 動画保存ｸﾗｽ実装
//--------------------------------------------------

#include "Recoder.hpp"

#include <iostream>
#include <iterator>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <sys/stat.h>

#include "Sysdef.h"
#include "LinuxUtility.hpp"
//#include "common.hpp"
#include "TransForm.hpp"
#include "ExternalMemoryUtility.hpp"
#include "Global.h"

extern void error_info_save(ErrorInfoType);						// utility.cpp
extern void error_info_send_req(void);							// utility.cpp

extern ErrorInfoType errorinfo;									// ｴﾗｰ情報


#define RECODER_INPUT_FPS					30												//!< 入力画像FPS
#define RECODER_SECONDS						60												//!< 秒

static pthread_t s_remove_thread;												// 最古日付ﾌｫﾙﾀﾞ削除ｽﾚｯﾄﾞｵﾌﾞｼﾞｪｸﾄ


CRecoder::CRecoder():m_threadData(m_info, m_boxjudge, m_recoder_mutex, m_end_recoder_thread, m_isErrorOccurred),
						m_end_recoder_thread(1){

}
CRecoder::~CRecoder(){
	//	DeleteRecoderThread();
}

bool CRecoder::Init(const InitParameter& initParameter){
	// ﾙｰﾄﾌｫﾙﾀﾞ設定
	m_threadData.SetAddName(initParameter.m_addName);
	// 保存時間設定
	m_threadData.SetRecodeMinute(initParameter.m_recode_minutes);
	// 保存FPS設定
	m_threadData.SetRecodeFps(initParameter.m_recode_fps);
	// 保存FPS設定
	m_recodingIntervalFrame = 1000000 / static_cast<uint64_t>(initParameter.m_recode_fps);	// 何μs毎のインターバル

	// 初期化
	m_first_addflame = true;
	m_addframe_sttime = getTime();
	m_addframe_remainder = 0;
	m_isErrorOccurred = false;

	// 動画保存ｽﾚｯﾄﾞ起動
	CreateRecoderThread();

	return true;
}

void CRecoder::AddFrame(const cv::Mat& image, int move_st, int reverse_sig,vector<BboxJudge> &boxjudge){
	RecoderInfo info;
	Bbox_Info bbox;
	int i;
	int offset = 0;
	if(m_first_addflame){
		m_addframe_sttime = getTime();
		m_addframe_remainder = m_recodingIntervalFrame;
		m_first_addflame = false;
	}

	// FPS調整
	uint64_t diff = getTimeDiff(m_addframe_sttime,getTime()) + m_addframe_remainder;
	if(diff <= m_recodingIntervalFrame){
		return;
	}
	m_addframe_remainder = diff - m_recodingIntervalFrame;	// 遅れを余りとして保持
	if( m_addframe_remainder > m_recodingIntervalFrame ){
		m_addframe_remainder = m_recodingIntervalFrame;		// 余りはフレーム間インターバルを最大とする
	}

	m_addframe_sttime = getTime();							// 基準更新

	info.image = image.clone();
	info.move_state = move_st;
	info.reverse_sig = reverse_sig;

	for( i = 0; i < (int)boxjudge.size(); i++ ){
		if(boxjudge[i].box.flg == 0) offset = FISHEYE_FCOL_OFFSET;
		if(boxjudge[i].box.flg == 1) offset = FISHEYE_RCOL_OFFSET;
		bbox.box.x1 = boxjudge[i].box.x1 + offset;
		bbox.box.x2 = boxjudge[i].box.x2-boxjudge[i].box.x1;
		bbox.box.y1 = boxjudge[i].box.y1 + FISHEYE_ROW_OFFSET;
		bbox.box.y2 = boxjudge[i].box.y2 - boxjudge[i].box.y1;
		bbox.box.score = boxjudge[i].box.score;
		bbox.judge = boxjudge[i].judge;
		pthread_mutex_lock(&m_recoder_mutex);
		m_boxjudge.push_back(bbox);
		pthread_mutex_unlock(&m_recoder_mutex);
	}

	pthread_mutex_lock(&m_recoder_mutex);
	m_info.push_back(info);
	pthread_mutex_unlock(&m_recoder_mutex);
}

bool CRecoder::Finalize(void){
	int ret = DeleteRecoderThread();
	if (ret == 0) return false;
	else          return true;
}


bool CRecoder::IsErrorOccurred(void){
	bool iserroccrred;
	pthread_mutex_lock(&m_recoder_mutex);
	iserroccrred = m_isErrorOccurred;
	pthread_mutex_unlock(&m_recoder_mutex);
	return iserroccrred;
}


int CRecoder::CreateRecoderThread(void){
	m_end_recoder_thread = 0;
	m_info.clear();
	m_boxjudge.clear();
	pthread_mutex_init(&m_recoder_mutex, NULL);
	return pthread_create(&m_recoder_thread, NULL, RecoderThreadFunc, (void*)(&m_threadData));
}

int CRecoder::DeleteRecoderThread(void){
	if(m_end_recoder_thread == 1){
		return 0;
	}
	m_end_recoder_thread = 1;
	int ret = pthread_join(m_recoder_thread, NULL);
	return ret;
}



void* CRecoder::RecoderThreadFunc(void *targ){
	int size,boxsize,j,cnt;
	int end_recoder_thread;
	int add_frame = 0;
	std::string fname = "";
	std::string tname = "";
	cv::VideoWriter writer;

	cv::Mat image;
	//int move_st,reverse_sig;
	ThreadData*	threadData = (ThreadData*)targ;
	static const int recoderFrameInterval = (RECODER_INPUT_FPS/threadData->m_recode_fps);
	static const int recoderFileFrame = (threadData->m_recode_minutes*RECODER_SECONDS*RECODER_INPUT_FPS)/recoderFrameInterval;


	while (true)
	{
		usleep(1000);
		pthread_mutex_lock(&threadData->m_recoder_mutex);
		size = threadData->m_info.size();
		boxsize = threadData->m_boxjudge.size();
		end_recoder_thread = threadData->m_end_recoder_thread;
		pthread_mutex_unlock(&threadData->m_recoder_mutex);
		if(end_recoder_thread){
			break;
		}
		// 追加画像あり
		if( size ){
			vector<Bbox_Info> hdetect;											// hitokenti
			// 画像受信
			pthread_mutex_lock(&threadData->m_recoder_mutex);
			image = threadData->m_info[0].image.clone();
			//move_st = threadData->m_info[0].move_state;
			//reverse_sig = threadData->m_info[0].reverse_sig;
			if(boxsize > 0) hdetect = threadData->m_boxjudge;
			threadData->m_info.erase(threadData->m_info.begin());
			threadData->m_boxjudge.clear();
			pthread_mutex_unlock(&threadData->m_recoder_mutex);

			if(add_frame == 0 ){
				if(SSDSecureFreeSpace(SSD_SAVE_CHECK_FREE_SIZE)==1){
					// 動画生成
					auto fourcc	= cv::VideoWriter::fourcc('m', 'p', '4', 'v'); // ビデオフォーマットの指定( ISO MPEG-4 / .mp4)
					//const auto fourcc	= cv::VideoWriter::fourcc('V', 'P', '8', '0');			//!< ﾋﾞﾃﾞｵﾌｫｰﾏｯﾄの指定
					auto fps = (float)threadData->m_recode_fps;
					auto width   = image.cols;
					auto height  = image.rows;
					const time_t nowTime = time(NULL);							//!< 現在時刻

					if(CreateRecoderDir(nowTime)){
						char date[64];																//!< 時刻情報（文字列）
						strftime(date, sizeof(date), "%Y%m%d%H%M%S", localtime(&nowTime));
						const string subDir = GetRecoderDirPath(nowTime);
						//fname = subDir + "/" + date + ".webm";
						fname = subDir + "/" + date + ".mp4";
						tname = subDir + "/" + date + ".txt";
						LOG(LOG_RECODER_DEBUG,"Recode file create :%s",fname.c_str());
						writer.open(fname.c_str(), fourcc, fps, cv::Size(width, height));
						std::ofstream ofs(tname,ios::app);
						ofs.close();
						pthread_mutex_lock(&threadData->m_recoder_mutex);
						threadData->m_isErrorOccurred = false;
						pthread_mutex_unlock(&threadData->m_recoder_mutex);

					}
					else{
						ERR("Create date folder failed.");
						pthread_mutex_lock(&threadData->m_recoder_mutex);
						threadData->m_isErrorOccurred = true;
						pthread_mutex_unlock(&threadData->m_recoder_mutex);
					}
				}
			}
			if(writer.isOpened()){						// 保存中の動画あり

				LOG(LOG_RECODER_DEBUG,"Write frame frame to %s",fname.c_str());
				// 動画保存
				add_frame++;
				writer << image;
				if(hdetect.size()>0){
					cnt = 0;
					std::ofstream ofs(tname,ios::app);
					for(j = 0; j < (int)hdetect.size(); j++ ){
						if((hdetect[j].judge & YOLO_PERSON_DANGER_JUDGE ) ||( hdetect[j].judge & YOLO_PERSON_DETECT_JUDGE )){
							if(cnt == 0) ofs << add_frame << std::endl;
				 			ofs << "1" << "," << hdetect[j].box.x1 << "," << hdetect[j].box.x2 << "," << hdetect[j].box.y1 << "," << hdetect[j].box.y2 << "," << std::setprecision(2) << hdetect[j].box.score << std::endl;
							cnt++;
						}
					}
					ofs.close();
					hdetect.clear();
				}
				if(add_frame >= recoderFileFrame ){
					LOG(LOG_RECODER_DEBUG,"Recode file close framecount:%d",add_frame);
					writer.release();							// 動画ｸﾛｰｽﾞ
					add_frame = 0;
				}
			}
		}
	}
	if(writer.isOpened()){						// 保存中の動画あり
		writer.release();						// 動画ｸﾛｰｽﾞ
		LOG(LOG_RECODER_DEBUG,"Recode file close framecount:%d",add_frame);
	}
	sync_exec();
	LOG(LOG_RECODER_DEBUG,"Recode thread close!!");

	return NULL;
}

CRecoder::ThreadData::ThreadData(vector<RecoderInfo>& info,
									vector<Bbox_Info>& bbox,
									pthread_mutex_t&  recoder_mutex,
									int&	end_recoder_thread,
									bool&	isErrorOccurred)
									:m_info(info),
									m_boxjudge(bbox),
									m_recoder_mutex(recoder_mutex),
									m_end_recoder_thread(end_recoder_thread),
									m_isErrorOccurred(isErrorOccurred){

}

bool CreateRecoderDir(const time_t time){
	// SDｶｰﾄﾞがなければｴﾗｰ
	if( IsMountedSD() == false ){
		ERR("CreateRecoderDir() SD Card Not Insert");
		return false;
	}

#if 1
	// 親ﾌｫﾙﾀﾞを作成
	// `stat`を使用してﾃﾞｨﾚｸﾄﾘの存在確認をしてから`mkdir`する方法も考えられるが、別ｽﾚｯﾄﾞでこの関数を同時に呼び出された場合、
	// 「`stat`の時点ではﾃﾞｨﾚｸﾄﾘが存在しないが`mkdir`の時点でﾃﾞｨﾚｸﾄﾘが存在しているｴﾗｰになる」場合がありうる
	// そのため`stat`を使用するのではなく、`mkdir`時にEEXISTをﾁｪｯｸする
	{
		const string dir = MOUNT_REDODER_DIR + "/" + RECODER_DIR;
		const int ret = mkdir(dir.c_str(),0777);
		// 別ｽﾚｯﾄﾞでmkdirされた場合、「statの時点ではﾃﾞｨﾚｸﾄﾘが存在しないがmkdirの時点でﾃﾞｨﾚｸﾄﾘが存在しているｴﾗｰになる」場合があるためEEXISTをﾁｪｯｸ
		if( ret != 0 && errno != EEXIST){
			ERR("mkdir failed. path=%s. errno=%d : %s", dir.c_str(),
														errno,
														strerror(errno));
			return false;
		}
	}

	// 日付のﾌｫﾙﾀﾞを作成
	{
		const string subDir = GetRecoderDirPath(time);
		const int ret = mkdir(subDir.c_str(),0777);
		if( ret != 0 && errno != EEXIST){
			ERR("mkdir failed. path=%s. errno=%d : %s", subDir.c_str(),
														errno,
														strerror(errno));
			return false;
		}
	}

	return true;

#else	// GCC8.1以降なら以下のような記載ができる
	// ただし、別ｽﾚｯﾄﾞで同時に呼び出された場合のｹｱは追加で必要（ﾃﾞｨﾚｸﾄﾘ存在ﾁｪｯｸの方法を変更しないといけない）
#include <filesystem>

	const string dir = GetRecoderDirPath(time);									//!< 作成するﾃﾞｨﾚｸﾄﾘﾊﾟｽ

	// ﾃﾞｨﾚｸﾄﾘが存在していれば作成の必要なし
	try{
		if(std::filesystem::exists(dir)){
			return true;
		}
	}
	catch(const std::filesystem::filesystem_error& e){
		ERR("CreateRecoderDir() failed exists. path=%s. error=%s.", dir.c_str(), e.what());
		return false;
	}

	// ﾃﾞｨﾚｸﾄﾘ作成
	try{
		if(std::filesystem::create_directories(dir)){
			return true;
		}else{
			ERR("CreateRecoderDir() failed create_directories. path=%s", dir.c_str());
			return false;
		}
	}
	catch(const std::filesystem::filesystem_error& e){
		ERR("CreateRecoderDir() failed create_directories. path=%s. error=%s.", dir.c_str(), e.what());
		return false;
	}
#endif
}


/*-------------------------------------------------------------------
  FUNCTION :	scandir_select()
  EFFECTS  :	ﾃﾞｨﾚｸﾄﾘ検索時ﾃﾞｨﾚｸﾄﾘｴﾝﾄﾘ排除
  NOTE	 :	ﾃﾞｨﾚｸﾄﾘ検索時ﾃﾞｨﾚｸﾄﾘｴﾝﾄﾘ排除
  -------------------------------------------------------------------*/
int scandir_select(const struct dirent *dir)
{
	if(dir->d_name[0] == '.'){
		return (0);
	}
	return (1);
}


/*-------------------------------------------------------------------
	FUNCTION :	datefolder_remove()
	EFFECTS  :	最古の日付フォルダ削除処理
	NOTE	 :	最古の日付フォルダ削除処理
-------------------------------------------------------------------*/
int datefolder_remove(void)
{
	std::string path = MOUNT_REDODER_DIR + "/" + RECODER_DIR;
	struct dirent **namelist;
	int r,i;
	int ret = 0;
	double start,end,diff;

	try{
		start = GetTimeMsec();

		// 最古の日付ﾌｫﾙﾀﾞを削除
		r = scandir(path.c_str(), &namelist, scandir_select, alphasort);		// ｽﾅｯﾌﾟｼｮｯﾄ保存ﾃﾞｨﾚｸﾄﾘ配下のﾃﾞｨﾚｸﾄﾘ情報取得
		if(r == -1) {
			ERR("scandir() Failed");
			return -1;
		}
		if( r > 0 ){															// 1件以上のﾃﾞｨﾚｸﾄﾘorﾌｧｲﾙ有
			LOG(LOG_COMMON_OUT,"date folder remove : %s/%s",path.c_str(),namelist[0]->d_name);
			std::string srcpath = path + "/" + namelist[0]->d_name;
			std::string mvpath = path + "/_" + namelist[0]->d_name;
			rename(srcpath.c_str(), mvpath.c_str());							// リネーム
			std::string command = "rm -rf " + mvpath;							// rmｺﾏﾝﾄﾞ作成
			ret = system(command.c_str());										// rmｺﾏﾝﾄﾞ実行
			for( i = 0; i < r; i++ ){
				free(namelist[i]);												// メモリ解放
			}
			free(namelist);														// メモリ解放
			if( ret == 0 ){
				end = GetTimeMsec();
				diff = end - start;
				LOG(LOG_COMMON_OUT,"date folder remove complete: %dmsec",(int)diff);
			}
			else{
				ERR("rm -rf failed");
				return -1;
			}
		}
		else{
			ERR("Delete file None");
			return -1;
		}
	}
	catch(const std::exception& ex){
		ERR("datefolder_remove err catch");
		return -1;
	}

	return 0;

}


/*-------------------------------------------------------------------
	FUNCTION :	remove_thread_func()
	EFFECTS  :	最古日付ﾌｫﾙﾀﾞ削除ｽﾚｯﾄﾞ関数
	NOTE	 :	最古日付ﾌｫﾙﾀﾞ削除ｽﾚｯﾄﾞ関数
-------------------------------------------------------------------*/
void *remove_thread_func(void *targ)
{
	if(datefolder_remove() == 0){										// 最古日付ﾌｫﾙﾀﾞ削除処理
		g_remove_result = 0;
	}
	else{
		g_remove_result = -1;
	}
	g_remove_comp = 1;

	return NULL;

}


/*-------------------------------------------------------------------
	FUNCTION :	create_remove_thread()
	EFFECTS  :	最古ﾌｧｲﾙ削除ｽﾚｯﾄﾞ生成
	NOTE	 :	最古ﾌｧｲﾙ削除ｽﾚｯﾄﾞを生成する
-------------------------------------------------------------------*/
int create_remove_thread(void)
{
	if(g_remove_req){													// 保存要求時は新たな保存要求は受け付けない
		return(-1);
	}
	g_remove_req = 1;													// 保存要求中
	g_remove_comp = 0;													// 保存完了ﾌﾗｸﾞ初期化
	g_remove_result = -1;												// 保存結果を失敗に初期化

	if( pthread_create(&s_remove_thread, NULL, remove_thread_func, NULL) != 0 ){
		g_remove_req = 0;
		ERR("date folder remove thread create failed");
		errorinfo.err3.data_save = 3;						// ﾃﾞｰﾀ保存異常
		// ｴﾗｰ保存
		error_info_save(errorinfo);
		error_info_send_req();
	}
	else{
		g_remove_result = 0;
		if( errorinfo.err3.data_save == 3 ){
			errorinfo.err3.data_save = 1;			// ﾃﾞｰﾀ保存異常解除
			error_info_send_req();
		}
	}

	return(0);
}


/*-------------------------------------------------------------------
	FUNCTION :	delete_remove_thread()
	EFFECTS  :	最古ﾌｧｲﾙ削除ｽﾚｯﾄﾞ終了
	NOTE	 :	最古ﾌｧｲﾙ削除ｽﾚｯﾄﾞを終了する
-------------------------------------------------------------------*/
int delete_remove_thread(void)
{
	int ret;
	if(g_remove_req){
		ret = pthread_join(s_remove_thread, NULL);
		if( ret != 0 ){
			ERR("delete_remove_thread pthread_join failed %d",ret);
			errorinfo.err3.data_save = 3;						// ﾃﾞｰﾀ保存異常
			// ｴﾗｰ保存
			error_info_save(errorinfo);
			error_info_send_req();
		}
	}
	return(0);
}


std::string GetRecoderDirPath(const time_t time){
	char date[64];
	strftime(date, sizeof(date), "%Y%m%d", localtime(&time));
	return MOUNT_REDODER_DIR + "/" + RECODER_DIR + "/" + date;
}
