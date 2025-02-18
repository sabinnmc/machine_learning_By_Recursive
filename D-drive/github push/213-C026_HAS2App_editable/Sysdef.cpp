//--------------------------------------------------
//! @file Sysdef.cpp
//! @brief システム定義
//--------------------------------------------------

#include "Sysdef.h"
#include "HailoDetect.h"

BboxInfo::BboxInfo(const boundingbox& box){
	*this = box;
}
BboxInfo& BboxInfo::operator=(const boundingbox& box){
	cls_idx = box.label;
	x1 = box.x1;
	y1 = box.y1;
	x2 = box.x2;
	y2 = box.y2;
	flg = box.flg;
	score = box.score;

	return *this;
}
