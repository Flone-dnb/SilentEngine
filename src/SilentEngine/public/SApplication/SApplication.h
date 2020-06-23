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
#include <mutex>
#include <unordered_map>

// DirectX
#include <wrl.h> // smart pointers
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include "SilentEngine/Private/d3dx12.h"
#include <DirectXColors.h>
#include <DirectXMath.h>

// Custom
#include "SilentEngine/Private/SGameTimer/SGameTimer.h"
#include "SilentEngine/Private/SGeometry/SGeometry.h"
#include "SilentEngine/Private/SUploadBuffer/SUploadBuffer.h"
#include "SilentEngine/Private/SFrameResource/SFrameResource.h"
#include "SilentEngine/Public/SVector/SVector.h"
#include "SilentEngine/Public/SMouseKey/SMouseKey.h"
#include "SilentEngine/Public/SKeyboardKey/SKeyboardKey.h"
#include "SilentEngine/Public/SVideoSettings/SVideoSettings.h"
#include "SilentEngine/Public/SProfiler/SProfiler.h"
#include "SilentEngine/Public/SLevel/SLevel.h"

// Other
#include <Windows.h>
#include <Windowsx.h>


// Libs
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")



#define ENGINE_D3D_FEATURE_LEVEL D3D_FEATURE_LEVEL_12_1


class SContainer;
class SComponent;
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------


//@@Class
/*
The class represents an application instance. It holds all main entities.
*/
class SApplication
{
public:

	//@@Function
	SApplication(HINSTANCE hInstance);
	SApplication() = delete;
	SApplication(const SApplication&) = delete;
	SApplication& operator= (const SApplication&) = delete;
	virtual ~SApplication();


	virtual LRESULT msgProc									   (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);



	// Main functions

		//@@Function
		/*
		* desc: disables the D3D12 debug layer. The D3D12 debug layer helps to debug D3D functions
		and shows in the Output tab a detailed description of the warnings and errors.
		D3D functions operate on the engine level so end users don't need to care about them.
		All debug build will have the D3D debug layer enabled, release build will not.
		As you might guess, the D3D debug layer comes with some overhead which can slow things down
		and even completely destroy your game's performance. You can disable the D3D debug
		layer for better performance in debug builds.
		*/
		void            initDisableD3DDebugLayer               ();
		//@@Function
		/*
		* desc: initializes all essential engine systems, creates the main window, starts Direct 3D rendering.
		Some functions (that contain "init" in their names like "...Init...()") can only be called before this function.
		* return: false if successful, true otherwise.
		* remarks: should be called before SApplication::run().
		*/
		bool            init								   ();
		//@@Function
		/*
		* desc: starts all essential engine systems, the main window will now process window messages, render new frames and etc.
		* return: WPARAM value of the last window message.
		* remarks: should be called after SApplication::run().
		*/
		int             run                                    ();
		//@@Function
		/*
		* desc: minimizes the main window.
		* return: false if successful, true otherwise.
		* remarks: should be called after SApplication::run().
		*/
		static bool     minimizeWindow                         ();
		//@@Function
		/*
		* desc: maximizes the main window.
		* return: false if successful, true otherwise.
		* remarks: should be called after SApplication::run().
		*/
		static bool     maximizeWindow                         ();
		//@@Function
		/*
		* desc: restores the main window.
		* return: false if successful, true otherwise.
		* remarks: should be called after SApplication::run().
		*/
		static bool     restoreWindow                          ();
		//@@Function
		/*
		* desc: hides the main window.
		* return: false if successful, true otherwise.
		* remarks: should be called after SApplication::run().
		*/
		static bool     hideWindow                             ();
		//@@Function
		/*
		* desc: shows the main window (if it was hidden).
		* return: false if successful, true otherwise.
		* remarks: should be called after SApplication::run().
		*/
		static bool     showWindow                             ();
		//@@Function
		/*
		* desc: closes the main window and application.
		* return: false if successful, true otherwise.
		* remarks: should be called after SApplication::run().
		*/
		static bool     close                                  ();

	
	// Level

		//@@Function
		/*
		* desc: returns the currently opened level, if one is opened.
		* return: valid pointer if the current level exists, nullptr otherwise.
		*/
		SLevel*         getCurrentLevel                        () const;


	// "Set" functions

		// Camera

