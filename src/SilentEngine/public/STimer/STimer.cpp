// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#include "STimer.h"


STimer::STimer()
{
	bRunning = false;

	dTimeInPauseInMS = 0.0;
}

void STimer::start()
{
	dTimeInPauseInMS = 0.0;

	startTime = std::chrono::steady_clock::now();

	bRunning = true;
}

void STimer::stop()
{
	if (bRunning)
	{
		bRunning = false;
	}
}

double STimer::getElapsedTimeInMS()
{
	if (bRunning)
	{
		double dTimeElapsedInMS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - startTime).count() / 1000000.0;

		return dTimeElapsedInMS - dTimeInPauseInMS;
	}
	else
	{
		return 0.0f;
	}
}

void STimer::pause()
{
	if (bRunning)
	{
		pauseTime = std::chrono::steady_clock::now();

		bRunning = false;
	}
}

void STimer::unpause()
{
	if (bRunning == false)
	{
		std::chrono::time_point<std::chrono::steady_clock> unpauseTime = std::chrono::steady_clock::now();

		dTimeInPauseInMS += static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(unpauseTime - pauseTime).count());

		bRunning = true;
	}
}

double STimer::getElapsedTimeInSec()
{
	if (bRunning)
	{
		return getElapsedTimeInMS() / 1000.0;
	}
	else
	{
		return 0.0f;
	}
}