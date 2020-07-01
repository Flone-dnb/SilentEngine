// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

class SApplication;

//@@Class
/*
The class is used for dynamic analysis, for example, for measuring draw call amount, FPS, or video memory used by the app.
*/
class SProfiler
{
public:
	//@@Function
	SProfiler(SApplication* pApp);
	SProfiler() = delete;
	SProfiler(const SProfiler&) = delete;
	SProfiler& operator= (const SProfiler&) = delete;


	//@@Function
	/*
	* desc: returns the time in seconds elapsed since the SApplication::run() was called.
	* param "fTimeInSec": pointer to your float value which will be used to set the time value.
	* return: false if successful, true otherwise.
	* remarks: should be called after calling the SApplication::run().
	*/
	bool   getTimeElapsedFromStart                           (float* fTimeInSec) const;
	//@@Function
	/*
	* desc: returns the current number of frames per second generated by the application.
	* param "iFPS": pointer to your int value which will be used to set the FPS value.
	* return: false if successful, true otherwise.
	* remarks: should be called after calling the SApplication::run().
	*/
	bool   getFPS                                            (int* iFPS)         const;
	//@@Function
	/*
	* desc: returns the time it was taken to render the last frame.
	* param "fTimeInMS": pointer to your float value which will be used to set the time value.
	* return: false if successful, true otherwise.
	* remarks: should be called after calling the SApplication::run().
	*/
	bool   getTimeToRenderFrame                              (float* fTimeInMS)  const;
	//@@Function
	/*
	* desc: returns the number of the draw calls it was made to render the last frame.
	* param "iDrawCallCount": pointer to your unsigned long long value which will be used to set the draw call count value.
	* return: false if successful, true otherwise.
	* remarks: should be called on tick(), otherwise it will have an incorrect number. The function is also should be
	called only after calling the SApplication::run().
	*/
	bool   getLastFrameDrawCallCount                         (unsigned long long* iDrawCallCount) const;
	//@@Function
	/*
	* desc: returns the number of triangles in the world (current level).
	* return: 0 if there is no current level loaded or no objects in the scene, otherwise the triangle count in the world.
	*/
	unsigned long long getTriangleCountInWorld               ();
	//@@Function
	/*
	* desc: returns currently used memory space (i.e. how much of the VRAM is used) of the display adapter (i.e. "video card").
	* param "pSizeInBytes": pointer to your unsigned long long value which will be used to set the memory value.
	* return: false if successful, true otherwise.
	* remarks: should be called after calling the SApplication::init().
	*/
	bool   getVideoMemoryUsageInBytesOfCurrentDisplayAdapter (unsigned long long* pSizeInBytes);


private:

	//@@Variable
	/* pointer to the SApplication which will be profiled. */
	SApplication* pApp;
};