		//@@Function
		/*
		* desc: sets the virtual camera's vertical FOV.
		* param "fFOVInDeg": vertical FOV value in degrees between 1 and 200.
		* return: false if successful, true otherwise.
		*/
		bool            setCameraFOV                           (float fFOVInDeg);
		//@@Function
		/*
		* desc: sets the fixed camera's zoom (radius in a spherical coordinate system).
		* param "fZoom": radius value for a spherical coordinate system, should be higher than 0.
		*/
		void            setFixedCameraZoom                     (float fZoom);
		//@@Function
		/*
		* desc: sets the fixed camera's rotation (phi and theta in a spherical coordinate system).
		* param "fPhi": vertical rotation.
		* param "fTheta": horizontal rotation.
		*/
		void            setFixedCameraRotation                 (float fPhi, float fTheta);
		//@@Function
		/*
		* desc: sets the fixed camera's rotation shift.
		* param "fHorizontalShift": horizontal rotation shift.
		* param "fVerticalShift": vertical rotation shift.
		*/
		void            setFixedCameraRotationShift            (float fHorizontalShift, float fVerticalShift);

		// Game

		//@@Function
		/*
		* desc: enables/disables onTick() function calls in SApplication which is called before a frame is drawn.
		* param "bTickCanBeCalled": true to enable, false otherwise.
		*/
		void            setCallTick                            (bool bTickCanBeCalled);

		// Window

		//@@Function
		/*
		* desc: shows/hides the mouse cursor.
		* param "bShow": true to show, false otherwise.
		*/
		void            setShowMouseCursor                     (bool bShow);
		//@@Function
		/*
		* desc: sets the cursor in the specified position in the main window.
		* param "vPos": position in window pixels in which the cursor should be set.
		* return: false if successful, true otherwise.
		* remarks: this function will return an error if it was called when the cursor is hidden, or if init() was not called before.
		*/
		bool            setCursorPos                           (const SVector& vPos);
		//@@Function
		/*
		* desc: adds frame stats (such as FPS and time to render a frame) in the window title.
		* param "bShow": true to show, false otherwise.
		*/
		void            setShowFrameStatsInWindowTitle         (bool bShow);
		//@@Function
		/*
		* desc: sets the text that will be displayed in the window title.
		* param "sTitleText": text to set in the window title.
		*/
		void            setWindowTitleText                     (const std::wstring& sTitleText);



	// "Get" functions

		// Camera

		//@@Function
		/*
		* desc: returns the vectors for the local axis of the fixed camera.
		* param "pvXAxis": this vector is directed in the local X-axis.
		* param "pvYAxis": this vector is directed in the local Y-axis.
		* param "pvZAxis": this vector is directed in the local Z-axis.
		*/
		void                   getFixedCameraLocalAxisVector   (SVector* pvXAxis, SVector* pvYAxis, SVector* pvZAxis) const;
		//@@Function
		/*
		* desc: returns the fixed camera's rotation.
		* param "fPhi": the pointer to your float value that will receive the vertical rotation value.
		* param "fTheta": the pointer to your float value that will receive the horizontal rotation value.
		*/
		void                   getFixedCameraRotation          (float* fPhi, float* fTheta) const;
		//@@Function
		/*
		* desc: returns the fixed camera's zoom.
		*/
		float                  getFixedCameraZoom              () const;

		// Other

		//@@Function
		/*
		* desc: returns the pointer to the SApplication instance if one was created.
		*/
		static SApplication*   getApp                          ();
		//@@Function
		/*
		* desc: used to retrieve the cursor position.
		* param "vPos": position in window pixels.
		* return: false if successful, true otherwise.
		* remarks: this function will return an error if it was called when the cursor is hidden, or if init() was not called before.
		*/
		bool                   getCursorPos                    (SVector* vPos);
		//@@Function
		/*
		* desc: used to retrieve the main window size.
		* param "vSize": window size in pixels.
		* return: false if successful, true otherwise.
		* remarks: should be called after SApplication::init().
		*/
		bool                   getWindowSize                   (SVector* vSize);
		//@@Function
		/*
		* desc: used to retrieve the main window handle.
		* return: main window handle.
		* remarks: should be called after SApplication::init().
		*/
		HWND                   getMainWindowHandle             () const;
		//@@Function
		/*
		* desc: used to retrieve the video settings.
		* return: a pointer to the video settings.
		*/
		SVideoSettings*        getVideoSettings                () const;
		//@@Function
		/*
		* desc: used to retrieve the profiler.
		* return: a pointer to the profiler.
		*/
		SProfiler*             getProfiler                     () const;


protected:

	// Game

