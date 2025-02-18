//--------------------------------------------------
//! @file utility.cpp
//! @brief ﾕｰﾃｨﾘﾃｨﾓｼﾞｭｰﾙ
//!
//! 本流の機能に関わらない付属的な機能
//--------------------------------------------------
#include "LinuxUtility.hpp"

#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <string.h>
#include <jpeglib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string>
#include <sys/types.h>
#include <dirent.h>
#include <vector>
#include <csetjmp>
#include <signal.h>
#include <termios.h>
#include <mntent.h>
#include <chrono>
#include <iostream>
#include <stdint.h>

// for debug log
#include <fstream>
#include <deque>
#include <sys/stat.h>
#include <iomanip>
#include <ctime>


// ﾌﾚｰﾑﾊﾞｯﾌｧ系
#include <linux/fb.h>
#include <linux/kd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <mntent.h>

#include "Sysdef.h"
#include "Global.h"

#include <boost/filesystem.hpp>

using namespace std;

/*===	SECTION NAME		===*/


/*===	user definition		===*/


/*===	pragma variable	===*/


/*===	external variable	===*/
extern ErrorInfoType errorinfo;													// ｴﾗｰ情報
extern ErrorDetailType errordetail;												// ｴﾗｰ詳細情報

/*===	external function	===*/


/*===	public function prototypes	===*/


/*===	static function prototypes	===*/


/*===	static typedef variable		===*/

/*===	static variable		===*/


/*===	constant variable	===*/
ErrorInfoType curerrinfo;
ErrorDetailType curerrdetail;

static const unsigned long base_address = 0xff180000;

/*===	global variable		===*/
static struct fb_var_screeninfo g_fb0_var_info;									// HD-TVI(MIPI)
static struct fb_var_screeninfo g_fb1_var_info;									// HD-TVI(LVDA)
static struct fb_var_screeninfo g_fb2_var_info;									// HDMI
static string g_externalTempPath = "";											//!< 外部温度ﾃﾞﾊﾞｲｽのﾌｧｲﾙﾊﾟｽ
static string g_wattagePath = "";												//!< 消費電力確認ﾃﾞﾊﾞｲｽのﾌｧｲﾙﾊﾟｽ

/*===	user definition		===*/

//--------------------------------------------------
//! @brief RTCの初期化が完了しているか？
//! @retval	true	初期化完了
//! @retval	false	未完了
//--------------------------------------------------
bool IsCompleteInitRTC(void){
	const int ret = system("hwclock");
	if(ret == 0){
		return true;
	}
	ERR("RTC initialized. not Date set");
	return false;
}


/*-------------------------------------------------------------------
  FUNCTION :	sync_exec()
  EFFECTS  :	SYNC実行
  NOTE	 :	SYNC実行
  -------------------------------------------------------------------*/
void sync_exec(void)
{
	char command[100];
	int ret;

	sprintf(command,"sync");
	ret = system(command);
	ret = system(command);
	ret = system(command);

	if( ret != 0 ){
		ERR("Sync NG");
	}
}


/*-------------------------------------------------------------------
	FUNCTION :	sync_dir()
	EFFECTS  :	ﾃﾞｨﾚｸﾄﾘをsync
	NOTE	 :	ﾃﾞｨﾚｸﾄﾘをsyncしないと確実な保存はされないらしい
				https://linuxjm.osdn.jp/html/LDP_man-pages/man2/fsync.2.html
				`fsync() の呼び出しは、ファイルが存在しているディレクトリのエントリーが
				ディスクへ 書き込まれたことを保証するわけではない。
				保証するためには明示的にそのディレクトリのファイルディスクリプターに
				対してもfsync() する必要がある。`
-------------------------------------------------------------------*/
int sync_dir(const std::string& dir) {
	DIR *dp;
	int dd;
	int ret;

	dp = opendir(dir.c_str());
	if (!dp) {
		ERR("Could not opendir %s", dir.c_str());
		return -1;
	}
	dd = dirfd(dp);
	if(dd<0) {
		ERR("Could not dirfd %s", dir.c_str());
		return -1;
	}
	ret = fsync(dd);
	if(ret==-1) {
		ERR("failed fsync %s", dir.c_str());
		return -1;
	}
	ret = closedir(dp);
	if(ret==-1) {
		ERR("failed closedir %s", dir.c_str());
		return -1;
	}

	return 0;
}


