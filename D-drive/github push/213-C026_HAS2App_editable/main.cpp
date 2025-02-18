//--------------------------------------------------
//! @file main.cpp
//! @brief ﾒｲﾝ処理
//--------------------------------------------------

#include <cassert>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <linux/rtc.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <mntent.h>
#include <md5.h>
#include <dirent.h>
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include <future>

#include "Sysdef.h"
#include "Global.h"
#include "unzip.h"
#include "LinuxUtility.hpp"
#include "BoardCtrl.hpp"
#include "CanCtrl.h"
#include "SerialCommand.h"
#include "Application.h"
#include "Capture.hpp"
#include "ExternalMemoryUtility.hpp"


using namespace std;


/*===	external function	===*/
int	ReadAppConf(void);
int create_serial_thread(void);
int delete_serial_thread(void);
int CreateCANThread(void);
int DeleteCANThread(void);
void error_info_init(ErrorInfoType *info);
void error_detail_init(ErrorDetailType *info);
void error_info_save(ErrorInfoType info);
bool draw_init(void);
void release_console();
int cust_getchar(void);
bool update_lattice(std::string file);
int create_disp0_thread(void);
int delete_disp0_thread(void);
int create_disp1_thread(void);
int delete_disp1_thread(void);
int create_cap0_thread(void);
int delete_cap0_thread(void);

/*===	static function prototypes	===*/
void update_exec(void);

/*===	static variable		===*/
int sdcard_enb;																// SDｶｰﾄﾞ有効/無効
ErrorInfoType errorinfo;													// ｴﾗｰ情報
ErrorDetailType errordetail;												// ｴﾗｰ詳細情報
// Buffer for decoded image data
timer_t timerid;															// Timer ID


/*===	user definition		===*/
// バージョン情報
const char	*PRODUCT	= "HAS2 (72t)";
const char	*TDN_VER	= "361-731-40000";
const char	*TDN_REV    = "99";
const char	*TDN_TYPE   = "FF";
const char	*VERSION	= "0.1.15";
const char	*RELEASE	= "2024/12/03";
const char	*RELEASE1	= "13:00";


/*===	global variable		===*/



//--------------------------------------------------
//! @brief RTC異常確認
//!
//! ｴﾗｰ情報の保存まで行う
//! @param[in]		isUninitializedRTC	RTC未初期化（未設定）状態か？
//--------------------------------------------------
void CheckRTC(bool is_uninitialized_rtc){
	// RTCｽﾃｰﾀｽ確認
	const auto [isLowBattery, isOscillationStop] = GetRTCStatus();

	// RTCｽﾃｰﾀｽ取得失敗時は異常確認できない
	if(g_errorManager.IsError(ErrorCode::GetRTCStatus)){
		return;
	}

	// 異常を残す
	g_errorManager.SetError(ErrorCode::LowBatteryRTC, isLowBattery);
	g_errorManager.SetError(ErrorCode::OscillationStopRTC, isOscillationStop);

	if( isLowBattery || isOscillationStop){
		if( (errorinfo.err3.rtc & 0x02) == 0x00 ){								// 未検知状態
			ERR("RTC status NG");
			errorinfo.err3.rtc = 3;												// RTC異常
			error_info_save(errorinfo);
			error_info_send_req();
		}
	}else{
		if(is_uninitialized_rtc == false){										// RTC未設定時は異常を解除しない
			if( (errorinfo.err3.rtc & 0x02) == 0x02 ){							// 異常検知解除
				LOG(LOG_COMMON_OUT,"Rtc Status return to Normal");
				errorinfo.err3.rtc = 1;											// RTC異常解除
				error_info_send_req();
			}
		}
	}
}

//--------------------------------------------------
//! @brief 電力確認
//!
//! ﾛｷﾞﾝｸﾞのみ
//--------------------------------------------------
void CheckWattageOfSOM(void){
#ifndef STOP_WATTAGE_CHECK
	// 電力確認
	const int microwatts = GetMicrowattsOfSOM();

	// 電力取得失敗時は異常確認できない
	if(g_errorManager.IsError(ErrorCode::GetMicrowattsOfSOM)){
		return;
	}

	// 環境試験用ﾛｸﾞ出力
	if(g_usWattageMoni){
		DEBUG_LOG("[WAT]%d", microwatts);
	}
#endif
}

