// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#pragma once

#include <chrono>


class STimer
{
public:

	STimer();

	void start();
	void stop();

	void pause();
	void unpause();

	double getElapsedTimeInSec();
	double getElapsedTimeInMS();

private:
	std::chrono::time_point<std::chrono::steady_clock> startTime;
	std::chrono::time_point<std::chrono::steady_clock> pauseTime;

	double dTimeInPauseInMS;

	bool bRunning;
};