/*-------------------------------------------------------------------
	FUNCTION :	error_info_init()
	EFFECTS  :	ｴﾗｰ情報初期化処理
	NOTE	 :	ｴﾗｰ情報を読出展開する
-------------------------------------------------------------------*/
void error_info_init(ErrorInfoType *info)
{
	FILE *fp;
	char path[256];
	struct stat st;												// ﾃﾞｨﾚｸﾄﾘ/ﾌｧｲﾙ属性
	size_t ret;

	memset((void *)info,0x00,sizeof(ErrorInfoType));				// 初期化
	memset((void *)&curerrinfo,0x00,sizeof(ErrorInfoType));

	sprintf(path,"%s%s",ERRINFO_FLDR,ERRINFO_NAME);
	fp = fopen(path, "r");
	if(fp){
		ret = (int)fread((void *)info,sizeof(ErrorInfoType),1,fp);
		if(ret!=1){
			ERR("Error info file read failed");
		}
		fclose(fp);
		memcpy((void *)&curerrinfo,(void *)info,sizeof(ErrorInfoType));
	}
	else{
		if(stat(ERRINFO_FLDR, &st) != 0){							// ｴﾗｰ情報ﾌｫﾙﾀﾞﾁｪｯｸ
			mkdir(ERRINFO_FLDR,0777);
		}
		fp = fopen(path, "w");
		if(fp){
			fwrite((void *)info,sizeof(ErrorInfoType),1,fp);
			fclose(fp);
		}
		else{
			ERR("Error info file create failed");
		}

	}

}

/*-------------------------------------------------------------------
	FUNCTION :	error_detail_init()
	EFFECTS  :	ｴﾗｰ詳細情報初期化処理
	NOTE	 :	ｴﾗｰ詳細情報を読出展開する
-------------------------------------------------------------------*/
void error_detail_init(ErrorDetailType *info)
{
	FILE *fp;
	char path[256];
	struct stat st;												// ﾃﾞｨﾚｸﾄﾘ/ﾌｧｲﾙ属性
	int ret;

	memset((void *)info,0x00,sizeof(ErrorDetailType));				// 初期化
	memset((void *)&curerrdetail,0x00,sizeof(ErrorDetailType));

	sprintf(path,"%s%s",ERRINFO_FLDR,ERRDETAIL_NAME);
	fp = fopen(path, "r");
	if(fp){
		ret = (int)fread((void *)&curerrdetail,sizeof(ErrorDetailType),1,fp);
		if(ret!=1){
			ERR("Error detail file read failed");
		}
		fclose(fp);
	}
	else{
		if(stat(ERRINFO_FLDR, &st) != 0){							// ｴﾗｰ情報ﾌｫﾙﾀﾞﾁｪｯｸ
			mkdir(ERRINFO_FLDR,0777);
		}
		fp = fopen(path, "w");
		if(fp){
			fwrite((void *)&curerrdetail,sizeof(ErrorDetailType),1,fp);
			fclose(fp);
		}
		else{
			ERR("Error detail file create failed");
		}

	}

}

/*-------------------------------------------------------------------
	FUNCTION :	error_info_save()
	EFFECTS  :	ｴﾗｰ情報保存処理
	NOTE	 :	ｴﾗｰ情報を保存する
-------------------------------------------------------------------*/
void error_info_save(ErrorInfoType info)
{
	FILE *fp;
	char path[256];
	ErrorInfoType tmp;
	struct stat st;												// ﾃﾞｨﾚｸﾄﾘ/ﾌｧｲﾙ属性
	int fd;

	memset(&tmp,0x00,sizeof(ErrorInfoType));

	tmp.err1.impprc_wdt = info.err1.impprc_wdt & 0x01;
	tmp.err1.pwvlt_uplimit = info.err1.pwvlt_uplimit & 0x01;
	tmp.err1.pwvlt_downlimit = info.err1.pwvlt_downlimit & 0x01;
	tmp.err1.serial = info.err1.serial & 0x01;
	tmp.err2.os = info.err2.os & 0x01;
	tmp.err2.application = info.err2.application & 0x01;
	tmp.err2.imgprc = info.err2.imgprc & 0x01;
	tmp.err2.imgprc_temp = info.err2.imgprc_temp & 0x01;
	tmp.err3.rtc = info.err3.rtc & 0x01;
	tmp.err3.mntor_out = info.err3.mntor_out & 0x01;
	tmp.err3.input_img = info.err3.input_img & 0x01;
	tmp.err3.data_save = info.err3.data_save & 0x01;
	tmp.err4.input_area = info.err4.input_area & 0x01;
	tmp.err4.base_cpu = info.err4.base_cpu & 0x01;

	if(!memcmp(&curerrinfo,&tmp,sizeof(ErrorInfoType))){
		return;
	}

	sprintf(path,"%s%s",ERRINFO_FLDR,ERRINFO_NAME);

	if(stat(ERRINFO_FLDR, &st) != 0){							// ｴﾗｰ情報ﾌｫﾙﾀﾞﾁｪｯｸ
		mkdir(ERRINFO_FLDR,0777);
	}
	fp = fopen(path, "w");
	if(!fp){
		ERR("Error info file create failed");
		return;
	}
	fd = fileno(fp);
	fwrite((void *)&tmp,sizeof(ErrorInfoType),1,fp);
	fsync(fd);
	fclose(fp);
	sync_dir(ERRINFO_FLDR);
	curerrinfo = tmp;

}