		//@@Function
		/*
		* desc: an overridable function, called after SApplication::run().
		*/
		virtual void onRun() {};

		//@@Function
		/*
		* desc: an overridable function, called (if setCallTick() was set to true) every time before a frame is drawn.
		* param "fDeltaTime": the time that has passed since the last onTick() call.
		This value will be valid even if you just enabled onTick() calls.
		*/
		virtual void onTick(float fDeltaTime) {};


	// Input

		//@@Function
		/*
		* desc: an overridable function, called when the user presses a mouse key.
		* param "mouseKey": the mouse key that has been pressed.
		* param "iMouseXPos": mouse X position in pixels.
		* param "iMouseYPos": mouse Y position in pixels.
		*/
		virtual void onMouseDown(SMouseKey mouseKey, int iMouseXPos, int iMouseYPos) {};
		//@@Function
		/*
		* desc: an overridable function, called when the user unpresses a mouse key.
		* param "mouseKey": the mouse key that has been unpressed.
		* param "iMouseXPos": mouse X position in pixels.
		* param "iMouseYPos": mouse Y position in pixels.
		*/
		virtual void onMouseUp  (SMouseKey mouseKey, int iMouseXPos, int iMouseYPos) {};
		//@@Function
		/*
		* desc: an overridable function, called when the user moves the mouse.
		* param "iMouseXMove": the number in pixels by which the mouse was moved (by X-axis).
		* param "iMouseYMove": the number in pixels by which the mouse was moved (by Y-axis).
		*/
		virtual void onMouseMove(int iMouseXMove, int iMouseYMove) {};
		//@@Function
		/*
		* desc: an overridable function, called when the user moves the mouse wheel.
		* param "bUp": true if the mouse wheel was moved up (forward, away from the user), false otherwise.
		* param "iMouseXPos": mouse X position in pixels.
		* param "iMouseYPos": mouse Y position in pixels.
		*/
		virtual void onMouseWheelMove(bool bUp, int iMouseXPos, int iMouseYPos) {};


		//@@Function
		/*
		* desc: an overridable function, called when the user presses a keyboard key.
		* param "keyboardKey": a keyboard key that was pressed.
		*/
		virtual void onKeyboardButtonDown  (SKeyboardKey keyboardKey) {};
		//@@Function
		/*
		* desc: an overridable function, called when the user unpresses a keyboard key.
		* param "keyboardKey": a keyboard key that was unpressed.
		*/
		virtual void onKeyboardButtonUp    (SKeyboardKey keyboardKey) {};


	// Window


		//@@Function
		/*
		* desc: an overridable function, called before the application closes (when it's about to close),
		it will be called after the SAplication::close() call or if the user pressed the red exit button in window title bar.
		*/
		virtual void onCloseEvent   () {};
		//@@Function
		/*
		* desc: an overridable function, called when the application is being minimized,
		it will be called after the SAplication::minimize() call or if the user pressed the minimize button in window title bar.
		*/
		virtual void onMinimizeEvent() {};
		//@@Function
		/*
		* desc: an overridable function, called when the application is being maximized,
		it will be called after the SAplication::maximize() call or if the user pressed the maximize button in window title bar.
		*/
		virtual void onMaximizeEvent() {};
		//@@Function
		/*
		* desc: an overridable function, called after the SApplication::restoreWindow() call, or if the user presses the maximize button
		when the main window already maximized.
		*/
		virtual void onRestoreEvent() {};
		//@@Function
		/*
		* desc: an overridable function, called after the SApplication::hideWindow() call.
		*/
		virtual void onHideEvent    () {};
		//@@Function
		/*
		* desc: an overridable function, called after the SApplication::showWindow() call.
		*/
		virtual void onShowEvent    () {};

private:

	// Initialization

