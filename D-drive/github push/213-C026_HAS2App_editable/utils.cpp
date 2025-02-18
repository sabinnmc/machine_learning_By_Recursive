/***********************************************************************
*
* Copyright (c) 2017-2019 Gyrfalcon Technology Inc. All rights reserved.
* See LICENSE file in the project root for full license information.
*
************************************************************************/
#include "utils.hpp"
#include <chrono>
#include <iostream>

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
