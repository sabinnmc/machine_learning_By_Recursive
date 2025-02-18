/********************************************************************************
;*                                                                                *
;*        Copyright (C) 2024 PAL Inc. All Rights Reserved                         *
;*                                                                                *
;********************************************************************************
;*                                                                                *
;*        ﾓｼﾞｭｰﾙ名      :    CanCommand.cpp                                       *
;*                                                                                *
;*        作　成　者    :    内之浦　伸治                                         *
;*                                                                                *
;********************************************************************************/
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <mntent.h>
#include <fstream>

#include "Sysdef.h"
#include "Global.h"
#include "ExternalMemoryUtility.hpp"
#include "LinuxUtility.hpp"

using namespace std;


/*===	SECTION NAME		===*/

/*===	user definition		===*/

/*===	pragma variable	===*/

/*===	external variable	===*/

/*===	external function	===*/

/*===	public function prototypes	===*/

/*===	static function prototypes	===*/

/*===	static typedef variable		===*/

/*===	static variable		===*/

/*===	constant variable	===*/

/*===	global variable		===*/
int g_sd_mount = 0;													// ﾚｺｰﾀﾞｰﾏｳﾝﾄ状態


bool MountUSB(void){
	// USBがなければ失敗
	const std::ifstream ifs(MOUNT_USB_SRC_PATH);								//!< ﾌｧｲﾙの存在確認用
	if(ifs.is_open() == false){
		return false;
	}
	LOG(LOG_COMMON_OUT,"USB found %s", MOUNT_USB_SRC_PATH);
	int sysret = system((string("mount ") + MOUNT_USB_SRC_PATH + " " + MOUNT_USB_DIR).c_str());
	if(sysret != 0){
		ERR("failed mount USB");
		return false;
	}
	LOG(LOG_COMMON_OUT,"USB mount");
	return true;
}

bool UnmountUSB(void){
	std::ifstream ifs(MOUNT_USB_SRC_PATH);								//!< ﾌｧｲﾙの存在確認用
	if(ifs.is_open() == true){
		ifs.close();
		int sysret = system((string("umount ") + MOUNT_USB_DIR).c_str());
		if(sysret != 0){
			ERR("failed umount USB");
			return false;
		}
		LOG(LOG_COMMON_OUT,"USB umount");
	}
	return true;
}

bool MountSD(void){
	// SDがなければ失敗
	const std::ifstream ifs(MOUNT_SDCARD_SRC_PATH);								//!< ﾌｧｲﾙの存在確認用
	if(ifs.is_open() == false){
		LOG(LOG_COMMON_OUT,"SdCard not found %s", MOUNT_SDCARD_SRC_PATH);
		return false;
	}
	int sysret = system((string("mount ") + MOUNT_SDCARD_SRC_PATH + " " + MOUNT_SDCARD_DIR).c_str());
	if(sysret != 0){
		ERR("failed mount SD");
		return false;
	}
	LOG(LOG_COMMON_OUT,"SdCard mount");
	return true;
}

bool MountSD(int num){
	// SDがなければ失敗
	const std::ifstream ifs(MOUNT_SDCARD_SRC_PATH);								//!< ﾌｧｲﾙの存在確認用
	if(ifs.is_open() == false){
		LOG(LOG_COMMON_OUT,"SdCard not found %s", MOUNT_SDCARD_SRC_PATH);
		return false;
	}
	UnmountSD(num);																// 未ｱﾝﾏｳﾝﾄ対策(起動中の物理的な抜き差しなど)
	int sysret = system((string("mount ") + MOUNT_SDCARD_SRC_PATH + " " + MOUNT_SDCARD_DIR + to_string(num)).c_str());
	if(sysret != 0){
		ERR("failed mount SD");
		return false;
	}
	LOG(LOG_COMMON_OUT,"SdCard mount");
	return true;
}

bool UnmountSD(void){
	int sysret = system((string("umount ") + MOUNT_SDCARD_DIR).c_str());
	if(sysret != 0){
		ERR("failed umount SD");
		return false;
	}
	LOG(LOG_COMMON_OUT,"SdCard umount");
	return true;
}

bool UnmountSD(int num){
	int sysret = system((string("umount ") + MOUNT_SDCARD_DIR + to_string(num)).c_str());
	if(sysret != 0){
		ERR("failed umount SD");
		return false;
	}
	LOG(LOG_COMMON_OUT,"SdCard umount");
	return true;
}

bool IsMountedSD(void){
	string mountText;
	const bool isSuccess = ReadStringFile(&mountText, "/proc/mounts");			// ﾏｳﾝﾄ情報の確認
	if(!isSuccess || mountText.find(MOUNT_SDCARD_DIR) == std::string::npos){
		g_errorManager.SetError(ErrorCode::NotFoundMountPoint, true);
		return false;															// SDのﾏｳﾝﾄ情報が存在しない
	}

	struct stat statBuf;
	if(stat(MOUNT_SDCARD_SRC_PATH, &statBuf)){									// ﾃﾞﾊﾞｲｽ接続の確認
		g_errorManager.SetError(ErrorCode::NotFoundMountPoint, true);
		return false;															// SDのﾃﾞﾊﾞｲｽが存在しない
	}

	return true;
}