		//@@Function
		/*
		* desc: creates main window (handle hMainWindow is valid after this function).
		* return: false if successful, true otherwise.
		*/
		bool createMainWindow                ();
		//@@Function
		/*
		* desc: creates main D3D objects such as: factory, device, fence, command objects.
		* return: false if successful, true otherwise.
		*/
		bool initD3DFirstStage               ();
		//@@Function
		/*
		* desc: creates swap chain, descriptor heaps.
		* return: false if successful, true otherwise.
		* remarks: here we CreateSwapChainForHwnd and disable alt+f4, this is why we need two stages of initD3D: 1st stage - create main objects,
		then create main window, 2nd stage - create swap chain for window.
		*/
		bool initD3DSecondStage              ();
		//@@Function
		/*
		* desc: picks the first display adapter or preferred display adapter if 'sPreferredDisplayAdapter' is filled.
		* param "pAdapter": the pointer to the adapter that will be filled.
		* return: false if successful, true otherwise.
		*/
		bool getFirstSupportedDisplayAdapter (IDXGIAdapter3*& pAdapter);
		//@@Function
		/*
		* desc: picks the first output adapter or preferred output adapter if 'sPreferredOutputAdapter' is filled.
		* param "pOutput": the pointer to the output that will be filled.
		* return: false if successful, true otherwise.
		*/
		bool getFirstOutputDisplay           (IDXGIOutput*& pOutput);
		//@@Function
		/*
		* desc: checks if the current MSAA settings and back buffer format are supported on this device.
		* return: false if successful, true otherwise.
		*/
		bool checkMSAASupport                ();
		//@@Function
		/*
		* desc: creates command queue, command allocator and command list.
		* return: false if successful, true otherwise.
		*/
		bool createCommandObjects            ();
		//@@Function
		/*
		* desc: creates swap chain.
		* return: false if successful, true otherwise.
		*/
		bool createSwapChain                 ();
		//@@Function
		/*
		* desc: retrieves the display params such as resolution, scaling and refresh rate.
		* param "bApplyResolution": if true, then resolution will be saved as window resolution.
		* return: false if successful, true otherwise.
		*/
		bool getScreenParams                 (bool bApplyResolution);
		//@@Function
		/*
		* desc: creates RTV and DSV descriptor heaps.
		* return: false if successful, true otherwise.
		*/
		bool createRTVAndDSVDescriptorHeaps  ();
		//@@Function
		/*
		* desc: creates CBV descriptor heap.
		* return: false if successful, true otherwise.
		*/
		bool createCBVHeap                   ();
		//@@Function
		/*
		* desc: creates CBVs.
		*/
		void createConstantBufferViews       ();
		//@@Function
		/*
		* desc: creates frame resources.
		* remarks: each frame resource has everything to render a frame.
		So we can render N frames using N frame resources continuously, without waiting
		for the CPU or GPU to catch up. Thanks to this we don't flushCommandQueue() after every frame.
		*/
		void createFrameResources            ();
		//@@Function
		/*
		* desc: creates root signature.
		* return: false if successful, true otherwise.
		*/
		bool createRootSignature             ();
		//@@Function
		/*
		* desc: compiles shaders and fills input layout.
		* return: false if successful, true otherwise.
		*/
		bool createShadersAndInputLayout     ();
		//@@Function
		/*
		* desc: creates PSO.
		* return: false if successful, true otherwise.
		*/
		bool createPSO                       ();
		//@@Function
		/*
		* desc: resets command list so it can be used.
		* return: false if successful, true otherwise.
		*/
		bool resetCommandList                ();
		//@@Function
		/*
		* desc: executes the commands from list.
		* return: false if successful, true otherwise.
		*/
		bool executeCommandList              ();


	// Window

		//@@Function
		/*
		* desc: called when the window size was changed. Resizes the swap chain and buffers.
		* return: false if successful, true otherwise.
		*/
		bool onResize                        ();


	// Game

		//@@Function
		/*
		* desc: updates the camera and buffers.
		*/
		void update                          ();
		//@@Function
		/*
		* desc: updates the camera.
		*/
		void updateCamera                    ();
		//@@Function
		/*
		* desc: updates the constant buffers.
		*/
		void updateObjectCBs                 ();
		//@@Function
		/*
		* desc: updates the objects' constant buffers.
		*/
		void updateComponentAndChilds        (SComponent* pComponent, SUploadBuffer<SObjectConstants>* pCurrentCB);
		//@@Function
		/*
		* desc: updates the main pass constant buffer.
		*/
		void updateMainPassCB                ();
		//@@Function
		/*
		* desc: draws the frame.
		*/
		void draw                            ();
		//@@Function
		/*
		* desc: draws the visible renderable containers.
		*/
		void drawVisibleRenderableContainers ();
		//@@Function
		/*
		* desc: draws visible renderable components.
		* param "pComponent": component to draw.
		*/
		void drawComponentAndChilds          (SComponent* pComponent);

		//@@Function
		/*
		* desc: used to set the FPS limit (FPS cap).
		* param "fFPSLimit": the value which the generated framerate should not exceed. 0 to disable FPS limit.
		* remarks: set fFPSLimit to 0 to disable FPS limit.
		*/
		void setFPSLimit                     (float fFPSLimit);

