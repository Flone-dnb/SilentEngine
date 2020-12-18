// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <string>
#include <vector>

// Custom
#include "SilentEngine/Public/SVector/SVector.h"
#include "SilentEngine/Private/SFrameResource/SFrameResource.h"
#include <dxgiformat.h>

//@@Struct
/*
Used to hold two values: width and height.
*/
struct SScreenResolution
{
	//@@Variable
	unsigned int iWidth;
	//@@Variable
	unsigned int iHeight;
};

enum MSAASampleCount
{
	SC_2  = 2,
	SC_4  = 4
};

class SApplication;

//@@Class
/*
The class is used to set and get video settings of a SApplication instance.
All "...init...()" functions should be called before calling the SApplication::init().
*/
class SVideoSettings
{
public:

	//@@Function
	SVideoSettings(SApplication* pApp);
	SVideoSettings() = delete;
	SVideoSettings(const SVideoSettings&) = delete;
	SVideoSettings& operator= (const SVideoSettings&) = delete;


	// Init

		//@@Function
		/*
		* desc: used to set the preferred display adapter (i.e. "video card" on your PC). The list of all supported display
		adapters may be retrieved through SVideoSettings::getSupportedDisplayAdapters() function.
		* param "sPreferredDisplayAdapter": the name of the preferred display adapter.
		* return: false if successful, true otherwise.
		* remarks: should be called before calling the SApplication::init().
		*/
		bool      setInitPreferredDisplayAdapter	   (std::wstring sPreferredDisplayAdapter);
		//@@Function
		/*
		* desc: used to set the preferred output adapter (i.e. monitor on your PC). The list of all supported output adapters may be
		retrieved through SVideoSettings::getOutputDisplaysOfCurrentDisplayAdapter() function.
		* param "sPreferredOutputAdapter": the name of the preferred output adapter.
		* return: false if successful, true otherwise.
		* remarks: should be called before calling the SApplication::init().
		*/
		bool      setInitPreferredOutputAdapter		   (std::wstring sPreferredOutputAdapter);
		//@@Function
		/*
		* desc: determines if the application should run in fullscreen mode.
		* param "bFullscreen": true for fullscreen, false for windowed mode.
		* return: false if successful, true otherwise.
		* remarks: should be called before calling the SApplication::init().
		*/
		bool      setInitFullscreen					   (bool bFullscreen);
		//@@Function
		/*
		* desc: used to enable VSync.
		* param "bEnable": true to enable VSync, false otherwise.
		* return: false if successful, true otherwise.
		* remarks: should be called before calling the SApplication::init().
		*/
		bool      setInitEnableVSync                   (bool bEnable);


	// Game

	//@@Function
	/*
	* desc: used to set the FPS limit (FPS cap).
	* param "fFPSLimit": the value which the generated framerate should not exceed. 0 to disable FPS limit.
	* remarks: set fFPSLimit to 0 to disable FPS limit.
	*/
	void      setFPSLimit                          (float fFPSLimit);


	// MSAA

	//@@Function
	/*
	* desc: used to enable/disable the MSAA - anti-aliasing technique.
	* param "bEnable": true to enable MSAA, false otherwise.
	* remarks: MSAA is enabled by default. Enabling MSAA might cause a slight decrease in performance.
	*/
	void      setMSAAEnabled                       (bool bEnable);
	//@@Function
	/*
	* desc: used to set the sample count (i.e. quality) of the MSAA.
	* param "sampleCount": sample count (i.e. quality).
	* remarks: the default sample count is 4. The higher the number of samples the lower the performance might get.
	*/
	bool      setMSAASampleCount                   (MSAASampleCount sampleCount);
	//@@Function
	/*
	* desc: used to determine if the MSAA is enabled.
	* return: true if MSAA is enabled, false otherwise.
	* remarks: MSAA is enabled by default.
	*/
	bool      isMSAAEnabled                        ();
	//@@Function
	/*
	* desc: used to retrieve the number of samples (i.e. quality) used by the MSAA.
	* return: number of samples used by the MSAA.
	* remarks: the default sample count is 4.
	*/
	MSAASampleCount getMSAASampleCount             ();


	// Screen

	//@@Function
	/*
	* desc: used to set the screen resolution.
	* param "screenResolution": screen resolution to be set.
	* return: false if successful, true otherwise.
	*/
	bool      setScreenResolution                  (SScreenResolution screenResolution);
	//@@Function
	/*
	* desc: used to switch between fullscreen and windowed modes.
	* param "bFullscreen": true for fullscreen mode, false for windowed.
	* return: false if successful, true otherwise.
	*/
	bool      setFullscreenWithCurrentResolution   (bool bFullscreen);