/*-------------------------------------------------------------------
	FUNCTION :	error_detail_save()
	EFFECTS  :	ｴﾗｰ詳細情報保存処理
	NOTE	 :	ｴﾗｰ詳細情報を保存する
-------------------------------------------------------------------*/
void error_detail_save(ErrorDetailType info)
{
	FILE *fp;
	char path[256];
	struct stat st;												// ﾃﾞｨﾚｸﾄﾘ/ﾌｧｲﾙ属性
	int fd;

	if(!memcmp(&curerrdetail,&info,sizeof(ErrorDetailType))){
		return;
	}


	curerrdetail.err1.ram_chk			|= info.err1.ram_chk;
	curerrdetail.err1.cpu_chk	 		|= info.err1.cpu_chk;
	curerrdetail.err1.rom_chksum		|= info.err1.rom_chksum;
	curerrdetail.err1.cpu_err			|= info.err1.cpu_err;
	curerrdetail.err2.rst_illegalorder 	|= info.err2.rst_illegalorder;
	curerrdetail.err2.rst_wdt_clock		|= info.err2.rst_wdt_clock;
	curerrdetail.err2.rst_illegalaccess	|= info.err2.rst_illegalaccess;
	curerrdetail.err2.rst_pwr_donw		|= info.err2.rst_pwr_donw;
	curerrdetail.err3.batmon_upper		|= info.err3.batmon_upper;
	curerrdetail.err3.mon_24v_upper		|= info.err3.mon_24v_upper;
	curerrdetail.err3.mon_3r3v_upper	|= info.err3.mon_3r3v_upper;
	curerrdetail.err3.mon_5v_upper		|= info.err3.mon_5v_upper;
	curerrdetail.err3.mon_1r8vd_upper	|= info.err3.mon_1r8vd_upper;
	curerrdetail.err3.mon_3r3va_upper	|= info.err3.mon_3r3va_upper;
	curerrdetail.err3.mon_1r8va_upper	|= info.err3.mon_1r8va_upper;
	curerrdetail.err3.mon_1r8vap_upper	|= info.err3.mon_1r8vap_upper;
	curerrdetail.err4.mon_12va_upper	|= info.err4.mon_12va_upper;
	curerrdetail.err5.batmon_lower		|= info.err5.batmon_lower;
	curerrdetail.err5.mon_24v_lower		|= info.err5.mon_24v_lower;
	curerrdetail.err5.mon_3r3v_lower	|= info.err5.mon_3r3v_lower;
	curerrdetail.err5.mon_5v_lower		|= info.err5.mon_5v_lower;
	curerrdetail.err5.mon_1r8vd_lower	|= info.err5.mon_1r8vd_lower;
	curerrdetail.err5.mon_3r3va_lower	|= info.err5.mon_3r3va_lower;
	curerrdetail.err5.mon_1r8va_lower	|= info.err5.mon_1r8va_lower;
	curerrdetail.err5.mon_1r8vap_lower	|= info.err5.mon_1r8vap_lower;
	curerrdetail.err6.mon_12va_lower	|= info.err6.mon_12va_lower;

	sprintf(path,"%s%s",ERRINFO_FLDR,ERRDETAIL_NAME);

	if(stat(ERRINFO_FLDR, &st) != 0){							// ｴﾗｰ情報ﾌｫﾙﾀﾞﾁｪｯｸ
		mkdir(ERRINFO_FLDR,0777);
	}
	fp = fopen(path, "w");
	if(!fp){
		ERR("Error detail file create failed");
		return;
	}
	fd = fileno(fp);
	fwrite((void *)&curerrdetail,sizeof(ErrorDetailType),1,fp);
	fsync(fd);
	fclose(fp);
	sync_dir(ERRINFO_FLDR);

}

/*-------------------------------------------------------------------
	FUNCTION :	error_detail_reset()
	EFFECTS  :	ｴﾗｰ詳細情報ﾘｾｯﾄ処理
	NOTE	 :	ｴﾗｰ詳細情報をﾘｾｯﾄする
-------------------------------------------------------------------*/
void error_detail_reset(void)
{
	FILE *fp;
	char path[256];
	struct stat st;												// ﾃﾞｨﾚｸﾄﾘ/ﾌｧｲﾙ属性
	int fd;


	sprintf(path,"%s%s",ERRINFO_FLDR,ERRDETAIL_NAME);

	memset((void *)&curerrdetail,0x00,sizeof(ErrorDetailType));

	if(stat(ERRINFO_FLDR, &st) != 0){							// ｴﾗｰ情報ﾌｫﾙﾀﾞﾁｪｯｸ
		mkdir(ERRINFO_FLDR,0777);
	}
	fp = fopen(path, "w");
	if(!fp){
		ERR("Error detail file create failed");
		return;
	}
	fd = fileno(fp);
	fwrite((void *)&curerrdetail,sizeof(ErrorDetailType),1,fp);
	fsync(fd);
	fclose(fp);
	sync_dir(ERRINFO_FLDR);

}

