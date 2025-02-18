//--------------------------------------------------
//! @file ErrorManager.cpp
//! @brief ｴﾗｰ管理ｸﾗｽ
//--------------------------------------------------
#include "ErrorManager.hpp"

#include <string>

#include "Global.h"
#include "LinuxUtility.hpp"

using namespace std;

CErrorManager::CErrorManager(){
	m_errorTable = vector<bool>(static_cast<int>(ErrorCode::Max), false);
}

bool CErrorManager::IsErrorOr(vector<ErrorCode> code)const{
	for(const auto c : code){
		if(IsError(c)){
			return true;
		}
	}
	return false;
}
bool CErrorManager::IsErrorAnd(vector<ErrorCode> code)const{
	for(const auto c : code){
		if(IsError(c) == false){
			return false;
		}
	}
	return true;
}

void CErrorManager::SetError(ErrorCode code, bool isError){
	int index = static_cast<int>(code);
	if(m_errorTable[index] != isError){
		m_errorTable[index] = isError;
		OutputLog(isError);
	}
}

void CErrorManager::OutputLog(bool isError){
	string text = BinToHexString(m_errorTable);
	if(isError){
		ERR("Error occurred : %s", text.c_str());
	}else{
		LOG(LOG_COMMON_OUT, "Error eliminating : %s", text.c_str());
	}
}
