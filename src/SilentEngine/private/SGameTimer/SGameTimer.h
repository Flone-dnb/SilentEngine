// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#pragma once

class SGameTimer
{
public:

	SGameTimer();

	float getTimeElapsedInSec() const;
	float getDeltaTimeBetweenFramesInSec() const;

	void reset();
	void unpause();
	void pause();
	void tick();

private:

	double  dSecondsPerCount;
	double  dDeltaTimeBetweenFrames;

	__int64 iResetTime;
	__int64 iPausedTime; // all time in pause mode
	__int64 iPauseTime;
	__int64 iPrevTime;
	__int64 iCurrentTime;

	bool    bOnPause;
};

