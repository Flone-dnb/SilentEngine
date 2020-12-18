// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#include "STimer.h"

#include <windows.h>

STimer::STimer()
{
	bRunning = false;
	bTimeoutEnabled = false;

	dTimeInPauseInMS = 0.0;
	fTimeInSecToTimeout = 0.0f;

	fTimerAccuracyInSec = 0.1f;
}

void STimer::setCallbackOnTimeout(std::function<void(void)> function, float fTimeInSecToTimeout, bool bLooping, float fTimerAccuracyInSec)
{
	bUsingCustomData = false;

	timeoutFunction = function;

	this->fTimeInSecToTimeout = fTimeInSecToTimeout;
	this->bLooping = bLooping;

	if (fTimerAccuracyInSec > 0.1f)
	{
		this->fTimerAccuracyInSec = 0.1f;
	}
	else
	{
		this->fTimerAccuracyInSec = fTimerAccuracyInSec;
	}

	bTimeoutEnabled = true;
}

void STimer::setCallbackOnTimeout(std::function<void(char[64])> function, char customData[64], float fTimeInSecToTimeout, bool bLooping, float fTimerAccuracyInSec)
{
	bUsingCustomData = true;

	memset(this->customData, 0, 64);
	std::memcpy(this->customData, customData, 64);

	timeoutFunctionWithCustomData = function;

	this->fTimeInSecToTimeout = fTimeInSecToTimeout;
	this->bLooping = bLooping;

	if (fTimerAccuracyInSec > 0.1f)
	{
		this->fTimerAccuracyInSec = 0.1f;
	}
	else
	{
		this->fTimerAccuracyInSec = fTimerAccuracyInSec;
	}

	bTimeoutEnabled = true;
}

void STimer::start()
{
	dTimeInPauseInMS = 0.0;

	startTime = std::chrono::steady_clock::now();

	mtxStop.lock();
	bRunning = true;
	mtxStop.unlock();

	if (bTimeoutEnabled)
	{
		iCurrentCallbackTimerIndex++;

		std::thread tTimeout(&STimer::timerTimeoutFunction, this, iCurrentCallbackTimerIndex);
		tTimeout.detach();
	}
}

void STimer::stop()
{
	if (bTimeoutEnabled)
	{
		bTimeoutEnabled = false;
	}

	mtxStop.lock();

	if (bRunning)
	{
		bRunning = false;

		Sleep(fTimerAccuracyInSec + 1); // not good
	}

	mtxStop.unlock();
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

bool STimer::isTimerRunning()
{
	mtxStop.lock();
	bool bRet = bRunning;
	mtxStop.unlock();

	return bRet;
}

void STimer::timerTimeoutFunction(size_t iCurrentCallbackTimerIndex)
{
	do
	{
		dTimeInPauseInMS = 0.0;

		startTime = std::chrono::steady_clock::now();

		do
		{
			std::this_thread::sleep_for(std::chrono::duration<float>(fTimerAccuracyInSec));

			if (iCurrentCallbackTimerIndex != this->iCurrentCallbackTimerIndex)
			{
				// User set the next callback, finish this one.
				return;
			}
		}while ((getElapsedTimeInSec() < fTimeInSecToTimeout) && bRunning);

		if (bRunning)
		{
			if (bUsingCustomData)
			{
				timeoutFunctionWithCustomData(customData);
			}
			else
			{
				timeoutFunction();
			}
		}

	}while(bLooping && bRunning);
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

		dTimeInPauseInMS += std::chrono::duration_cast<std::chrono::nanoseconds>(unpauseTime - pauseTime).count() / 1000000.0;

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