		//@@Function
		/*
		* desc: returns the time in seconds elapsed since the SApplication::run() was called.
		* param "fTimeInSec": pointer to your float value which will be used to set the time value.
		* return: false if successful, true otherwise.
		* remarks: should be called after calling the SApplication::run().
		*/
		bool getTimeElapsedFromStart         (float* fTimeInSec) const;
		//@@Function
		/*
		* desc: returns the current number of frames per second generated by the application.
		* param "iFPS": pointer to your int value which will be used to set the FPS value.
		* return: false if successful, true otherwise.
		* remarks: should be called after calling the SApplication::run().
		*/
		bool getFPS                          (int* iFPS) const;
		//@@Function
		/*
		* desc: returns the time it was taken to render the last frame.
		* param "fTimeInMS": pointer to your float value which will be used to set the time value.
		* return: false if successful, true otherwise.
		* remarks: should be called after calling the SApplication::run().
		*/
		bool getTimeToRenderFrame            (float* fTimeInMS) const;
		//@@Function
		/*
		* desc: returns the number of the draw calls it was made to render the last frame.
		* param "iDrawCallCount": pointer to your unsigned long long value which will be used to set the draw call count value.
		* return: false if successful, true otherwise.
		* remarks: should be called on tick(), otherwise it will have an incorrect number. The function is also should be
		called only after calling the SApplication::run().
		*/
		bool getLastFrameDrawCallCount       (unsigned long long* iDrawCallCount) const;


	// Multisampling

		//@@Function
		/*
		* desc: used to enable/disable the MSAA - anti-aliasing technique.
		* param "bEnable": true to enable MSAA, false otherwise.
		* remarks: MSAA is enabled by default. Enabling MSAA might cause a slight decrease in performance.
		*/
		void setMSAAEnabled                  (bool bEnable);
		//@@Function
		/*
		* desc: used to set the sample count (i.e. quality) of the MSAA.
		* param "sampleCount": sample count (i.e. quality).
		* remarks: the default sample count is 4. The higher the number of samples the lower the performance might get.
		*/
		bool setMSAASampleCount              (MSAASampleCount eSampleCount);
		
		//@@Function
		/*
		* desc: used to determine if the MSAA is enabled.
		* return: true if MSAA is enabled, false otherwise.
		* remarks: MSAA is enabled by default.
		*/
		bool isMSAAEnabled                   () const;
		//@@Function
		/*
		* desc: used to retrieve the number of samples (i.e. quality) used by the MSAA.
		* return: number of samples used by the MSAA.
		* remarks: the default sample count is 4.
		*/
		MSAASampleCount getMSAASampleCount   () const;

	// Screen

		//@@Function
		/*
		* desc: used to switch between fullscreen and windowed modes.
		* param "bFullscreen": true for fullscreen mode, false for windowed.
		* return: false if successful, true otherwise.
		*/
		bool setFullscreenWithCurrentResolution   (bool bFullScreen);
		//@@Function
		/*
		* desc: used to set the screen resolution.
		* param "screenResolution": screen resolution to be set.
		* return: false if successful, true otherwise.
		*/
		bool setScreenResolution                  (SScreenResolution screenResolution);
		//@@Function
		/*
		* desc: used to retrieve current screen resolution.
		* param "pScreenResolution": pointer to the struct in which the screen resolution width and height values will be set.
		* return: false if successful, true otherwise.
		*/
		bool getCurrentScreenResolution           (SScreenResolution* pScreenResolution) const;
		//@@Function
		/*
		* desc: used to determine if the current screen mode is fullscreen.
		* return: false if successful, true otherwise.
		*/
		bool isFullscreen                         () const;

	// Graphics

		//@@Function
		/*
		* desc: used to set the "background" color of the world.
		* param "vColor": the vector that contains the color in XYZ as RGB.
		*/
		void setBackBufferFillColor               (SVector vColor);
		//@@Function
		/*
		* desc: used to enable/disable wireframe display mode.
		* param "bEnable": true to enable, false to disable.
		*/
		void setEnableWireframeMode               (bool bEnable);
		//@@Function
		/*
		* desc: used to enable/disable backface culling technique.
		* param "bEnable": true to enable, false to disable.
		*/
		void setEnableBackfaceCulling             (bool bEnable);

