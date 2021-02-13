// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SProfiler.h"

// Custom
#include "SilentEngine/Public/SApplication/SApplication.h"

SProfiler::SProfiler(SApplication* pApp)
{
	this->pApp = pApp;
}

bool SProfiler::getTimeElapsedFromStart(float* fTimeInSec) const
{
	return pApp->getTimeElapsedFromStart(fTimeInSec);
}

bool SProfiler::getFPS(int* iFPS) const
{
	return pApp->getFPS(iFPS);
}

bool SProfiler::getTimeToRenderFrame(float* fTimeInMS) const
{
	return pApp->getTimeToRenderFrame(fTimeInMS);
}

#if defined(DEBUG) || defined(_DEBUG)
SFrameStats SProfiler::getFrameStats()
{
	std::lock_guard<std::mutex> lock(mtxFrameStats);

	return lastFrameStats;
}
#endif

float SProfiler::getTimeSpentWaitingForGPUBetweenFramesInMS()
{
	return fTimeSpentWaitingForGPUBetweenFramesInMS;
}

bool SProfiler::getLastFrameDrawCallCount(unsigned long long* iDrawCallCount) const
{
	return pApp->getLastFrameDrawCallCount(iDrawCallCount);
}

bool SProfiler::getVideoMemoryUsageInBytesOfCurrentDisplayAdapter(unsigned long long* pSizeInBytes)
{
	return pApp->getVideoMemoryUsageInBytesOfCurrentDisplayAdapter(pSizeInBytes);
}

#if defined(DEBUG) || defined(_DEBUG)
void SProfiler::setFrameStats(const SFrameStats& frameStats)
{
	std::lock_guard<std::mutex> lock(mtxFrameStats);

	lastFrameStats = frameStats;
}
#endif