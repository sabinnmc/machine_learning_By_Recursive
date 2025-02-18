//--------------------------------------------------
//! @file Param.cpp
//! @brief ﾊﾟﾗﾒｰﾀｱｸｾｽﾓｼﾞｭｰﾙ
//--------------------------------------------------

#include "Param.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/optional.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include "Sysdef.h"
#include "Global.h"

using namespace std;
using namespace boost::property_tree;

/*===	SECTION NAME		===*/


/*===	user definition		===*/


/*===	pragma variable	===*/


/*===	external variable	===*/


/*===	external function	===*/


/*===	public function prototypes	===*/


/*===	static function prototypes	===*/


/*===	static typedef variable		===*/
typedef boost::escaped_list_separator<char>
BOOST_ESCAPED_LIST_SEP;
typedef boost::tokenizer< boost::escaped_list_separator<char> >
BOOST_TOKENIZER_ESCAPED_LIST;

/*===	static variable		===*/


/*===	constant variable	===*/


/*===	global variable		===*/


ConfigData::ConfigData(){
	m_inferenceTypeDatas.resize(MAX_INFERENCE_TYPE);
}

/*-------------------------------------------------------------------
	FUNCTION :	GetVal()
	EFFECTS  :	設定ﾌｧｲﾙの値読み出し  Read the value of the configuration file
	NOTE	 :	見つからない場合はﾃﾞﾌｫﾙﾄ設定を返却   If not found, return default settings
-------------------------------------------------------------------*/
template<class T>
T GetVal(const ptree& pt, const string& param, const T& defaultVal){
	if (auto value = pt.get_optional<T>(param)) {
		return value.get();
	}
	// 読み出し失敗時はﾃﾞﾌｫﾙﾄ値設定
	return defaultVal;
}

/*-------------------------------------------------------------------
	FUNCTION :	ReadAppConf()
	EFFECTS  :	ｱﾌﾟﾘ設定ﾌｧｲﾙ読み出し
	NOTE	 :	ｱﾌﾟﾘ設定ﾌｧｲﾙを読み出し変数へ展開
-------------------------------------------------------------------*/
int	ReadAppConf(void)
{
	int result = 0;
	ptree pt;
	char path[256];
	std::string cfg_file;

	sprintf(path,"%s%s",CONFIG_FLDR,CONFIG_FILE);
	cfg_file = path;

	read_ini(cfg_file, pt);

	// システム設定
	{
		ConfigData::DebugData data;
		g_ulDebugLog0 = GetVal<ulong>(pt, "debug.log0", 0);
		g_usTempMoni = GetVal<ushort>(pt, "debug.tempmoni", 0);
		g_usWattageMoni = GetVal<ushort>(pt, "debug.wattagemoni", 0);
		g_debugOutputPath = GetVal<string>(pt, "debug.output_path", "");
		data.m_isEnvironmentTesting = (GetVal<ulong>(pt, "debug.environment_testing", 0) == 1);
		data.m_autoRebootSeconds = GetVal<ulong>(pt, "debug.auto_reboot_seconds", 0);
		data.m_autoRebootMemUsed = GetVal<float>(pt, "debug.auto_reboot_memused", 0.0f);
		g_configData.SetDebugData(data);
	}
	// アプリケーション設定
	{
		g_fCoreTempThresh = GetVal<float>(pt, "setting.tempthresh", 98.0f);
		g_usRecoderMinutes = GetVal<ushort>(pt, "setting.recoder_minutes", 10);
		g_usRecoderFps = GetVal<ushort>(pt, "setting.recoder_fps", 1);
		g_usRecoderEna =  GetVal<ushort>(pt, "setting.recoder_ena", 1);
		g_usInferEna =  GetVal<ushort>(pt, "setting.infer_ena", 1);
		g_usAccOffTimer =  GetVal<ushort>(pt, "setting.accoff_timer", 3600);
	}
	//LUT
	{
		for(size_t i=0;i<LUT_DATA_MAX;++i){
			auto GetName = [&](string name) {return name + to_string(i);};
			g_LutConfig[i][0].candata =  i;
			g_LutConfig[i][0].filename = GetVal<string>(pt, GetName("lut.frontfile_"), "");
			g_LutConfig[i][1].filename = GetVal<string>(pt, GetName("lut.rearfile_"), "");
		}
		auto GetName = [&](string name) {return name + to_string(0);};
		g_LutConfigDETECT[0].filename = GetVal<string>(pt, GetName("lut.f_detectfile_"), "");

		for(size_t i=0; i<LUT_PTO_TYPE; ++i) {
			for(size_t j=0; j<MFD_FAVORITE_MAX; ++j) {
				for(size_t k=0; k<MONITOR_MAX; ++k) {
					auto GetName = [&](string name0, string name1, string name2) {return name0 + to_string(i) + name1 + to_string(j) + name2 + to_string(k);};
					g_lut_favorite_table[i][j][k] = GetVal<int>(pt, GetName("lut.pto", "_favorite", "_moni"), (int)j);
				}
			}
		}
	}

	// 推論の設定
	{
		ConfigData::InferenceTypeData data;
		data.m_modelPath = GetVal<string>(pt, "ai.model_path", "");
		data.m_modelBackupPath = GetVal<string>(pt, "ai.model_backup_path", "");
		// captureに数値が指定されていたらﾃﾞﾊﾞｲｽ、文字列ならﾌｧｲﾙ。未設定ならﾌｧｲﾙが存在しない扱い
		if(auto sourceDeviceNo = GetVal<int>(pt, "ai.capture", -1); sourceDeviceNo != -1){
			data.m_sourceType = ConfigData::InferenceTypeData::eSourceType::DEVICE;
			data.m_sourceDeviceNo = sourceDeviceNo;
		}else{
			data.m_sourceType = ConfigData::InferenceTypeData::eSourceType::FILE;
			data.m_sourceFilePath = GetVal<string>(pt, "ai.capture", "");
		}
		data.m_scoreThreshold = GetVal<double>(pt,"ai.score_threshold", 0.2);
		data.m_nmsThreshold = GetVal<double>(pt, "ai.nms_threshold", 0.5);
		data.m_detectSetting.m_minBoxW = GetVal<sint>(pt, "ai.box_min_w_", 0);
		data.m_detectSetting.m_minBoxH = GetVal<sint>(pt, "ai.box_min_h_", 0);
		data.m_detectSetting.m_maxBoxW = GetVal<sint>(pt, "ai.box_max_w_", IMAGE_WIDTH_FHD);
		data.m_detectSetting.m_maxBoxH = GetVal<sint>(pt, "ai.box_max_h_", IMAGE_HEIGHT_FHD);
		g_configData.SetInferenceTypeData(static_cast<eInferenceType>(0), data);
		g_areabinFilePath = GetVal<string>(pt, "area.detection", "");
	}

	result = 1;
	return(result);
}