		//@@Function
		/*
		* desc: used to retrieve the "background" color of the world.
		* return: the vector that contains the color in XYZ as RGB.
		*/
		SVector getBackBufferFillColor            () const;
		//@@Function
		/*
		* desc: used to determine if the wireframe display mode is enabled.
		* return: true if enabled, false otherwise.
		*/
		bool isWireframeModeEnabled               () const;
		//@@Function
		/*
		* desc: used to determine if the backface culling technique is enabled.
		* return: true if enabled, false otherwise.
		*/
		bool isBackfaceCullingEnabled             () const;
		//@@Function
		/*
		* desc: returns the number of triangles in the world (current level).
		* return: 0 if there is no current level loaded or no objects in the scene, otherwise the triangle count in the world.
		*/
		unsigned long long getTriangleCountInWorld ();


	// Display adapters.

		//@@Function
		/*
		* desc: returns the list of the display adapters (i.e. "video cards") on your PC that engine supports.
		* remarks: should be called after calling the SApplication::init().
		*/
		std::vector<std::wstring>  getSupportedDisplayAdapters () const;
		//@@Function
		/*
		* desc: returns the current display adapter (i.e. "video card") that is being used.
		* remarks: should be called after calling the SApplication::init().
		*/
		std::wstring               getCurrentDisplayAdapter    () const;
		//@@Function
		/*
		* desc: used to retrieve the size of the VRAM (video memory) of the current display adapter (i.e. "video card").
		* return: false if successful, true otherwise.
		* remarks: should be called after calling the SApplication::init().
		*/
		bool                       getVideoMemorySizeInBytesOfCurrentDisplayAdapter(SIZE_T* pSizeInBytes) const;
		//@@Function
		/*
		* desc: returns currently used memory space (i.e. how much of the VRAM is used) of the display adapter (i.e. "video card").
		* param "pSizeInBytes": pointer to your unsigned long long value which will be used to set the memory value.
		* return: false if successful, true otherwise.
		* remarks: should be called after calling the SApplication::init().
		*/
		bool                       getVideoMemoryUsageInBytesOfCurrentDisplayAdapter(UINT64* pSizeInBytes) const;


	// Output displays.

		//@@Function
		/*
		* desc: returns the list of the output adapters (i.e. monitors) on your PC that support current display adapter (i.e. "video card").
		* remarks: should be called after calling the SApplication::init().
		*/
		std::vector<std::wstring>  getOutputDisplaysOfCurrentDisplayAdapter() const;
		//@@Function
		/*
		* desc: returns the name of the current output adapter (i.e. monitor).
		* remarks: should be called after calling the SApplication::init().
		*/
		std::wstring               getCurrentOutputDisplay     () const;
		//@@Function
		/*
		* desc: returns the refresh rate value of the current output adapter (i.e. monitor).
		* remarks: should be called after calling the SApplication::init().
		*/
		float                      getCurrentOutputDisplayRefreshRate      () const;
		//@@Function
		/*
		* desc: returns the list of the available screen resolutions of the current output adapter (i.e. monitor).
		* param "vResolutions": pointer to your std::vector which will be filled.
		* return: false if successful, true otherwise.
		* remarks: should be called after calling the SApplication::init().
		*/
		bool                       getAvailableScreenResolutionsOfCurrentOutputDisplay(std::vector<SScreenResolution>* vResolutions) const;


	// Init
		//@@Function
		/*
		* desc: used to set the preferred display adapter (i.e. "video card" on your PC). The list of all supported display
		adapters may be retrieved through SVideoSettings::getSupportedDisplayAdapters() function.
		* param "sPreferredDisplayAdapter": the name of the preferred display adapter.
		* return: false if successful, true otherwise.
		* remarks: should be called before calling the SApplication::init().
		*/
		bool setInitPreferredDisplayAdapter		   (std::wstring sPreferredDisplayAdapter);
		//@@Function
		/*
		* desc: used to set the preferred output adapter (i.e. monitor on your PC). The list of all supported output adapters may be
		retrieved through SVideoSettings::getOutputDisplaysOfCurrentDisplayAdapter() function.
		* param "sPreferredOutputAdapter": the name of the preferred output adapter.
		* return: false if successful, true otherwise.
		* remarks: should be called before calling the SApplication::init().
		*/
		bool setInitPreferredOutputAdapter		   (std::wstring sPreferredOutputAdapter);
		//@@Function
		/*
		* desc: determines if the application should run in fullscreen mode.
		* param "bFullscreen": true for fullscreen, false for windowed mode.
		* return: false if successful, true otherwise.
		* remarks: should be called before calling the SApplication::init().
		*/
		bool setInitFullscreen					   (bool bFullscreen);
		//@@Function
		/*
		* desc: used to enable VSync.
		* param "bEnable": true to enable VSync, false otherwise.
		* return: false if successful, true otherwise.
		* remarks: should be called before calling the SApplication::init().
		*/
		bool setInitEnableVSync                    (bool bEnable);