/*-------------------------------------------------------------------
	FUNCTION :	error_reset()
	EFFECTS  :	ｴﾗｰﾘｾｯﾄ処理
	NOTE	 :	ｴﾗｰﾘｾｯﾄする
-------------------------------------------------------------------*/
void error_reset(ErrorInfoType *info,ErrorDetailType *detail)
{

	if(info->err1.impprc_wdt == 0x01)		info->err1.impprc_wdt = 0;
	if(info->err1.pwvlt_uplimit == 0x01)	info->err1.pwvlt_uplimit = 0;
	if(info->err1.pwvlt_downlimit == 0x01)	info->err1.pwvlt_downlimit = 0;
	if(info->err1.serial == 0x01)			info->err1.serial = 0;
	if(info->err2.os == 0x01)				info->err2.os = 0;
	if(info->err2.application == 0x01)		info->err2.application = 0;
	if(info->err2.imgprc == 0x01)			info->err2.imgprc = 0;
	if(info->err2.imgprc_temp == 0x01)		info->err2.imgprc_temp = 0;
	if(info->err3.rtc == 0x01)				info->err3.rtc = 0;
	if(info->err3.mntor_out == 0x01)		info->err3.mntor_out = 0;
	if(info->err3.input_img == 0x01)		info->err3.input_img = 0;
	if(info->err3.data_save == 0x01)		info->err3.data_save = 0;
	if(info->err4.input_area == 0x01)		info->err4.input_area = 0;
	if(info->err4.base_cpu == 0x01)			info->err4.base_cpu = 0;

	error_info_save(*info);
	error_detail_reset();												// 一旦初期化
	error_detail_save(*detail);											// 現異常書き込み

}


bool ReadBinaryFile(std::vector<char>*const pReadData, const std::string& path){
	assert(pReadData);
	ifstream ifs(path, ios::in|ios::binary);
	// 読み込み失敗検出
	if(!ifs){
		ERR("failed binary read open %s.", path.c_str());
		return false;
	}
	// ﾃﾞｰﾀ取得
	*pReadData = vector<char>{(istreambuf_iterator<char>(ifs)), (istreambuf_iterator<char>())};
	return true;
}

bool ReadStringFile(std::string*const pReadData, const std::string& path){
	assert(pReadData);
	ifstream ifs(path, ios::in);
	// ｵｰﾌﾟﾝ失敗検出
	if(!ifs){
		ERR("failed read open %s.", path.c_str());
		return false;
	}
	// ﾃﾞｰﾀ取得
	*pReadData = string{(istreambuf_iterator<char>(ifs)), (istreambuf_iterator<char>())};
	return true;
}

bool WriteStringFile(const std::string& writeData, const std::string& path){
	ofstream ofs(path);
	// ｵｰﾌﾟﾝ失敗検出
	if(!ofs){
		ERR("failed write open %s.", path.c_str());
		return false;
	}
	// 書き込み
	ofs << writeData;
	return true;
}