//--------------------------------------------------
//! @brief WDTﾊﾟﾙｽ更新
//--------------------------------------------------
void UpdateWDTPulse(void){
	static bool nextOutput = true;
	static double timer = 0.;													//!< 経過時間
	static double oldTime = GetTimeMsec();									//!< 前回呼び出された時刻
	const double nowTime = GetTimeMsec();										//!< 現在時刻

	// 経過時間を計測
	timer += nowTime - oldTime;
	// 周期時間を迎えた場合はWDTﾊﾟﾙｽを更新
	if(timer >= WDT_PULSE_TIME){
		OutputWDTPulse(nextOutput);
		nextOutput = !nextOutput;
		timer = 0.;
	}
	// 時刻更新
	oldTime = nowTime;
}

//--------------------------------------------------
//! @brief メモリ使用率を取得
//! @param[in,out]	used				メモリ使用パーセンテージ（0～100）
//! @retval	true	値の取得に成功
//! @retval	false	値の取得に取得失敗
//--------------------------------------------------
bool GetMemoryUsedRate(float*const used){
	assert(used);
	FILE	*fp;
	char	buf[100];
	bool	result = false;

	// shellコマンド実行及び結果の取得
	if((fp = popen("free | head -n 2 | tail -n 1 | awk '{print ($3/$2)*100}'","r")) == NULL ){
		ERR("Failed to get memory used.");
		return(result);
	}
	//結果読み出し
	if( fgets(buf, 100, fp) != NULL ){
		result = true;
		*used = (float)atof(buf);	// Float型数値へ変換
		LOG(LOG_MEMORY_DEBUG,"Memory Used %f",*used);
	}
	pclose(fp);

	return(result);
}

/*-------------------------------------------------------------------
	FUNCTION :	update_check()
	EFFECTS  :	ｱｯﾌﾟﾃﾞｰﾄ確認
	NOTE	 :	危険動作判定ｱﾌﾟﾘ/学習ﾃﾞｰﾀｱｯﾌﾟﾃﾞｰﾄ確認
-------------------------------------------------------------------*/
int update_check(void)
{
	FILE *fp;
	struct mntent *e;
	struct stat st;
	int result = 0;
	char path[256];

	try{
		fp = setmntent("/proc/mounts", "r");						// ﾏｳﾝﾄﾎﾟｲﾝﾄ取得

		if( fp ){
			while((e = getmntent(fp))!= NULL ){
				if(!strcmp(MOUNT_USB_SSD_DIR.c_str(),e->mnt_dir)){			// ﾏｳﾝﾄﾎﾟｲﾝﾄがSDｶｰﾄﾞﾏｳﾝﾄ先
					sdcard_enb = 1;
					break;
				}

			}
			if(sdcard_enb){
				sprintf(path,"%s/%s",MOUNT_USB_SSD_DIR.c_str(),UPDATE_FILE);

				if (stat(path, &st) == 0) {
					if((st.st_mode & S_IFMT) == S_IFREG){
						result = 1;
						LOG(LOG_COMMON_OUT,"Update File Found");
					}
				}
			}
			endmntent(fp);
		}
	}
	catch(const std::exception& ex){
		ERR("update_check err catch");
	}

	return(result);
}