	// Camera

		//@@Function
		/*
		* desc: used to set the camera's near clip plane.
		* param "fNearClipPlaneValue": near clip plane value.
		* return: false if successful, true otherwise.
		* remarks: should be called after calling the SApplication::init().
		*/
		bool setNearClipPlane                     (float fNearClipPlaneValue);
		//@@Function
		/*
		* desc: used to set the camera's far clip plane.
		* param "fFarClipPlaneValue": far clip plane value.
		* return: false if successful, true otherwise.
		* remarks: should be called after calling the SApplication::init().
		*/
		bool setFarClipPlane                      (float fFarClipPlaneValue);

		//@@Function
		/*
		* desc: used to retrieve the camera's near clip plane.
		*/
		float getNearClipPlaneValue               () const;
		//@@Function
		/*
		* desc: used to retrieve the camera's far clip plane.
		*/
		float getFarClipPlaneValue                () const;


	// Level

		//@@Function
		/*
		* desc: used to spawn container in level.
		* param "pContainer": the pointer to a container that you want to spawn.
		* remarks: this function is thread-safe (you can call it from any thread).
		*/
		bool spawnContainerInLevel                (SContainer* pContainer);
		//@@Function
		/*
		* desc: used to despawn container in level.
		* param "pContainer": the pointer to a container that you want to despawn.
		* remarks: this function is thread-safe (you can call it from any thread).
		*/
		void despawnContainerFromLevel            (SContainer* pContainer);


	// Other

		//@@Function
		/*
		* desc: used to retrieve the current screen aspect ratio.
		*/
		float getScreenAspectRatio            () const;

		//@@Function
		/*
		* desc: sets the fence value and waits until all operations prior to this fence point are done.
		* return: false if successful, true otherwise.
		*/
		bool  flushCommandQueue               ();
		//@@Function
		/*
		* desc: calculates fTimeToRenderFrame and iFPS values.
		*/
		void  calculateFrameStats             ();
		//@@Function
		/*
		* desc: rounds up the given value to the next multiple value.
		* return: rounded value.
		*/
		size_t roundUp                        (size_t iNum, size_t iMultiple);


	// Buffers and views.

		//@@Function
		/*
		* desc: used to retrieve the current back buffer resource from swap chain or from MSAA buffer.
		* param "bNonMSAAResource": if true, then the current buffer will be retrieved from the swap chain,
		if false and MSAA is enabled then will return the MSAA render target resources.
		*/
		ID3D12Resource*             getCurrentBackBufferResource   (bool bNonMSAAResource = false) const;
		//@@Function
		/*
		* desc: used to retrieve the current back buffer view handle for swap chain buffers (if MSAA is disabled) or MSAA buffer (if MSAA is enabled).
		*/
		D3D12_CPU_DESCRIPTOR_HANDLE getCurrentBackBufferViewHandle () const;
		//@@Function
		/*
		* desc: used to retrieve the depth stencil buffer view handle.
		*/
		D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilViewHandle      () const;



	// -----------------------------------------------------------------

	friend class SMeshComponent;
	friend class SRuntimeMeshComponent;
	friend class SLevel;


	static SApplication* pApp;

	
	// Main DX objects.
	Microsoft::WRL::ComPtr<IDXGIFactory4>   pFactory;
	Microsoft::WRL::ComPtr<ID3D12Device>    pDevice;
	Microsoft::WRL::ComPtr<IDXGIAdapter3>   pAdapter;
	Microsoft::WRL::ComPtr<IDXGIOutput>     pOutput;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> pSwapChain;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pWireframePSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pNoBackfaceCullingPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pNoBackfaceCullingWireframePSO;


	// Command objects.
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>        pCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    pCommandListAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList;

	
	// Fence.
	Microsoft::WRL::ComPtr<ID3D12Fence>    pFence;
	UINT64 iCurrentFence                    = 0;