bool InitHWMON(void){
	static const string PARENT_DIR = "/sys/class/hwmon/";						//!< HWMONの親ﾃﾞｨﾚｸﾄﾘ
	static const string DIR_PREFIX = "hwmon";									//!< ﾃﾞｨﾚｸﾄﾘ名の数字以外の部分
	static const string TEMP_FILE_NAME = "temp1_input";							//!< 温度のﾌｧｲﾙ名
	static const string TEMP_MODULE_NAME = "lm75";								//!< 外部温度のカーネルモジュール名
	static const int NUM_HWMON = 2;												//!< HWMONの数
#ifndef STOP_WATTAGE_CHECK
	static const string WATTAGE_FILE_NAME = "power1_input";						//!< 消費電力のﾌｧｲﾙ名
	static const string WATTAGE_MODULE_NAME = "";								//!< 消費電力のカーネルモジュール名
#endif

	// ﾁｪｯｸ対象ﾃﾞﾊﾞｲｽのﾃﾞｰﾀを作成
	struct Data{
		string fileName;														//!< ﾃﾞﾊﾞｲｽのﾌｧｲﾙ名
		string* pPath;															//!< ﾊﾟｽを格納する場所
		ErrorCode errorCode;													//!< ｴﾗｰ発生時に使用するｴﾗｰｺｰﾄﾞ
		string moduleName;														// 対応するカーネルモジュール名
	};
	// ﾃﾞﾊﾞｲｽの種類を増やすときは関数ｺﾒﾝﾄにｴﾗｰｺｰﾄﾞの追記を忘れないこと
	vector<Data> datas = {														//!< ﾁｪｯｸ対象ﾃﾞﾊﾞｲｽのﾃﾞｰﾀ
		{TEMP_FILE_NAME, &g_externalTempPath, ErrorCode::NotFoundTempDevice, TEMP_MODULE_NAME},		// 温度
#ifndef STOP_WATTAGE_CHECK
		{WATTAGE_FILE_NAME, &g_wattagePath, ErrorCode::NotFoundPowerDevice, WATTAGE_MODULE_NAME},	// 電力
#endif
	};

	// 各ﾃﾞﾊﾞｲｽのﾌｧｲﾙの存在確認を実施し、存在していればﾊﾟｽを保存
	bool existAllFile = true;													//!< 全てのﾌｧｲﾙが見つかったか？
	for(auto& data : datas){
		bool existFile = false;													//!< ﾌｧｲﾙが見つかったか？
		for(int i=0;i<NUM_HWMON;++i){
			const string dir = PARENT_DIR + DIR_PREFIX + to_string(i) + "/";	//!< 親ﾃﾞｨﾚｸﾄﾘ
			const string path = dir + data.fileName;							//!< ﾌｧｲﾙﾊﾟｽ
			const std::ifstream ifs(path);										//!< ﾌｧｲﾙの存在確認用
			if(ifs.is_open()){
				*(data.pPath) = path;
				existFile = true;
				break;
			}
			// ﾌｧｲﾙの存在確認が失敗した場合、モジュールの再読み込みを行う
			if(data.moduleName != ""){
				const string command = "modprobe " + data.moduleName;
				if(!system(command.c_str())){
					const std::ifstream rifs(path);								//!< ﾌｧｲﾙの存在確認用
					if(rifs.is_open()){
						*(data.pPath) = path;
						existFile = true;
						break;
					}
				}
			}
		}
		// 存在していなければｴﾗｰ
		g_errorManager.SetError(data.errorCode, !existFile);
		if(existFile == false){
			ERR("Not Found %s", data.fileName.c_str());
			existAllFile = false;
		}
	}

	return existAllFile;
}


int GetMicrowattsOfSOM(void){
	// 読み込み
	string text;
	const bool isSuccess = g_wattagePath.empty() ? false : ReadStringFile(&text, g_wattagePath);
	g_errorManager.SetError(ErrorCode::GetMicrowattsOfSOM, !isSuccess);
	if(isSuccess == false){
		return -1;
	}

	// 計算
	return stoi(text);
}

std::tuple<float, float> GetInternalTemp(void){
	// 計算処理用のﾃｰﾌﾞﾙを作成
	struct Data{
		string path;
		ErrorCode errorCode;
	};
	static const vector<Data> datas = {
		{"/sys/class/thermal/thermal_zone0/temp", ErrorCode::GetInternalTempZone0},
		{"/sys/class/thermal/thermal_zone1/temp", ErrorCode::GetInternalTempZone1},
	};
	vector<float> values(datas.size());

	// それぞれの値を取得して計算
	for(uint i=0; i<datas.size(); ++i){
		const auto& data = datas[i];
		// 読み込み
		string text;
		const bool isSuccess = ReadStringFile(&text, data.path);
		g_errorManager.SetError(data.errorCode, !isSuccess);
		if(isSuccess == false){
			continue;
		}
		// 計算
		values[i] = static_cast<float>(stoi(text)) / 1000.f;
	}

	return {values[0], values[1]};
}

float GetExternalTemp(void){
	// 読み込み
	string text;
	const bool isSuccess = g_externalTempPath.empty() ? false : ReadStringFile(&text, g_externalTempPath);
	g_errorManager.SetError(ErrorCode::GetExternalTemp, !isSuccess);
	if(isSuccess == false){
		return -1;
	}

	// 計算
	return static_cast<float>(stoi(text)) / 1000.f;
}

void DeleteExternalTempPath(void){	// 温度ｾﾝｻｰの取得パスを削除
	// 温度取得失敗時のOS異常発生の検証用
	g_externalTempPath.erase();
}


static struct termios g_old_term;
static struct termios g_new_term;
static int g_readchar = -1;

static bool g_term_set = false;
void release_console() {
	int consfd;
	if (g_term_set) {
//		while (getchar() != -1) {  // read cached characters
//		}
		tcsetattr(0, TCSANOW, &g_old_term);
		g_term_set = false;
	}
	consfd = open("/dev/tty0", O_RDWR | O_CLOEXEC);
	if (consfd != -1) {
		ioctl(consfd, KDSETMODE, KD_TEXT);
		close(consfd);
	}
}