	//@@Function
	/*
	* desc: used to retrieve current screen resolution.
	* param "pScreenResolution": pointer to the struct in which the screen resolution width and height values will be set.
	* return: false if successful, true otherwise.
	*/
	bool      getCurrentScreenResolution           (SScreenResolution* pScreenResolution);
	//@@Function
	/*
	* desc: used to determine if the current screen mode is fullscreen.
	* return: false if successful, true otherwise.
	*/
	bool      isFullscreen                         ();
	//@@Function
	/*
	* desc: used to retrieve the current screen aspect ratio.
	*/
	float     getScreenAspectRatio                 ();


	// Graphics

	//@@Function
	/*
	* desc: sets the texture filter mode.
	* param "textureFilterMode": point filter - fast, bad quality,
	linear filter - medium quality,
	anisotropic filter - slow, best quality.
	* remarks: default texture filter is anisotropic filter.
	*/
	void      setTextureFilterMode                 (TEX_FILTER_MODE textureFilterMode);
	//@@Function
	/*
	* desc: returns the texture filter mode.
	* remarks: default texture filter is anisotropic filter.
	*/
	TEX_FILTER_MODE getTextureFilterMode           ();
	//@@Function
	/*
	* desc: used to set the "background" color of the world.
	* param "vColor": the vector that contains the color in XYZ as RGB.
	*/
	void      setBackBufferFillColor               (SVector vColor);
	//@@Function
	/*
	* desc: used to enable/disable wireframe display mode.
	* param "bEnable": true to enable, false to disable.
	*/
	void      setEnableWireframeMode               (bool bEnable);

	//@@Function
	/*
	* desc: used to retrieve the "background" color of the world.
	* return: the vector that contains the color in XYZ as RGB.
	*/
	SVector   getBackBufferFillColor               ();
	//@@Function
	/*
	* desc: used to determine if the wireframe display mode is enabled.
	* return: true if enabled, false otherwise.
	*/
	bool      isWireframeModeEnabled               ();

	
	// Display adapters.

	//@@Function
	/*
	* desc: returns the list of the display adapters (i.e. "video cards") on your PC that engine supports.
	* remarks: should be called after calling the SApplication::init().
	*/
	std::vector<std::wstring>  getSupportedDisplayAdapters ();
	//@@Function
	/*
	* desc: returns the current display adapter (i.e. "video card") that is being used.
	* remarks: should be called after calling the SApplication::init().
	*/
	std::wstring               getCurrentDisplayAdapter    ();
	//@@Function
	/*
	* desc: returns currently used memory space (i.e. how much of the VRAM is used) of the display adapter (i.e. "video card").
	* param "pSizeInBytes": pointer to your unsigned long long value which will be used to set the memory value.
	* return: false if successful, true otherwise.
	* remarks: should be called after calling the SApplication::init().
	*/
	bool                       getVideoMemoryUsageInBytesOfCurrentDisplayAdapter(UINT64* pSizeInBytes) const;
	//@@Function
	/*
	* desc: used to retrieve the size of the VRAM (video memory) of the current display adapter (i.e. "video card").
	* return: false if successful, true otherwise.
	* remarks: should be called after calling the SApplication::init().
	*/
	bool                       getVideoMemorySizeInBytesOfCurrentDisplayAdapter  (unsigned long long* pSizeInBytes);


	// Output display.

	//@@Function
	/*
	* desc: returns the list of the output adapters (i.e. monitors) on your PC that support current display adapter (i.e. "video card").
	* remarks: should be called after calling the SApplication::init().
	*/
	std::vector<std::wstring>  getOutputDisplaysOfCurrentDisplayAdapter();
	//@@Function
	/*
	* desc: returns the name of the current output adapter (i.e. monitor).
	* remarks: should be called after calling the SApplication::init().
	*/
	std::wstring               getCurrentOutputDisplay                 ();
	//@@Function
	/*
	* desc: returns the refresh rate value of the current output adapter (i.e. monitor).
	* remarks: should be called after calling the SApplication::init().
	*/
	float                      getCurrentOutputDisplayRefreshRate      ();
	//@@Function
	/*
	* desc: returns the list of the available screen resolutions of the current output adapter (i.e. monitor).
	* param "vResolutions": pointer to your std::vector which will be filled.
	* return: false if successful, true otherwise.
	* remarks: should be called after calling the SApplication::init().
	*/
	bool                       getAvailableScreenResolutionsOfCurrentOutputDisplay(std::vector<SScreenResolution>* vResolutions);

private:

	//@@Variable
	/* pointer to the SApplication the video settings of which will be controlled. */
	SApplication* pApp;
};