	// Swap chain.
	static const UINT iSwapChainBufferCount = 2;
	int iCurrentBackBuffer                  = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> pSwapChainBuffer[iSwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> pDepthStencilBuffer;


	// Descriptor heaps and descriptor sizes
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pRTVHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pDSVHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pCBVHeap;
	UINT iRTVDescriptorSize                 = 0;
	UINT iDSVDescriptorSize                 = 0;
	UINT iCBVSRVUAVDescriptorSize           = 0;


	// Frame resources.
	static const int iFrameResourcesCount   = SFRAME_RES_COUNT;
	std::vector<std::unique_ptr<SFrameResource>> vFrameResources;
	SFrameResource* pCurrentFrameResource   = nullptr;
	int iCurrentFrameResourceIndex          = 0;

	std::vector<D3D12_INPUT_ELEMENT_DESC>        vInputLayout;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>  pRootSignature;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;


	// CB constants.
	SRenderPassConstants mainRenderPassCB;
	size_t iRenderPassCBVOffset = 0;
	size_t iActualObjectCBCount = 0;
	

	// Camera.
	DirectX::XMFLOAT3   vCameraPos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3   vCameraTargetPos = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT4X4 vView   = SMath::getIdentityMatrix4x4();
	DirectX::XMFLOAT4X4 vProj   = SMath::getIdentityMatrix4x4();


	// Buffer formats.
	DXGI_FORMAT BackBufferFormat            = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT DepthStencilFormat          = DXGI_FORMAT_D24_UNORM_S8_UINT;


	// Video settings.
	friend class SVideoSettings;
	SVideoSettings* pVideoSettings;


	// Profiler.
	friend class SProfiler;
	SProfiler*     pProfiler;

	
	// Level
	SLevel*        pCurrentLevel = nullptr;


	// MSAA.
	Microsoft::WRL::ComPtr<ID3D12Resource>  pMSAARenderTarget;
	bool           MSAA_Enabled             = true;
	UINT           MSAA_SampleCount         = 4;
	UINT           MSAA_Quality             = 0;


	// Screen.
	bool           bFullscreen              = false;
	int            iMainWindowWidth         = 800;
	int            iMainWindowHeight        = 600;
	float          fFOVInDeg                = 90;
	UINT           iRefreshRateNumerator    = 60;
	UINT           iRefreshRateDenominator  = 1;
	DXGI_MODE_SCANLINE_ORDER iScanlineOrder = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	DXGI_MODE_SCALING        iScaling       = DXGI_MODE_SCALING_UNSPECIFIED;


	// Viewport
	D3D12_VIEWPORT ScreenViewport; 
	D3D12_RECT     ScissorRect;
	float          fNearClipPlaneValue      = 1.0f;
	float          fFarClipPlaneValue       = 1000.0f;
	float          fTheta                   = 1.5f * DirectX::XM_PI;
	float          fPhi                     = DirectX::XM_PIDIV4;
	float          fRadius                  = 6.0f;

	
	// Windows stuff.
	HINSTANCE      hApplicationInstance     = nullptr;
	HWND           hMainWindow              = nullptr;
	SMouseKey      pressedMouseKey = SMB_NONE;
	bool           bMouseCursorShown        = true;
	bool           bRawInputReady           = false;
	float          fWindowCenterX           = 0;
	float          fWindowCenterY           = 0;


	// VSunc
	bool           bVSyncEnabled            = false;


	// Graphics
	float          backBufferFillColor[4]   = {0.0f, 0.0f, 0.0f, 1.0f};
	bool           bUseBackFaceCulling      = true;
	bool           bUseFillModeWireframe    = false;


	// Performance
	unsigned long long iLastFrameDrawCallCount  = 0;
	int            iFPS                     = 0;
	float          fTimeToRenderFrame       = 0.0f;
	float          fFPSLimit                = 0.0f;
	double         fDelayBetweenFramesInMS  = 0.0f;
	bool           bShowFrameStatsInTitle   = false;


	SGameTimer     gameTimer;

	
	std::mutex     mtxSpawnDespawn;
	std::mutex     mtxDraw;


	std::wstring   sMainWindowTitle         = L"Silent Application";
	std::wstring   sPreferredDisplayAdapter = L"";
	std::wstring   sPreferredOutputAdapter  = L"";

	bool           bUsingWARPAdapter        = false;

	bool           bWindowMaximized         = false;
	bool           bWindowMinimized         = false;
	bool           bResizingMoving          = false;

	bool           bCustomWindowSize        = false;

	bool           bInitCalled              = false;
	bool           bRunCalled               = false;

	bool           bCallTick                = false;

	bool           bD3DDebugLayerEnabled    = true;
};