/// @brief Previous signal handlers.
static void (*g_prev_sigint)(int sig) = NULL;
static void (*g_prev_sigquit)(int sig) = NULL;
static void (*g_prev_sigterm)(int sig) = NULL;

static void process_signal(int sig, void (*prev_handler)(int sig)) {
	release_console();
	if (prev_handler == SIG_IGN) {
		return;
	}
	if (prev_handler == SIG_DFL) {
		signal(sig, SIG_DFL);
		raise(sig);
		return;
	}
	(*prev_handler)(sig);
}

static void on_sigint(int sig) {
	process_signal(sig, g_prev_sigint);
}

static void on_sigquit(int sig) {
	process_signal(sig, g_prev_sigquit);
}

static void on_sigterm(int sig) {
	process_signal(sig, g_prev_sigterm);
}

bool kbhit()
{
    char ch;
    int nread;

    if(g_readchar !=-1) {
        return true;
    }

    g_new_term.c_cc[VMIN]=0;
    tcsetattr(0,TCSANOW,&g_new_term);
    nread=read(0,&ch,1);
    g_new_term.c_cc[VMIN]=1;
    tcsetattr(0,TCSANOW,&g_new_term);

    if(nread == 1) {
        g_readchar = ch;
        return true;
    }

    return false;
}

char linux_getch()
{
    char ch;
    int nread;

    if(g_readchar != -1) {
       ch = g_readchar;
       g_readchar = -1;
       return (ch);
    }
    nread=read(0,&ch,1);
    if(nread == 1) {
		return(ch);
	}
	else{
		return(-1);
	}
}

int cust_getchar(void)
{
	int ret = -1;

	if (kbhit()) {
		char ch = linux_getch();
		ret = (int)ch;
	}
	return (ret);
}