/*-------------------------------------------------------------------
	FUNCTION :	ReadFavoriteLUTConf()
	EFFECTS  :	視点変換LUT設定の読み出し
	NOTE	 :	ReadAppConf()のLUT部分だけ抜粋
-------------------------------------------------------------------*/
int	ReadFavariteLUTConf(void)
{
	int result = 0;
	ptree pt;
	char path[256];
	std::string cfg_file;

	sprintf(path,"%s%s",CONFIG_FLDR,CONFIG_FILE);
	cfg_file = path;

	read_ini(cfg_file, pt);

	//LUT
	for(size_t i=0; i<LUT_PTO_TYPE; ++i) {
		for(size_t j=0; j<MFD_FAVORITE_MAX; ++j) {
			for(size_t k=0; k<MONITOR_MAX; ++k) {
				auto GetName = [&](string name0, string name1, string name2) {return name0 + to_string(i) + name1 + to_string(j) + name2 + to_string(k);};
				g_lut_favorite_table[i][j][k] = GetVal<int>(pt, GetName("lut.pto", "_favorite", "_moni"), (int)j);
			}
		}
	}
	result = 1;
	return(result);
}


/*-------------------------------------------------------------------
	FUNCTION :  UpdateLutFavoriteTable()
	EFFECTS  :	LUTお気に入りテーブルの更新
	NOTE	 :
-------------------------------------------------------------------*/
void	UpdateConfigFileforLutFavoriteTable(void)
{
	ptree pt;
	char path[256];
	std::string cfg_file;

	sprintf(path,"%s%s",CONFIG_FLDR, CONFIG_FILE);
	cfg_file = path;

	read_ini(cfg_file, pt);

	// お気に入りテーブルの変更
	for(size_t i=0; i<LUT_PTO_TYPE; ++i) {
		for(size_t j=0; j<MFD_FAVORITE_MAX; ++j) {
			for(size_t k=0; k<MONITOR_MAX; ++k) {
				auto GetName = [&](string name0, string name1, string name2) {return name0 + to_string(i) + name1 + to_string(j) + name2 + to_string(k);};
				pt.put(GetName("lut.pto", "_favorite", "_moni"), g_lut_favorite_table[i][j][k]);
			}
		}
	}
	write_ini(cfg_file, pt);
}
