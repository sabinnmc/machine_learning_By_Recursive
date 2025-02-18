/***********************************************************************
*
* Copyright (c) 2017-2019 Gyrfalcon Technology Inc. All rights reserved.
* See LICENSE file in the project root for full license information.
*
************************************************************************/

#include <chrono>
#include <iostream>

#pragma once

typedef std::chrono::steady_clock::time_point TP;

std::chrono::steady_clock::time_point getTime();

uint64_t getTimeDiff(std::chrono::steady_clock::time_point nTS, std::chrono::steady_clock::time_point nTE);

float getFPS(std::chrono::steady_clock::time_point nTS, std::chrono::steady_clock::time_point nTE, int nNumFrames, uint64_t nTimeAdj = 0);

