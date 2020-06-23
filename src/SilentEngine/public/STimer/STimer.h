// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#pragma once

#include <chrono>
#include <functional>
#include <thread>

//@@Class
/*
The class provides the timer functionality. Get the elapsed time since the timer started, or set a timeout which will call your function.
*/
class STimer
{
public:

	//@@Function
	STimer();

	//@@Function
	/*
	* desc: sets the callback function, which will be called after the fTimeInSecToTimeout expired
	from the time when the start() was called.
	* param "function": the function which will be called after the fTimeInSecToTimeout has expired since start() call.
	* param "fTimeInSecToTimeout": time in seconds after which callback function will be called.
	* param "bLooping": if true then after the callback function was called timer will repeat itself from the beginning,
	and after another fTimeInSecToTimeout will again call callback function an so on and on until the stop() will not be called.
	* param "fTimerAccuracyInSec": determines the time after which the timer checks whether it has exceeded fTimeInSecToTimeout or not.
	This value cannot be higher than 0.1f.
	* remarks: setting the fTimeInSecToTimeout to a, for example, 0.75f while the fTimerAccuracyInSec = 0.1f will result in the timer
	calling callback function after the 0.8f not the 0.75f. An example of std::function pointing to a member function of a class -
	"std::function<void(void)> f = std::bind(&Foo::func, this);".
	*/
	void setCallbackOnTimeout(std::function<void(void)> function, float fTimeInSecToTimeout, bool bLooping, float fTimerAccuracyInSec);

	//@@Function
	/*
	* desc: starts the timer and callback timer if the setCallbackOnTimeout() and setTime() was called.
	*/
	void start();

	//@@Function
	/*
	* desc: stops the timer and callback timer if the setCallbackOnTimeout() and setTime() was called. After
	calling this function getElapsedTimeInSec() and getElapsedTimeInMS() will always return 0.0f.
	*/
	void stop();

	//@@Function
	/*
	* desc: pauses the timer, but getElapsedTimeInSec() and getElapsedTimeInMS() will exclude paused time only after the
	unpause() call.
	*/
	void pause();

	//@@Function
	/*
	* desc: unpauses the timer, so that getElapsedTimeInSec() and getElapsedTimeInMS() will exclude paused time now.
	*/
	void unpause();

	//@@Function
	/*
	* desc: returns the time in seconds that has passed since the start() call was made.
	* return: time in seconds that has passed since the start() call was made.
	* remarks: note that elapsed time will exclude paused time only after the unpause() was called.
	*/
	double getElapsedTimeInSec();

	//@@Function
	/*
	* desc: returns the time in milliseconds that has passed since the start() call was made.
	* return: time in milliseconds that has passed since the start() call was made.
	* remarks: note that elapsed time will exclude paused time only after the unpause() was called.
	*/
	double getElapsedTimeInMS();

private:

	//@@Function
	/*
	* desc: this function loops waiting for the timeout to happen.
	*/
	void timerTimeoutFunction(size_t iCurrentCallbackTimerIndex);

	//@@Variable
	/* holds the time when the start() was called. */
	std::chrono::time_point<std::chrono::steady_clock> startTime;

	//@@Variable
	/* holds the time when the pause() was called. */
	std::chrono::time_point<std::chrono::steady_clock> pauseTime;

	//@@Variable
	/* function which will be called after the timeout. */
	std::function<void(void)> timeoutFunction;

	//@@Variable
	/* after each start() with callback enabled will be increased. */
	size_t iCurrentCallbackTimerIndex = 0;

	//@@Variable
	/* time spend in pause (recalculated after the unpause() call). */
	double dTimeInPauseInMS;

	//@@Variable
	/* time to timeout and call callback function "timeoutFunction". */
	float fTimeInSecToTimeout;
	//@@Variable
	/* time after which callback timer will check if the "fTimeInSecToTimeout" was exceeded. */
	float fTimerAccuracyInSec;

	//@@Variable
	/* true if the start() was called. */
	bool bRunning;

	//@@Variable
	/* true if we should repeat callback timer and wait another fTimeInSecToTimeout. */
	bool bLooping;
	//@@Variable
	/* true if setCallbackOnTimeout() was called. */
	bool bTimeoutEnabled;
};

