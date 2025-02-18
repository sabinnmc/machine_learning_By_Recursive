//--------------------------------------------------
//! @file ExternalMemoryUtility.hpp
//! @brief 外部メモリに関するﾕｰﾃｨﾘﾃｨﾓｼﾞｭｰﾙ
//!
//! USBメモリ or SDカードの付属的な機能
//--------------------------------------------------

#pragma once
#include <tuple>
#include <vector>
#include "Sysdef.h"

extern int g_sd_mount;														// レコーダマウント状態


//--------------------------------------------------
//! @brief SDｶｰﾄﾞのﾏｳﾝﾄ
//! @param[in]		num		MOUNT_SDCARD_DIR + {num} (/mnt/card{num})
//! @retval	true	成功
//! @retval	false	失敗
//--------------------------------------------------
bool MountSD(void);
bool MountSD(int num);

//--------------------------------------------------
//! @brief USBのﾏｳﾝﾄ
//! @retval	true	成功
//! @retval	false	失敗
//--------------------------------------------------
bool MountUSB(void);

//--------------------------------------------------
//! @brief USBのｱﾝﾏｳﾝﾄ
//! @retval	true	成功
//! @retval	false	失敗
//--------------------------------------------------
bool UnmountUSB(void);

//--------------------------------------------------
//! @brief SDｶｰﾄﾞのｱﾝﾏｳﾝﾄ
//! @param[in]		num		MOUNT_SDCARD_DIR + {num} (/mnt/card{num})
//! @retval	true	成功
//! @retval	false	失敗
//--------------------------------------------------
bool UnmountSD(void);
bool UnmountSD(int num);

//--------------------------------------------------
//! @brief SDｶｰﾄﾞがﾏｳﾝﾄされているか
//!
//! ｴﾗｰ発生時は ErrorManager に ErrorCode::NotFoundMountPoint を登録
//! @retval	true	ﾏｳﾝﾄされている
//! @retval	false	ﾏｳﾝﾄされていない or ｴﾗｰ発生
//--------------------------------------------------
bool IsMountedSD(void);

//--------------------------------------------------
//! @brief SSDﾏｳﾝﾄ確認
//!
//! @retval	true		ﾏｳﾝﾄ済み
//! @retval	false		未ﾏｳﾝﾄ
//--------------------------------------------------
bool CheckSSDMount(void);

//--------------------------------------------------
//! @brief SSD空き容量確認
//!
//! @param[in]		size				空き容量ｻｲｽﾞ
//! @retval	1			指定ｻｲｽﾞの空き容量あり
//! @retval	0			空き容量が指定ｻｲｽﾞ未満
//! @retval	-1			処理失敗
//--------------------------------------------------
int SSDSecureFreeSpace(ulonglong size);
