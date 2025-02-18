//--------------------------------------------------
//! @file Recoder.hpp
//! @brief 動画保存ｸﾗｽ定義
//--------------------------------------------------

#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include "Global.h"
#include "Sysdef.h"
#include "LinuxUtility.hpp"

using namespace std;

//--------------------------------------------------
//! @brief 動画保存ｸﾗｽ
//!
//! 画像を渡すことで別ｽﾚｯﾄﾞにて動画保存を実施する
//--------------------------------------------------
class CRecoder{
public:
	//--------------------------------------------------
	//! @brief 初期化用構造体
	//--------------------------------------------------
	struct InitParameter{
		std::string m_addName="";							//!< ﾌｧｲﾙ名への付加名称
		ushort	m_recode_minutes=10;						//!< 保存時間設定
		ushort	m_recode_fps=1;								//!< 保存FPS設定
	};
	//--------------------------------------------------
	//! @brief ﾚｺｰﾀﾞｰ情報構造体
	//--------------------------------------------------
	struct RecoderInfo{
		cv::Mat				image;							//!< 画像
		int					move_state;						//!< 移動方向ｽﾃｰﾄ
		int					reverse_sig;					//!< ﾘﾊﾞｰｽ信号
	};
	//--------------------------------------------------
	//! @brief kenti情報構造体
	//--------------------------------------------------
	struct Bbox_Info{
		int judge;
		BboxInfo box;
	};
	//--------------------------------------------------
	//! @brief ｺﾝｽﾄﾗｸﾀ
	//--------------------------------------------------
	CRecoder();
	//--------------------------------------------------
	//! @brief ﾃﾞｽﾄﾗｸﾀ
	//--------------------------------------------------
	~CRecoder();
	//--------------------------------------------------
	//! @brief 初期化
	//!
	//! @param[in]		initParameter	初期化用ﾊﾟﾗﾒｰﾀ
	//! @retval			true			初期化成功
	//! @retval			false			初期化失敗
	//--------------------------------------------------
	bool Init(const InitParameter& initParameter);
	//--------------------------------------------------
	//! @brief ﾌﾚｰﾑ追加
	//!
	//! 別ｽﾚｯﾄﾞにて動画へﾌﾚｰﾑを追加する
	//! @param[in]		image			追加する対象の画像
	//! @param[in]		move_st			追加する対象の移動方向ｽﾃｰﾄ
	//! @param[in]		reverse_sig		追加する対象のﾘﾊﾞｰｽ信号
	//--------------------------------------------------
	void AddFrame(const cv::Mat& image, int move_st, int reverse_sig,vector<BboxJudge> &boxjudge);
	//--------------------------------------------------
	//! @brief ｴﾗｰが発生したか？
	//! @retval	true	ｴﾗｰが発生した
	//! @retval	false	ｴﾗｰが発生していない
	//--------------------------------------------------
	bool IsErrorOccurred(void);

	//--------------------------------------------------
	//! @brief 録画スレッドの解放
	//! @retval	true	ｴﾗｰが発生した
	//! @retval	false	ｴﾗｰが発生していない
	//--------------------------------------------------
	bool Finalize(void);

private:
	//--------------------------------------------------
	//! @brief 動画保存ｽﾚｯﾄﾞ生成
	//!
	//! @retval	0		成功
	//! @retval	0以外	失敗。pthread_createの戻り値
	//--------------------------------------------------
	int CreateRecoderThread(void);

	//--------------------------------------------------
	//! @brief 動画保存ｽﾚｯﾄﾞ終了
	//!
	//! @retval	0		成功
	//! @retval	0以外	失敗。pthread_joinの戻り値
	//--------------------------------------------------
	int DeleteRecoderThread(void);

	//--------------------------------------------------
	//! @brief 動画保存ｽﾚｯﾄﾞ
	//!
	//! @param[in]		targ			未使用
	//! @retval	NULL
	//--------------------------------------------------
	static void* RecoderThreadFunc(void *targ);