/*-------------------------------------------------------------------
	FUNCTION :	CheckSSDMount()
	EFFECTS  :	SSDﾏｳﾝﾄ確認
	NOTE	 :	SSDﾏｳﾝﾄ確認
-------------------------------------------------------------------*/
bool CheckSSDMount(void){
	FILE *fp;
	struct mntent *e;
	struct stat st;
	bool result = false;
	char path[257];
	int ret = 0;
	try{
		fp = setmntent("/proc/mounts", "r");						// ﾏｳﾝﾄﾎﾟｲﾝﾄ取得

		if( fp ){
			while((e = getmntent(fp))!= NULL ){
				if(!strcmp(MOUNT_REDODER_DIR.c_str(),e->mnt_dir)){			// ﾏｳﾝﾄﾎﾟｲﾝﾄがSDｶｰﾄﾞﾏｳﾝﾄ先
					result = true;
					sprintf(path,"%s/%s",MOUNT_REDODER_DIR.c_str(),RECODER_DIR.c_str());
					if(stat(path, &st) != 0){									// 動画保存ﾌｫﾙﾀﾞﾁｪｯｸ
						ret = mkdir(path,0777);										// 動画保存ﾌｫﾙﾀﾞが無い場合作成
						if( ret != 0 ){
							result = false;
							ERR("Recoder Folder Create Failed");
						}
					}
					break;
				}

			}
			if(result){
				LOG(LOG_COMMON_OUT,"SSD Mounted");
			}
			endmntent(fp);
		}
	}
	catch(const std::exception& ex){
		ERR("CheckSSDMount err catch");
	}
	return(result);
}


/*-------------------------------------------------------------------
	FUNCTION :	SSDSecureFreeSpace()
	EFFECTS  :	SSD空き容量確認
	NOTE	 :	SSD空き容量確認
-------------------------------------------------------------------*/
int SSDSecureFreeSpace(ulonglong size)
{
	struct statvfs buf = {0};
	ulonglong freesize;
	int ret = 0;
	int result = -1;

	try{
		ret = statvfs(MOUNT_REDODER_DIR.c_str(), &buf);									// ﾃﾞｨﾚｸﾄﾘ情報取得
		if (ret != 0) {
			ERR("statvfs() Failed :%d",ret);
			return -1;
		}
		freesize = (long long)buf.f_bfree * (long long)buf.f_bsize;				// 空き容量算出
		if( freesize >= size ){										// 既定ｻｲｽﾞ以上空き確認
			LOG(LOG_RECODER_DEBUG,"SSD enough free space : %llX",freesize);
			result = 1;
		}
		else{
			LOG(LOG_COMMON_OUT,"SSD not enough free space : %llX",freesize);
			result = 0;
		}
	}
	catch(const std::exception& ex){
		ERR("SSDSecureFreeSpace err catch");
		return -1;
	}
	return(result);
}


/*-------------------------------------------------------------------
	FUNCTION :	sdcard_test()
	EFFECTS  :	SDｶｰﾄﾞﾃｽﾄ(出荷検査)
	NOTE	 :	出荷検査時のSDｶｰﾄﾞﾃｽﾄを実行する
-------------------------------------------------------------------*/
int sdcard_test(void)
{
	int result = 0;
	int ret;
	FILE *fp = NULL;
	int fd;
	char filepath[256];
	char str[100];
	char rstr[100];
	int i;

	if( !g_sd_mount ){											// SDｶｰﾄﾞ未挿入
		ERR("SD Card Not Insert");
		return -1;
	}

	try{
		sprintf(filepath,"%s/sdtest.txt",MOUNT_SDCARD_DIR);
		fp = fopen(filepath, "w");							// ﾌｧｲﾙｵｰﾌﾟﾝ
		if (!fp) {
			ERR("Could not open %s for writing", filepath);
			result = -1;
			return(result);
		}
		fd = fileno(fp);
		for( i = 0; i < 100; i++ ){
			str[i] = i;
		}
		ret = fwrite(str,1,sizeof(str),fp);
		if( ret != (int)sizeof(str) ){
			ERR("sdcard_test Write Length Error %d", ret);
			result = -1;
		}
		fsync(fd);
		fclose(fp);													// ﾌｧｲﾙｸﾛｰｽﾞ
		fp = fopen(filepath, "r");							// ﾌｧｲﾙｵｰﾌﾟﾝ
		if (!fp) {
			ERR("Could not open %s for reading", filepath);
			result = -1;
			return(result);
		}
		ret = fread(rstr,1,sizeof(rstr),fp);
		if( ret != (int)sizeof(rstr) ){
			ERR("sdcard_test Read Length Error %d", ret);
			result = -1;
		}
		else{
			if(memcmp(str,rstr,100) != 0 ){
				ERR("sdcard_test Verify NG!");
				result = -1;
			}
		}
		fclose(fp);													// ﾌｧｲﾙｸﾛｰｽﾞ
		remove(filepath);											// ﾃｽﾄﾌｧｲﾙ削除
	}
	catch(const std::exception& ex){
		ERR("sdcard_test err catch");
		return -1;
	}

	return(result);
}
