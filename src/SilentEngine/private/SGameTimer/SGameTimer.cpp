// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#include "SGameTimer.h"

#include <Windows.h>

SGameTimer::SGameTimer()
{
	dSecondsPerCount        = 0.0;
	dDeltaTimeBetweenTicks = -1.0;

	iResetTime   = 0;
	iPausedTime  = 0;
	iPauseTime   = 0;
	iPrevTime    = 0;
	iCurrentTime = 0;

	bOnPause = false;



	// Get time between counts.

	__int64 iCountsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&iCountsPerSec);
	dSecondsPerCount = 1.0 / static_cast<double>(iCountsPerSec);
}

void SGameTimer::pause()
{
	if (bOnPause == false)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&iPauseTime);

		bOnPause = true;
	}
}

void SGameTimer::tick()
{
	if (bOnPause)
	{
		dDeltaTimeBetweenTicks = 0.0;
		return;
	}

	QueryPerformanceCounter((LARGE_INTEGER*)&iCurrentTime);

	dDeltaTimeBetweenTicks = (iCurrentTime - iPrevTime) * dSecondsPerCount;

	iPrevTime = iCurrentTime;

	// Force nonnegative. The DXSDK's CDXUTTimer mentions that if the 
	// processor goes into a power save mode or we get shuffled to another
	// processor, then dDeltaTimeBetweenFrames can be negative.

	if (dDeltaTimeBetweenTicks < 0.0)
	{
		dDeltaTimeBetweenTicks = 0.0;
	}
}

float SGameTimer::getTimeElapsedInSec() const
{
	if (bOnPause)
	{
		//                     |<--iPausedTime-->|
		// ----*---------------*-----------------*------------*-------------*------> time
		//  iResetTime       pause()         unpause()     iPauseTime   Current Time

		// We are paused, do not count the time that has passed since we paused.

		__int64 iPassedCounts = (iPauseTime - iPausedTime) - iResetTime;

		return static_cast<float>(iPassedCounts * dSecondsPerCount);
	}
	else
	{
		//                     |<--iPausedTime-->|
		// ----*---------------*-----------------*--------------*------> time
		//  iResetTime       pause()         unpause()     Current Time

		__int64 iPassedCounts = (iCurrentTime - iPausedTime) - iResetTime;

		return static_cast<float>(iPassedCounts * dSecondsPerCount);
	}
}

float SGameTimer::getDeltaTimeBetweenTicksInSec() const
{
	return static_cast<float>(dDeltaTimeBetweenTicks);
}

void SGameTimer::reset()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&iCurrentTime);

	iResetTime = iCurrentTime;
	iPrevTime  = iCurrentTime;
	iPauseTime = 0;
	
	bOnPause = false;
}

void SGameTimer::unpause()
{
	if (bOnPause)
	{
		__int64 iUnpauseTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&iUnpauseTime);

		iPausedTime += (iUnpauseTime - iPauseTime);

		iPrevTime = iUnpauseTime;
		iPauseTime = 0;

		bOnPause = false;
	}
}