	//--------------------------------------------------
	//! @brief ｽﾚｯﾄﾞ用ﾃﾞｰﾀ
	//--------------------------------------------------
	struct ThreadData{
		//--------------------------------------------------
		//! @brief ｺﾝｽﾄﾗｸﾀ
		//!
		//! ﾒﾝﾊﾞ変数に参照ﾃﾞｰﾀがあるためｺﾝｽﾄﾗｸﾀにて設定
		//! 推論結果に応じてﾃﾞｰﾀの更新をするため、ここで渡した参照は排他処理無しに外部で触らないこと
		//! @param[in,out]	info					追加情報
		//! @param[in,out]	bbox					追加情報
		//! @param[in,out]	recoder_mutex			推論Mutex
		//! @param[in,out]	end_recoder_thread	推論ｽﾚｯﾄﾞ終了ﾌﾗｸﾞ
		//--------------------------------------------------
		ThreadData(vector<RecoderInfo>& info,
					vector<Bbox_Info>& bbox,
					pthread_mutex_t&  recoder_mutex,
					int&	end_recoder_thread,
					bool&	isErrorOccurred);
		//--------------------------------------------------
		//! @brief ﾌｧｲﾙ名への付加名称を設定
		//! @param[in]		fldr		ﾌｫﾙﾀﾞ
		//--------------------------------------------------
		inline void SetAddName(const std::string& name){
			m_addName = name;
		}
		//--------------------------------------------------
		//! @brief 保存時間を設定
		//! @param[in]		fldr		ﾌｫﾙﾀﾞ
		//--------------------------------------------------
		inline void SetRecodeMinute(const ushort& minutes){
			m_recode_minutes = minutes;
		}
		//--------------------------------------------------
		//! @brief 保存FPSを設定
		//! @param[in]		fldr		ﾌｫﾙﾀﾞ
		//--------------------------------------------------
		inline void SetRecodeFps(const ushort& fps){
			m_recode_fps = fps;
		}
		vector<RecoderInfo>&	m_info;					//!< 追加対象情報
		vector<Bbox_Info>&  m_boxjudge;					//!< 追加対象情報
		pthread_mutex_t&	m_recoder_mutex;			//!< 動画保存Mutex
		int&	m_end_recoder_thread;					//!< 動画保存ｽﾚｯﾄﾞ終了ﾌﾗｸﾞ
		// 内部ﾊﾟﾗﾒｰﾀ
		std::string			m_addName;					//!< ﾌｧｲﾙ名への付加名称
		ushort				m_recode_minutes;			//!< 保存時間設定
		ushort				m_recode_fps;				//!< 保存FPS設定
		bool&				m_isErrorOccurred;			//!< ｴﾗｰが発生したか？
	};

	ThreadData	m_threadData;					//!< ｽﾚｯﾄﾞ用ﾃﾞｰﾀ
	vector<RecoderInfo> m_info;							//!< 追加対象情報
	vector<Bbox_Info> m_boxjudge;						//!< 追加対象情報
	mutable pthread_mutex_t  m_recoder_mutex;			//!< 動画保存Mutex
	mutable pthread_t m_recoder_thread;					//!< 動画保存ｽﾚｯﾄﾞｵﾌﾞｼﾞｪｸﾄ
	int	m_end_recoder_thread;							//!< 動画保存ｽﾚｯﾄﾞ終了ﾌﾗｸﾞ
	uint64_t 				m_recodingIntervalFrame;	//!< 記録するﾌﾚｰﾑの間隔
	TP 						m_addframe_sttime;			// フレーム取得時間
	uint64_t 				m_addframe_remainder;		// フレーム取得時間補正値
	bool 					m_first_addflame;			// フレーム取得初回判定
	bool					m_isErrorOccurred;			//!< ｴﾗｰが発生したか？
};


//--------------------------------------------------
//! @brief 時刻に合わせたﾚｺｰﾀﾞｰのﾃﾞｨﾚｸﾄﾘを作成
//! @param[in]		time				ﾃﾞｨﾚｸﾄﾘ名に使用する時刻情報
//! @retval	true	作成成功（既に存在していた場合を含む）
//! @retval	false	作成失敗
//--------------------------------------------------
bool CreateRecoderDir(const time_t time);

//--------------------------------------------------
//! @brief 時刻に合わせたﾚｺｰﾀﾞｰのﾃﾞｨﾚｸﾄﾘﾊﾟｽを取得
//! @param[in]		time				ﾃﾞｨﾚｸﾄﾘ名に使用する時刻情報
//! @return	ﾃﾞｨﾚｸﾄﾘのﾌﾙﾊﾟｽ
//--------------------------------------------------
std::string GetRecoderDirPath(const time_t time);

//--------------------------------------------------
//! @brief 最古ﾌｧｲﾙ削除ｽﾚｯﾄﾞ生成
//--------------------------------------------------
int create_remove_thread(void);

//--------------------------------------------------
//! @brief 最古ﾌｧｲﾙ削除ｽﾚｯﾄﾞ終了
//--------------------------------------------------
int delete_remove_thread(void);