/*-------------------------------------------------------------------
	FUNCTION :	update_exec()
	EFFECTS  :	ｱｯﾌﾟﾃﾞｰﾄ処理実行
	NOTE	 :	危険動作判定ｱﾌﾟﾘ/学習ﾃﾞｰﾀｱｯﾌﾟﾃﾞｰﾄの実行
-------------------------------------------------------------------*/
void update_exec(void)
{
	unzFile uf=NULL;
    unz_global_info gi;
    int err,ret;
	unsigned int i;
	unsigned char packsum[MD5_DIGEST_LENGTH+1];
	unsigned char md5[MD5_DIGEST_LENGTH+1];
	char path[512];
	MD5_CTX ctx;
	unsigned char *databuf = NULL;
	unsigned char *packbuf = NULL;
	int package_flg = 0;
	int packlen = 0;
	int datalen = 0;
	int packmd5flg = 0;
	int result = 0;
	FILE *fp;
	int fd;
	struct stat st;							// ﾃﾞｨﾚｸﾄﾘ/ﾌｧｲﾙ属性
	string dataFileName;
	string package_path = MOUNT_USB_SSD_DIR + "/" + string(UPDATE_FILE);

	g_updatestatus = kUpdateWait;
	try{
		// ｱｯﾌﾟﾃﾞｰﾄﾊﾟｯｹｰｼﾞの検索
		uf = unzOpen(package_path.c_str());															// ｱｯﾌﾟﾃﾞｰﾄ圧縮ﾌｧｲﾙｵｰﾌﾟﾝ
		if(uf){
			g_updatestatus = kUpdateStart;
			LOG(LOG_COMMON_OUT,"Open Update File!!");
			err = unzGetGlobalInfo (uf,&gi);												// 情報取得
			if (err!=UNZ_OK){
				ERR("error %d with zipfile in unzGetGlobalInfo ",err);
				unzCloseCurrentFile(uf);
				g_updatestatus = kUpdateErr;
				return;
			}
			else{
				for (i=0;i<gi.number_entry;i++){												// ﾌｧｲﾙ情報数分ﾙｰﾌﾟ
					char filename_inzip[256];
					char filename[256];
					char *dirEnd;
					unz_file_info file_info;
					err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);		// ｶﾚﾝﾄﾌｧｲﾙ情報取得
					if (err!=UNZ_OK)															// ﾌｧｲﾙ情報取得失敗
					{
						ERR("error %d with zipfile in unzGetCurrentFileInfo",err);
						g_updatestatus = kUpdateErr;
						return;
					}
					dirEnd = strrchr(filename_inzip,'/');
					if(dirEnd == NULL ){
						strcpy(filename,filename_inzip);
					}
					else{
						strcpy(filename,dirEnd+1);
					}

					LOG(LOG_COMMON_OUT,"Update Zip in file - filename:%s",filename);
					if(!memcmp(filename,UPDATE_PACK_PREFIX,strlen(UPDATE_PACK_PREFIX)) && !memcmp(&filename[strlen(filename)-4],".zip",4)){
						err = unzOpenCurrentFilePassword(uf,UPDATE_PASSWORD);					// ｱﾌﾟﾘMD5ﾁｪｯｸｻﾑﾌｧｲﾙｵｰﾌﾟﾝ
						if( err == UNZ_OK ){
							packlen = file_info.uncompressed_size;
							packbuf = new unsigned char[packlen];
							err = unzReadCurrentFile(uf,packbuf,packlen);							// ﾌｧｲﾙﾘｰﾄﾞ
							if(err<=0){
								ERR("Pack Data Read Failed err:%d len:%d",err,packlen);
								g_updatestatus = kUpdateErr;
								return;
							}
						}
						else{
							ERR("Pack Data Open failed err:%d",err);
							g_updatestatus = kUpdateErr;
							return;
						}
					}
					else if(!strcmp(filename,UPDATE_PACK_MD5)){										// ｱﾌﾟﾘMD5ﾁｪｯｸｻﾑﾌｧｲﾙ
						err = unzOpenCurrentFilePassword(uf,UPDATE_PASSWORD);					// ｱﾌﾟﾘMD5ﾁｪｯｸｻﾑﾌｧｲﾙｵｰﾌﾟﾝ
						if( err == UNZ_OK ){
							memset(packsum,0x00,MD5_DIGEST_LENGTH+1);
							err = unzReadCurrentFile(uf,packsum,MD5_DIGEST_LENGTH);				// ﾌｧｲﾙﾘｰﾄﾞ
							if(err>0){
								packmd5flg = 1;													// ｱﾌﾟﾘMD5ﾁｪｯｸｻﾑ取得ﾌﾗｸﾞ
							}
							else{
								ERR("Package MD5 File Read Failed err:%d",err);
								g_updatestatus = kUpdateErr;
								return;
							}
						}
						else{
							ERR("Package DM5 File Open failed err:%d", err);
							g_updatestatus = kUpdateErr;
							return;
						}
					}
					if ((i+1)<gi.number_entry)
					{
						err = unzGoToNextFile(uf);
						if (err!=UNZ_OK)
						{
							ERR("error %d with zipfile in unzGoToNextFile",err);
							g_updatestatus = kUpdateErr;
							return;
						}
					}
				}
				if( result == 0 && packbuf && packmd5flg ){
					memset(md5,0x00,MD5_DIGEST_LENGTH+1);
					MD5Init(&ctx);																// ﾁｪｯｸｻﾑﾗｲﾌﾞﾗﾘ初期化
					MD5Update(&ctx,packbuf,packlen);												// ﾁｪｯｸｻﾑﾃﾞｰﾀｱｯﾌﾟﾃﾞｰﾄ
					MD5Final(md5,&ctx);															// ﾁｪｯｸｻﾑ完了
					if(!memcmp(md5,packsum,MD5_DIGEST_LENGTH)){									// ﾁｪｯｸｻﾑ一致
						sprintf(path,"%s",UPDATE_PACK_FILE);
						fp = fopen(path,"wb");									// 学習ﾃﾞｰﾀﾌｧｲﾙ書き込み
						if(fp){
							ret = fwrite(packbuf,1,packlen,fp);
							if( ret != packlen ){												// 書き込み失敗
								ERR("Pack Data File Write Failed");
								fclose(fp);
							}
							else{
								package_flg = 1;
								fclose(fp);
							}
						}
						else{
							ERR("Pack Data File Write Failed");
							g_updatestatus = kUpdateErr;
							return;
						}
					}
					else{
						ERR("Pack Data CheckSum NG");
						g_updatestatus = kUpdateErr;
						return;
					}
				}
				if(packbuf)	delete [] packbuf;
			}
			unzCloseCurrentFile(uf);
		}
		else{
			ERR("Open Update File Failed");
			g_updatestatus = kUpdateErr;
			return;
		}

		// ﾊﾟｯｹｰｼﾞﾌｧｲﾙ取り出し成功時
		if( package_flg ){																// ﾊﾟｯｹｰｼﾞﾌｧｲﾙ取り出し成功
			sprintf(path,"%s",UPDATE_PACK_FILE);
			// ｱｯﾌﾟﾃﾞｰﾄﾊﾟｯｹｰｼﾞの検索
			uf = unzOpen(path);															// ｱｯﾌﾟﾃﾞｰﾄ圧縮ﾌｧｲﾙｵｰﾌﾟﾝ
			if(uf){
				LOG(LOG_COMMON_OUT,"Open Package File");
				err = unzGetGlobalInfo (uf,&gi);												// 情報取得
				if (err!=UNZ_OK){
					ERR("error %d with zipfile in unzGetGlobalInfo ",err);
					unzCloseCurrentFile(uf);
					g_updatestatus = kUpdateErr;
					return;
				}
				else{
					for (i=0;i<gi.number_entry;i++){												// ﾌｧｲﾙ情報数分ﾙｰﾌﾟ
						char filename_inzip[256];
						char filename[256];
						char *dirEnd;
						unz_file_info file_info;
						err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);		// ｶﾚﾝﾄﾌｧｲﾙ情報取得
						if (err!=UNZ_OK)															// ﾌｧｲﾙ情報取得失敗
						{
							ERR("error %d with zipfile in unzGetCurrentFileInfo",err);
							g_updatestatus = kUpdateErr;
							return;
						}
						dirEnd = strrchr(filename_inzip,'/');
						if(dirEnd == NULL ){
							strcpy(filename,filename_inzip);
						}
						else{
							strcpy(filename,dirEnd+1);
						}


						LOG(LOG_COMMON_OUT,"Package in file - filename:%s",filename);
						string strFileName = filename;
						if(!strcmp(filename,UPDATE_APP_FILE)){											// ｱﾌﾟﾘｹｰｼｮﾝﾌｧｲﾙ
							err = unzOpenCurrentFile(uf);					// ｱﾌﾟﾘｹｰｼｮﾝﾌｧｲﾙｵｰﾌﾟﾝ
							if( err == UNZ_OK ){
								datalen = file_info.uncompressed_size;
								databuf = new unsigned char[datalen];
								err = unzReadCurrentFile(uf,databuf,datalen);							// ﾌｧｲﾙﾘｰﾄﾞ
								if(err<=0){
									ERR("APP Data Read Failed err:%d len:%d",err,datalen);
									g_updatestatus = kUpdateErr;
									return;
								}
								else{
									char oldfile[256];
									char newfile[256];
									sprintf(oldfile,"./%s",UPDATE_APP_FILE);								// 元ﾌｧｲﾙ名
									sprintf(newfile,"./old_%s",UPDATE_APP_FILE);							// 退避ﾌｧｲﾙ名
									ret = rename(oldfile,newfile);											// ﾘﾈｰﾑ
									if( !ret ){
										fp = fopen(oldfile,"wb");											// ｱﾌﾟﾘｹｰｼｮﾝﾌｧｲﾙ書き込み
										if(fp){
											fd = fileno(fp);
											ret = fwrite(databuf,1,datalen,fp);
											if( ret != datalen ){											// 書き込み失敗
												ERR("APP File Write Failed");
												fclose(fp);
												remove(oldfile);											// 書き込み失敗ﾌｧｲﾙ削除
												rename(newfile,oldfile);									// 退避ﾌｧｲﾙを戻す
												g_updatestatus = kUpdateErr;
												return;
											}
											else{
												char command[512];
												fsync(fd);
												fclose(fp);
												remove(newfile);											// 書き込み失敗ﾌｧｲﾙ削除
												sprintf(command,"chmod 777 %s",UPDATE_APP_FILE);
												ret = system(command);
											}
										}
										else{
											ERR("APP File Create Failed");
											g_updatestatus = kUpdateErr;
											return;
										}
									}
									else{
										ERR("APP File Rename Failed :%d ",ret);
										g_updatestatus = kUpdateErr;
										return;
									}
								}
								if(databuf)	delete [] databuf;
								databuf = NULL;
							}
							else{
								ERR("App File Open failed err:%d",err);
								g_updatestatus = kUpdateErr;
								return;
							}
						}
						else if(GetExtensionName(strFileName) == ".bin"){
							sprintf(path,"%s%s",WEIGHTS_FLDR,filename);			// 学習ﾃﾞｰﾀﾌｧｲﾙ
						}
						else if(GetExtensionName(strFileName) == ".cfg"){		// ｺﾝﾌｨｸﾞﾃﾞｰﾀﾌｧｲﾙ
							sprintf(path,"%s%s",CONFIG_FLDR,filename);
						}
						else if(GetExtensionName(strFileName) == ".binlut"){	// LUPﾌｧｲﾙ
							sprintf(path,"%s%s",LUT_FLDR,filename);
						}
						else if(GetExtensionName(strFileName) == ".bit"){		// fpgaﾌｧｲﾙ
							sprintf(path,"%s%s",FPGA_FLDR,filename);
						}

						err = unzOpenCurrentFile(uf);							// ｻｳﾝﾄﾞﾃﾞｰﾀﾌｧｲﾙｵｰﾌﾟﾝ
						if( err == UNZ_OK ){
							datalen = file_info.uncompressed_size;
							databuf = new unsigned char[datalen];
							err = unzReadCurrentFile(uf,databuf,datalen);						// ﾌｧｲﾙﾘｰﾄﾞ
							if(err<=0){
								ERR("%s Read Failed err:%d len:%d",filename, err, datalen);
								if(strcmp(filename,"")==true) {
									g_updatestatus = kUpdateErr;
									return;
								}
							}
							else{
								remove(path);											// 元学習ﾃﾞｰﾀ削除
								fp = fopen(path,"wb");									// 学習ﾃﾞｰﾀﾌｧｲﾙ書き込み
								if(fp){
									fd = fileno(fp);
									ret = fwrite(databuf,1,datalen,fp);
									if( ret != datalen ){												// 書き込み失敗
										ERR("%s Write Failed",filename);
										fclose(fp);
										remove(path);									// 書き込み失敗ﾌｧｲﾙ削除
										g_updatestatus = kUpdateErr;
										return;
									}
									else{
										fsync(fd);
										fclose(fp);
									}
								}
								else{
									ERR("%s Create Failed",filename);
									g_updatestatus = kUpdateErr;
									return;
								}
							}
							if(databuf)	delete [] databuf;
							databuf = NULL;
						}
						else{
							ERR("%s Open failed err:%d",filename,err);
							g_updatestatus = kUpdateErr;
							return;
						}
						if ((i+1)<gi.number_entry)
						{
							err = unzGoToNextFile(uf);
							if (err!=UNZ_OK)
							{
								ERR("error %d with zipfile in unzGoToNextFile",err);
								g_updatestatus = kUpdateErr;
								return;
							}
						}
						if(GetExtensionName(strFileName) == ".bit"){
							if(update_lattice(filename) == false) {
								g_updatestatus = kUpdateErr;
								return;
							}
						}

					}
				}
				unzCloseCurrentFile(uf);
			}
			else{
				ERR("Open Package File Failed");
				g_updatestatus = kUpdateErr;
				return;
			}
		}
		else{
			ERR("Update Package not found");
			g_updatestatus = kUpdateErr;
			return;
		}

		sprintf(path,"%s",UPDATE_PACK_FILE);
		if (stat(path, &st) == 0) {
			if((st.st_mode & S_IFMT) == S_IFREG){
				remove(path);								// ﾊﾟｯｹｰｼﾞﾌｧｲﾙ削除
			}
		}
	}
	catch(const std::exception& ex){
		ERR("update_exec err catch : %s", ex.what());
		g_updatestatus = kUpdateErr;
		return;
	}

	sync_exec();
	UnmountUSB();																// ここに書くのはあまり視認性が良くないが仕方無しと判断
	g_updatestatus = kUpdateFinsh;
}