/*-------------------------------------------------------------------
	FUNCTION :	draw_init()
	EFFECTS  :	描画系初期化
	NOTE	 :	描画系初期化
-------------------------------------------------------------------*/
bool draw_init(void)
{
	// ｼｸﾞﾅﾙ操作を登録
	g_prev_sigint = signal(SIGINT, on_sigint);
	if(g_prev_sigint == SIG_ERR){
		ERR("signal SIGINT failed. errno=%d : %s", errno, strerror(errno));
	}
	g_prev_sigquit = signal(SIGQUIT, on_sigquit);
	if(g_prev_sigquit == SIG_ERR){
		ERR("signal SIGQUIT failed. errno=%d : %s", errno, strerror(errno));
	}
	g_prev_sigterm = signal(SIGTERM, on_sigterm);
	if(g_prev_sigterm == SIG_ERR){
		ERR("signal SIGTERM failed. errno=%d : %s", errno, strerror(errno));
	}

	// ﾌﾟﾛｾｽが正常終了した時に解放処理が走るよう登録しておく
	if(atexit(release_console) != 0){
		ERR("failed atexit(release_console)");
	}

	// ｱｸﾃｨﾌﾞｺﾝｿｰﾙをｸﾞﾗﾌｨｯｸﾓｰﾄﾞにする
	{
		long mode = 0;
		int consfd = open("/dev/tty0", O_RDWR | O_CLOEXEC);

		if (consfd != -1) {	// open成功
			// 現在のﾓｰﾄﾞ（ﾃｷｽﾄﾓｰﾄﾞ/ｸﾞﾗﾌｨｯｸﾓｰﾄﾞ）が取得できなければｴﾗｰ
			if (ioctl(consfd, KDGETMODE, &mode) < 0) {
				ERR("Could not determine console mode (text or graphics). errno=%d : %s", errno, strerror(errno));
			}
			else {
				if (mode == KD_TEXT) {	// ｺﾝｿｰﾙがﾃｷｽﾄﾓｰﾄﾞであればｸﾞﾗﾌｨｯｸﾓｰﾄﾞに切り替える
					if (ioctl(consfd, KDSETMODE, KD_GRAPHICS) < 0) {
						ERR("Could not change console to graphics mode");
					}
				}
				// すでにｸﾞﾗﾌｨｯｸﾓｰﾄﾞであればそのまま終了
			}

			if(close(consfd) != 0) { // open成功は最後にclose
				ERR("Failed console close. errno=%d : %s", errno, strerror(errno));
			}
		}
		else {	// open失敗
			ERR("Failed to open /dev/tty0 errno=%d : %s", errno, strerror(errno));
		}
	}

	// ﾀｰﾐﾅﾙ属性を設定する
	{
		// 現在のﾀｰﾐﾅﾙ属性を取得
		if(tcgetattr(0, &g_old_term)<0){
			ERR("Failed get term. errno=%d : %s", errno, strerror(errno));
		}
		memcpy(&g_new_term, &g_old_term, sizeof(g_new_term));

		// 新しくﾀｰﾐﾅﾙ属性を設定
		g_new_term.c_lflag &= ~ICANON;	// 非ｶﾉﾆｶﾙﾓｰﾄﾞ
		g_new_term.c_lflag &= ~ECHO;		// 入力された文字をｴｺｰしない
		g_new_term.c_lflag &= ~ISIG;		// INTR, QUIT, SUSP, DSUSP の文字を受信した時、対応するｼｸﾞﾅﾙを発生させない
		g_new_term.c_cc[VMIN] = 0;
		g_new_term.c_cc[VTIME] = 0;
		tcsetattr(0, TCSANOW, &g_new_term);
		g_term_set = true;
	}

	// ﾌﾚｰﾑﾊﾞｯﾌｧをｸﾘｱ
	{
		// HDMIﾌﾚｰﾑﾊﾞｯﾌｧをｵｰﾌﾟﾝ
		const int fbfd = open ("/dev/fb2", O_RDWR | O_CLOEXEC);
		if(fbfd < 0){
			// HDMIｹｰﾌﾞﾙが刺さっていない場合は毎回ｴﾗｰになる。通常運用の挙動であるためｴﾗｰ出力は行わない
			//ERR("Failed to open /dev/fb0 errno=%d : %s", errno, strerror(errno));
			//return false;
		}
		else {
			// ﾌﾚｰﾑﾊﾞｯﾌｧのﾒﾓﾘｻｲｽﾞを取得
			ioctl (fbfd, FBIOGET_VSCREENINFO, &g_fb2_var_info);
			const int fb_width = g_fb2_var_info.xres;
			const int fb_height = g_fb2_var_info.yres;
			const int fb_bpp = g_fb2_var_info.bits_per_pixel;
			const int fb_bytes = fb_bpp / 8;
			const int fb_data_size = fb_width * fb_height * fb_bytes;
			// ﾌﾚｰﾑﾊﾞｯﾌｧに書き込む
			char *fbdata = (char *)mmap (0, fb_data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, (off_t)0);
			if(fbdata){
				memset (fbdata, 0, fb_data_size);
				munmap (fbdata, fb_data_size);
			}else{
				ERR("mmap() failed for /dev/fb2. errno=%d : %s", errno, strerror(errno));
			}
			if(close(fbfd) != 0){
				ERR("Failed /dev/fb2 close. errno=%d : %s", errno, strerror(errno));
			}
		}
	}

	// HD-TVI(MIPI)ﾌﾚｰﾑﾊﾞｯﾌｧをｸﾘｱ
	{
		// ﾌﾚｰﾑﾊﾞｯﾌｧをｵｰﾌﾟﾝ
		const int fbfd = open ("/dev/fb0", O_RDWR | O_CLOEXEC);	// HD-TVI(MIPI)
		if(fbfd < 0){
			// HD-TVI(MIPI)ﾌﾚｰﾑﾊﾞｯﾌｧが認識されていない場合、エラー
			ERR("Failed to open /dev/fb0 errno=%d : %s", errno, strerror(errno));
			return false;
		}

		// ﾌﾚｰﾑﾊﾞｯﾌｧのﾒﾓﾘｻｲｽﾞを取得
		ioctl (fbfd, FBIOGET_VSCREENINFO, &g_fb0_var_info);
		const int fb_width = g_fb0_var_info.xres;
		const int fb_height = g_fb0_var_info.yres;
		const int fb_bpp = g_fb0_var_info.bits_per_pixel;
		const int fb_bytes = fb_bpp / 8;
		const int fb_data_size = fb_width * fb_height * fb_bytes;

		// ﾌﾚｰﾑﾊﾞｯﾌｧに書き込む
		char *fbdata = (char *)mmap (0, fb_data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, (off_t)0);
		if(fbdata){
			memset (fbdata, 0, fb_data_size);
			munmap (fbdata, fb_data_size);
		}else{
			ERR("mmap() failed for /dev/fb0. errno=%d : %s", errno, strerror(errno));
		}
		if(close(fbfd) != 0){
			ERR("Failed /dev/fb0 close. errno=%d : %s", errno, strerror(errno));
		}
	}
	// HD-TVI(MIPI)ﾌﾚｰﾑﾊﾞｯﾌｧをｸﾘｱ
	{
		// ﾌﾚｰﾑﾊﾞｯﾌｧをｵｰﾌﾟﾝ
		const int fbfd = open("/dev/fb1", O_RDWR | O_CLOEXEC); // HD-TVI(LVDS)
		if (fbfd < 0) {
			// HD-TVI(LVDS)ﾌﾚｰﾑﾊﾞｯﾌｧが認識されていない場合、エラー
			ERR("Failed to open /dev/fb1 errno=%d : %s", errno,
				strerror(errno));
			return false;
		}

		// ﾌﾚｰﾑﾊﾞｯﾌｧのﾒﾓﾘｻｲｽﾞを取得
		ioctl(fbfd, FBIOGET_VSCREENINFO, &g_fb1_var_info);
		const int fb_width = g_fb1_var_info.xres;
		const int fb_height = g_fb1_var_info.yres;
		const int fb_bpp = g_fb1_var_info.bits_per_pixel;
		const int fb_bytes = fb_bpp / 8;
		const int fb_data_size = fb_width * fb_height * fb_bytes;

		// ﾌﾚｰﾑﾊﾞｯﾌｧに書き込む
		char *fbdata = (char *)mmap(0, fb_data_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, (off_t)0);
		if (fbdata) {
			memset(fbdata, 0, fb_data_size);
			munmap(fbdata, fb_data_size);
		} else {
			ERR("mmap() failed for /dev/fb1. errno=%d : %s", errno,
				strerror(errno));
		}
		if (close(fbfd) != 0) {
			ERR("Failed /dev/fb1 close. errno=%d : %s", errno,
				strerror(errno));
		}
	}

	return true;
}

