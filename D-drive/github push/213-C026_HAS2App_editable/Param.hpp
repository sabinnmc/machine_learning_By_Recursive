//--------------------------------------------------
//! @file Param.hpp
//! @brief ﾊﾟﾗﾒｰﾀｱｸｾｽﾓｼﾞｭｰﾙ
//--------------------------------------------------

#pragma once
#include <vector>
#include <string>

#include "Sysdef.h"
#include "Detect.hpp"

//--------------------------------------------------
//! @brief ｺﾝﾌｨｸﾞﾌｧｲﾙのﾃﾞｰﾀ
//--------------------------------------------------
class ConfigData{
public:
	//----------------------------型宣言----------------------------

	//--------------------------------------------------
	//! @brief 推論ﾀｲﾌﾟごとの設定ﾃﾞｰﾀ
	//--------------------------------------------------
	struct InferenceTypeData{
		//--------------------------------------------------
		//! @brief 入力ﾃﾞｰﾀの種類
		//--------------------------------------------------
		enum class eSourceType{
			DEVICE,																//!< ｶﾒﾗ映像
			FILE,																//!< 動画ﾌｧｲﾙ
		};

		std::string			m_modelPath="";										//!< 推論ﾓﾃﾞﾙのﾊﾟｽ
		std::string			m_modelBackupPath="";								//!< 推論ﾓﾃﾞﾙ（ﾊﾞｯｸｱｯﾌﾟ）のﾊﾟｽ
		eSourceType			m_sourceType=eSourceType::FILE;						//!< 入力ﾃﾞｰﾀの種類
		int					m_sourceDeviceNo=-1;								//!< 入力ﾃﾞﾊﾞｲｽのNo。  ｶﾒﾗ映像を推論する場合のみ使用
		std::string			m_sourceFilePath="";								//!< 入力動画ﾌｧｲﾙのﾊﾟｽ。動画ﾌｧｲﾙを推論する場合のみ使用
		double				m_scoreThreshold=-1.;								//!< ｽｺｱの閾値
		double				m_nmsThreshold=-1.;									//!< NMSの閾値
		DetectSetting		m_detectSetting;
	};

	//--------------------------------------------------
	//! @brief 録画関連の設定ﾃﾞｰﾀ
	//--------------------------------------------------
	struct RecoderData{
		ushort	m_beforeSeconds;												//!< 保存ﾄﾘｶﾞ前の保存時間（秒）
		ushort	m_afterSeconds;													//!< 保存ﾄﾘｶﾞ後の保存時間（秒）
		ushort	m_FPS;															//!< 保存FPS設定
	};
	
	//--------------------------------------------------
	//! @brief ﾃﾞﾊﾞｯｸﾞ関連の設定ﾃﾞｰﾀ
	//--------------------------------------------------
	struct DebugData{
		bool	m_isEnvironmentTesting;											//!< 環境試験か？
		ulong	m_autoRebootSeconds = 0;										//!< 自動再起動する時間（秒）
		float	m_autoRebootMemUsed = 0.;										//!< 自動再起動するメモリ使用量（%）
	};


	//---------------------------関数宣言---------------------------

	//--------------------------------------------------
	//! @brief ｺﾝｽﾄﾗｸﾀ
	//--------------------------------------------------
	ConfigData();

	//--------------------------------------------------
	//! @brief 推論ﾀｲﾌﾟごとの設定ﾃﾞｰﾀを格納
	//! @param[in]		type				推論ﾀｲﾌﾟ
	//! @param[in]		data				推論ﾀｲﾌﾟごとの設定ﾃﾞｰﾀ
	//--------------------------------------------------
	inline void SetInferenceTypeData(eInferenceType type, const InferenceTypeData& data){
		m_inferenceTypeDatas[static_cast<int>(type)] = data;
	}

	//--------------------------------------------------
	//! @brief 推論ﾀｲﾌﾟごとの設定ﾃﾞｰﾀを取得
	//! @param[in]		type				推論ﾀｲﾌﾟ
	//! @return	推論ﾀｲﾌﾟごとの設定ﾃﾞｰﾀ
	//--------------------------------------------------
	inline const InferenceTypeData& GetInferenceTypeData(eInferenceType type)const{
		return m_inferenceTypeDatas[static_cast<int>(type)];
	}

	//--------------------------------------------------
	//! @brief 録画関連の設定ﾃﾞｰﾀを格納
	//! @param[in]		data				録画関連の設定ﾃﾞｰﾀ
	//--------------------------------------------------
	inline void SetRecoderData(const RecoderData& data){
		m_recoderData = data;
	}

	//--------------------------------------------------
	//! @brief 録画関連の設定ﾃﾞｰﾀを取得
	//! @return	録画関連の設定ﾃﾞｰﾀ
	//--------------------------------------------------
	inline const RecoderData& GetRecoderData()const{
		return m_recoderData;
	}

	//--------------------------------------------------
	//! @brief ﾃﾞﾊﾞｯｸﾞ関連の設定ﾃﾞｰﾀを格納
	//! @param[in]		data				ﾃﾞﾊﾞｯｸﾞ関連の設定ﾃﾞｰﾀ
	//--------------------------------------------------
	inline void SetDebugData(const DebugData& data){
		m_debugData = data;
	}

	//--------------------------------------------------
	//! @brief ﾃﾞﾊﾞｯｸﾞ関連の設定ﾃﾞｰﾀを取得
	//! @return	ﾃﾞﾊﾞｯｸﾞ関連の設定ﾃﾞｰﾀ
	//--------------------------------------------------
	inline const DebugData& GetDebugData()const{
		return m_debugData;
	}
	
	//--------------------------------------------------
	//! @brief 自動再起動する時間（秒）を取得
	//! @return	自動再起動する時間（秒）
	//--------------------------------------------------
	inline const ulong& GetDebugAutoRebootSeconds()const{
		return GetDebugData().m_autoRebootSeconds;
	}

	//--------------------------------------------------
	//! @brief 自動再起動するメモリ使用量（%）を取得
	//! @return	自動再起動するメモリ使用量（%）
	//--------------------------------------------------
	inline const float& GetDebugAutoRebootMemUsed()const{
		return GetDebugData().m_autoRebootMemUsed;
	}

private:
	std::vector<InferenceTypeData>	m_inferenceTypeDatas;						//!< 推論ﾀｲﾌﾟごとの設定ﾃﾞｰﾀ
	RecoderData						m_recoderData;								//!< 録画関連の設定ﾃﾞｰﾀ
	DebugData						m_debugData;								//!< ﾃﾞﾊﾞｯｸﾞ関連の設定ﾃﾞｰﾀ
};