/*-------------------------------------------------------------------
	FUNCTION :	update_main()
	EFFECTS  :	ｱｯﾌﾟﾃﾞｰﾄ処理ﾒｲﾝﾙｰﾌﾟ
	NOTE	 :	危険動作判定ｱﾌﾟﾘ/学習ﾃﾞｰﾀｱｯﾌﾟﾃﾞｰﾄ処理ﾒｲﾝﾙｰﾌﾟ
-------------------------------------------------------------------*/
int update_main(void)
{
	future<void> ws_usb;
	int exit_code = -1;						// 終了ﾌﾗｸﾞ

	g_app_state = APPSTATE_APP_STARTUP;
	som_status_send_req(g_app_state, g_gb_mode, UPDSTATE_NONE);
	OutputAppWakeup(true);
	waittime_info_send_req(g_usAccOffTimer);									// 待機時間設定通知伝文送信要求

	// FPGAﾘｾｯﾄ解除
	StartUpFPGA();

	// UPDATE
	ws_usb = std::async(std::launch::async, update_exec);						// ｱｯﾌﾟﾃﾞｰﾄ実行
	while(g_updatestatus != kUpdateFinsh){
		UpdateWDTPulse();
		if(g_updatestatus == kUpdateErr) break;
		usleep(1000);
	}

	if( g_updatestatus == kUpdateFinsh ){
		LOG(LOG_COMMON_OUT,"Update Success!!");
		// ｱｯﾌﾟﾃﾞｰﾄ成功を通知
		//som_status_send_req(g_app_state, g_gb_mode, UPDSTATE_COMPLETE);
	}
	else{
		ERR("Update Failed");
		// ｱｯﾌﾟﾃﾞｰﾄ失敗を通知
		//som_status_send_req(g_app_state, g_gb_mode, UPDSTATE_FAILED);
	}

	while (exit_code == -1) {
		UpdateWDTPulse();
		usleep(100000);										// 100mｳｪｲﾄ
		if(IsShutdownReq()){
			exit_code = 1;
		}
		int key = cust_getchar();													// ｷｰ入力取得
		if( key == 'q'){
			exit_code = 0;													// ｱﾌﾟﾘ終了
			LOG(LOG_COMMON_OUT, "update_main Q");
		}
	}

	return exit_code;
}