string GetMonthEnglishText(int month){
	assert(month>=1 && month<=12);
	static const string monthText[12] = {"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"};
	return monthText[month-1];
}

uint32_t get_screen_width() {
  return g_fb2_var_info.xres;
}

uint32_t get_screen_height() {
  return g_fb2_var_info.yres;
}


std::string GetFileName(const std::string& path){
	auto path_i = path.find_last_of("\\") + 1;
	auto ext_i = path.find_last_of(".");
	if(path_i == string::npos){
		path_i = 0;
	}
	if(ext_i == string::npos){
		ext_i = path.size()-1;
	}
	return path.substr(path_i,ext_i-path_i);
}

std::string GetExtensionName(const std::string& path){
	auto ext_i = path.find_last_of(".");
	if(ext_i == string::npos){
		return "";
	}
	return path.substr(ext_i,path.size()-ext_i);
}

std::string GetDirPath(const std::string& path){
	const std::string::size_type pos = std::max<signed>(path.find_last_of('/'), path.find_last_of('\\'));
	return (pos == std::string::npos) ? std::string() : path.substr(0, pos + 1);
}

std::string BinToHexString(const std::vector<bool>& bin){
	// 値が存在しない場合は例外的にカラを返す
	if(bin.size()==0){
		return "";
	}

	std::string text;			//!< 出力用文字列
	uint index = 0;				//!< bin配列のindex

	// 出力用文字列を作成
	while(index < bin.size()){
		std::stringstream ss;	//!< 1ﾊﾞｲﾄごとの文字列
		int val = 0;			//!< 1ﾊﾞｲﾄごとの数値
		// 1ﾊﾞｲﾄごとの数値を計算
		for(int i=0;i<8;++i){
			// 桁がｵｰﾊﾞｰする直前で計算終了
			if(index >= bin.size()){
				break;
			}
			val += static_cast<int>(bin[index]) << index%8;
			++index;
		}
		// 1ﾊﾞｲﾄごとの文字列を作成
		ss << std::setfill('0') << std::setw(2) << std::hex << val;

		// ｽﾍﾟｰｽ区切りで登録（bin[0]が右端にくるように）
		// std::showbaseを使用すると0埋めがうまく動作しないので、手動で0xを付与
		text = " 0x" + ss.str() + text;
	}
	// 先頭の空白を削除
	text.erase(0, 1);

	return text;
}


typedef std::chrono::steady_clock::time_point TP;

std::chrono::steady_clock::time_point getTime()
{
	return std::chrono::steady_clock::now();
}

uint64_t getTimeDiff(std::chrono::steady_clock::time_point nTS, std::chrono::steady_clock::time_point nTE)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(nTE-nTS).count();
}

float getFPS(std::chrono::steady_clock::time_point nTS, std::chrono::steady_clock::time_point nTE, int nNumFrames, uint64_t nTimeAdj)
{
    return nNumFrames * 1.0 * 1e6 / (std::chrono::duration_cast<std::chrono::microseconds>(nTE-nTS).count() - nNumFrames * nTimeAdj);
}

/*-------------------------------------------------------------------
  FUNCTION :	GetTimeMsec()
  EFFECTS  :	時間取得
  NOTE	 :
  -------------------------------------------------------------------*/
double GetTimeMsec(void)
{
	double time;
	std::chrono::system_clock::time_point now;
	now = std::chrono::system_clock::now();
    time =  (double)(std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count())/(double)1000;
	return time;
}