/*-------------------------------------------------------------------
	FUNCTION :	notifyfunc()
	EFFECTS  :	ﾀｲﾏ通知ｽﾚｯﾄﾞ
	NOTE	 :	ﾀｲﾏ通知ｽﾚｯﾄﾞ
-------------------------------------------------------------------*/
void notifyfunc(union sigval st){
	if( g_checktimer_flg == 0 ){
		g_checktimer_flg = 1;
	}
	if(g_checktimer_flg2 == 0 ){
		g_checktimer_flg2 = 1;
	}
}


/*-------------------------------------------------------------------
	FUNCTION :	main()
	EFFECTS  :	ﾒｲﾝ関数
	NOTE	 :	危険動作判定ｱﾌﾟﾘﾒｲﾝ処理
-------------------------------------------------------------------*/
int main(int argc, char **argv) {

	int ret = 1;
	int exitCode = 0;						//!< この関数を抜ける際に戻す値
	struct sigevent evp;					// シグナルイベント
	struct itimerspec itval;				// Timer割り込み間隔

	timerid = 0;							// Time ID 初期化
	g_checktimer_flg = 0;					// ﾀｲﾏ満了ﾌﾗｸﾞ初期化
	g_checktimer_flg2 = 0;					// ﾀｲﾏ満了ﾌﾗｸﾞ初期化
	sdcard_enb = 0;							// SDｶｰﾄﾞ無効に初期化
	g_app_state = APPSTATE_LINUX_STARTUP;
	g_gb_mode = FL_MODE_NORMAL;				// 非作動ﾓｰﾄﾞに初期化
	g_updatestatus = kUpdateStop;
	g_wdt_occerd = false;					// WDT発生は無しに初期化
	g_survival_cnt = 0;						// 生存確認ﾁｪｯｸ用ｶｳﾝﾄ
	g_survival_flg = 0;						// 生存確認ﾁｪｯｸﾌﾗｸﾞ

#ifdef SD_LOG_ENABLE
	g_video_status.tp2850 = 0;
	g_video_status.fpga_0 = 0;
	g_video_status.fpga_1 = 0;
	g_video_status.tp2912_0 = 0;
	g_video_status.tp2912_1 = 0;
	g_video_status.exio = 0;
#endif

	openlog("applog", LOG_PID, LOG_LOCAL5);

	// ｱﾌﾟﾘ設定ﾌｧｲﾙ読み出し
	ReadAppConf();

	// FrameBuffer初期化
	const bool isSuccessDrawInit = draw_init();
	if (isSuccessDrawInit == false) {
		ERR("draw items Initialize failed...");
	}

	// hwmon初期化
	bool isSuccess = InitHWMON();
	if(isSuccess == false){
		ERR("hwmon Initialize failed...");
	}

	memset(&evp, 0, sizeof(evp));
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = notifyfunc;
	itval.it_value.tv_sec = 2;     // 最初の1回目は2秒後
	itval.it_value.tv_nsec = 0;
	itval.it_interval.tv_sec = 1;  // 2回目以降は1秒間隔
	itval.it_interval.tv_nsec = 0;
	if(timer_create(CLOCK_REALTIME, &evp, &timerid) < 0) {
		ERR("Timer Create failed ");
	}
	if(timerid){
		if(timer_settime(timerid, 0, &itval, NULL) < 0) {
			ERR("Timer Setting failed ");
		}
	}

	// ｴﾗｰ情報初期化
	error_info_init(&errorinfo);
	error_detail_init(&errordetail);

	// RL78通信初期化
	RL78CommunicationInit();

	// ｼﾘｱﾙ通信ｽﾚｯﾄﾞ生成
	if(create_serial_thread()!=0){
		errorinfo.err2.application = 3;							// ｱﾌﾟﾘｹｰｼｮﾝ異常
		// ｴﾗｰ保存
		error_info_save(errorinfo);
		sync_exec();
	}

	// CAN通信ｽﾚｯﾄﾞ生成
	if(CreateCANThread()!=0){
		errorinfo.err2.application = 3;							// ｱﾌﾟﾘｹｰｼｮﾝ異常
		// ｴﾗｰ保存
		error_info_save(errorinfo);
		sync_exec();
	}

	// 映像出力ｽﾚｯﾄﾞ生成
	if(create_disp0_thread()!=0){
		errorinfo.err2.application = 3;							// ｱﾌﾟﾘｹｰｼｮﾝ異常
		// ｴﾗｰ保存
		error_info_save(errorinfo);
		sync_exec();
	}
	if(create_disp1_thread()!=0){
		errorinfo.err2.application = 3;							// ｱﾌﾟﾘｹｰｼｮﾝ異常
		// ｴﾗｰ保存
		error_info_save(errorinfo);
		sync_exec();
	}

	if(create_cap0_thread()!=0){
		errorinfo.err2.application = 3;							// ｱﾌﾟﾘｹｰｼｮﾝ異常
		// ｴﾗｰ保存
		error_info_save(errorinfo);
		sync_exec();
	}

	// SoMｽﾃｰﾀｽ通知
	som_status_send_req(g_app_state, g_gb_mode, UPDSTATE_NONE);

	MountUSB();
	MountSD();

	bool isNeedUpdate = update_check()!=0;						// ｱｯﾌﾟﾃﾞｰﾄﾁｪｯｸ
	if(isNeedUpdate){											// ｱｯﾌﾟﾃﾞｰﾄﾁｪｯｸ
		ret = update_main();									// ｱｯﾌﾟﾃﾞｰﾄﾒｲﾝﾙｰﾌﾟ
	}
	else{
		ret = detect_main();											// 危険動作検知ﾒｲﾝﾙｰﾌﾟ
	}

	if(timerid){
		timer_delete(timerid);
	}

	// ｼﾘｱﾙ通信ｽﾚｯﾄﾞ終了
	delete_serial_thread();
	DeleteCANThread();
	// ﾃﾞｨｽﾌﾟﾚｲｽﾚｯﾄﾞ終了
	delete_disp0_thread();
	delete_disp1_thread();

	delete_cap0_thread();

	LOG(LOG_COMMON_OUT,"Has2App End");
	UnmountSD();

	if( ret == 1 ){
		LOG(LOG_COMMON_OUT,"Shutdown Request.......");
		bool isSuccess = WriteStringFile("1", "/home/share/shutdown_req");
		g_errorManager.SetError(ErrorCode::Shutdown, !isSuccess);
	}else if ( ret == 2 ){
		LOG(LOG_COMMON_OUT,"Auto Reboot Request.......");
		exitCode = 1;
	}
	else if( ret == 3 ){
		LOG(LOG_COMMON_OUT,"Reboot Request.......");
		bool isSuccess = WriteStringFile("1", "/home/share/reboot_req");
		g_errorManager.SetError(ErrorCode::Reboot, !isSuccess);
	}
	LOG(LOG_COMMON_OUT,"End Application");

	// ﾌﾚｰﾑﾊﾞｯﾌｧ解放
	try{
		release_console();
	}
	catch (exception & rE)
	{
		ERR("release_console failed. : %s", rE.what());
		exitCode = -1;
	}

	return exitCode;
}
