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
#include <future>

// DirectX
#include <wrl.h> // smart pointers
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#pragma warning(push, 0) // disable warnings from this header
#include "SilentEngine/Private/d3dx12.h"
#pragma warning(pop)
#include <DirectXColors.h>
#include <DirectXMath.h>

// DXC
#include "dxc/dxcapi.h"
#include <atlbase.h> // Common COM helpers.

// DirectXTK
#include "GraphicsMemory.h"

// Custom
#include "SilentEngine/Private/SGameTimer/SGameTimer.h"
#include "SilentEngine/Private/SRenderItem/SRenderItem.h"
#include "SilentEngine/Private/SUploadBuffer/SUploadBuffer.h"
#include "SilentEngine/Private/SFrameResource/SFrameResource.h"
#include "SilentEngine/Public/SVector/SVector.h"
#include "SilentEngine/Public/SMouseKey/SMouseKey.h"
#include "SilentEngine/Public/SKeyboardKey/SKeyboardKey.h"
#include "SilentEngine/Public/SVideoSettings/SVideoSettings.h"
#include "SilentEngine/Public/SProfiler/SProfiler.h"
#include "SilentEngine/Public/SLevel/SLevel.h"
#include "SilentEngine/Public/SMaterial/SMaterial.h"
#include "SilentEngine/Private/SBlurEffect/SBlurEffect.h"
#include "SilentEngine/Public/SComputeShader/SComputeShader.h"
#include "SilentEngine/Public/SCamera/SCamera.h"
#include "SilentEngine/Private/SCustomShaderResources/SCustomShaderResources.h"
#include "SilentEngine/Private/AudioEngine/SAudioEngine/SAudioEngine.h"
#include "SilentEngine/Private/GUI/SGUIObject/SGUIObject.h"

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
class SShader;
class SShaderObjects;
class SComputeShader;
class SCameraComponent;
class SGUIObject;
class SGUILayer;
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

struct SCustomShaderMaterials
{
	std::vector<std::string> vCustomMaterialNames;
	bool bWillUseTextures = false;
};

struct SCustomShaderProperties
{
	SCustomShaderMaterials customMaterials;
	bool bWillUseInstancing = false;
};


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
		D3D functions operate on the engine level so end users don't need to care about them,
		but if you disable the D3D debug layer and stumble upon an engine error, a warning or not a critical engine error, you might
		not even know about an error in the engine and so in your game.
		All debug build will have the D3D debug layer enabled, release build will not.
		As you might guess, the D3D debug layer comes with some overhead which can slow things down
		and even completely destroy your game's performance. You can disable the D3D debug
		layer for better performance in debug builds.
		* remarks: should be called before SApplication::init().
		*/
		void            initDisableD3DDebugLayer               ();
		//@@Function
		/*
		* desc: forces the shaders to be compiled in release mode (better performance, no debug info)
		even if the current build configuration is debug mode. All shaders will be always compiled in
		release mode if the current build configuration is release mode.
		* remarks: should be called before SApplication::init().
		*/
		void            initCompileShadersInRelease            ();
		//@@Function
		/*
		* desc: initializes all essential engine systems, creates the main window, starts Direct 3D rendering.
		Some functions (that contain "init" in their names like "...Init...()") can only be called before this function.
		* return: false if successful, true otherwise.
		* remarks: should be called before SApplication::run().
		*/
		bool            init								   (const std::wstring& sMainWindowClassName = L"MainWindow");
		//@@Function
		/*
		* desc: starts all essential engine systems, the main window will now process window messages, render new frames and etc.
		* return: WPARAM value of the last window message.
		* remarks: should be called after SApplication::init().
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


	// Visual settings.

		//@@Function
		/*
		* desc: used to set the global visual settings.
		*/
		void             setGlobalVisualSettings               (const SGlobalVisualSettings& settings);
		//@@Function
		/*
		* desc: returns the global visual settings.
		*/
		SGlobalVisualSettings getGlobalVisualSettings          ();

	
	// Materials

		//@@Function
		/*
		* desc: registers a new material in the application so it can be used in the component, such as SMeshComponent.
		* param "sMaterialName": unique name of the new material.
		* param "bErrorOccurred": will be true if an error occurred and the returned pointer is nullptr.
		* return: valid pointer if successful, nullptr otherwise.
		* remarks: any material should be registered first and only then used.
		You can reuse already registered materials on any number of components you want.
		It's recommended to use this function in loading moments of your application (ex. loading screen)
		as this function may drop the framerate a little.
		*/
		SMaterial*       registerMaterial				       (const std::string& sMaterialName, bool& bErrorOccurred);
		//@@Function
		/*
		* desc: returns a registered material.
		* return: valid pointer if successful, nullptr otherwise.
		*/
		SMaterial*      getRegisteredMaterial                  (const std::string& sMaterialName);
		//@@Function
		/*
		* desc: returns all registered materials.
		* return: valid pointer if successful, nullptr otherwise.
		*/
		std::vector<SMaterial*>* getRegisteredMaterials         ();
		//@@Function
		/*
		* desc: unregisters a material with the given name, make sure that no object is using this material.
		* return: false if successful, true otherwise.
		* remarks: if any spawned object uses this material then the material will be unbinded from such objects,
		you can manually unbind the material from objects that use it
		by using SComponent::unbindMaterial().
		This function will not unload the textures that this material is using.
		If you want to clear all textures and material, the right way is to
		call SContainer::unbindMaterialsFromAllComponents() (or unbindMaterial() on all components and child components), and then use
		unloadTexture() and unregisterMaterial() (no matter in what order).
		You cannot unregister the Default Engine Material.
		It's recommended to use this function in loading moments of your application (ex. loading screen)
		as this function may drop the framerate a little.
		All registered material will be unregistered in the SApplication's destructor function.
		*/
		bool            unregisterMaterial                     (const std::string& sMaterialName);


	// GUI

		//@@Function
		/*
		* desc: registers a new GUI object that can be used in GUI.
		* param "pObject": your pointer to the GUI object that you've created and want to register.
		* param "bWillBeUsedInLayout": specify 'true' if this object will be a child of some layout, this bypasses the setSizeToKeep() check,
		so you don't have to specify it because the layout will manage the size of this object.
		* remarks: registered object will be invisible, call setVisible(true) when you want to show this GUI object on screen.
		Note that you will need to unregister this resource (via SApplication::unregisterGUIObject), it won't happen automatically,
		but all registered GUI resources will be unregistered in the SApplication's destructor function.
		It's recommended to use this function in loading moments of your application (ex. loading screen)
		as this function may drop the framerate a little.
		*/
		void            registerGUIObject                         (SGUIObject* pGUIObject, bool bWillBeUsedInLayout);
		//@@Function
		/*
		* desc: returns all loaded GUI objects.
		* return: valid pointer if successful, nullptr otherwise.
		* remarks: in debug mode, some GUI objects will be created by the SProfiler for the SProfiler::showOnScreen() function,
		an attempt to delete such objects will result in an error (unregisterGUIObject() will return 'true'). You can
		check if the GUI object was created by the SProfiler using the SGUIObject::isProfilerObject() function.
		*/
		std::vector<SGUILayer>* getLoadedGUIObjects              ();
		//@@Function
		/*
		* desc: unregisters the given GUI object and deletes it.
		* return: false if successful, true if the specified object was not registered earlier (via SApplication::registerGUIObject).
		* remarks: any pointers to this object will be invalid after this call (the object will be deleted in this function).
		In debug mode, some GUI objects (called system objects) will be created by the SProfiler or other engine class (for the SProfiler::showOnScreen() function for example),
		an attempt to delete such objects will result in an error (unregisterGUIObject() will return 'true'). You can
		check if the GUI object is a system object by using the SGUIObject::isSystemObject() function.
		It's recommended to use this function in loading moments of your application (ex. loading screen)
		as this function may drop the framerate a little.
		*/
		bool           unregisterGUIObject                       (SGUIObject* pGUIObject);


	// Textures

		//@@Function
		/*
		* desc: loads the given texture from the disk to the GPU memory so it can be used in the materials.
		* param "sTextureName": unique name of the texture, cannot be empty.
		* param "sPathToTexture": path to the texture file (only .dds texture format).
		* param "bErrorOccurred": will be true if an error occurred and the returned handle is invalid, false otherwise.
		* remarks: note that you will need to unload the loaded textures (via SApplication::unloadTextureFromGPU), it won't happen automatically,
		but all loaded textures will be unloaded in the SApplication's destructor function.
		You can reuse already loaded textures on any number of materials you want.
		This struct (STextureHandle) is just a handle to the actual texture so you can make copies of it.
		It's recommended to use this function in loading moments of your application (ex. loading screen)
		as this function may drop the framerate a little.
		*/
		STextureHandle  loadTextureFromDiskToGPU               (std::string sTextureName, std::wstring sPathToTextureFile, bool& bErrorOccurred);
		//@@Function
		/*
		* desc: returns a loaded texture.
		* param "sTextureName": the name of the loaded texture.
		* param "bNotFound": will be true if there is no loaded texture with this name.
		* return: valid handle if successful.
		*/
		STextureHandle  getLoadedTexture                       (const std::string& sTextureName, bool& bNotFound);
		//@@Function
		/*
		* desc: returns all loaded in the GPU memory textures.
		*/
		std::vector<STextureHandle> getLoadedTextures          ();
		//@@Function
		/*
		* desc: unloads the given texture from the GPU memory.
		* return: false if successful, true otherwise.
		* remarks: if any spawned object uses the material with this texture then the material will be unbinded from such objects,
		you can manually unbind the material/texture from objects that use it
		by using SMaterial::unbindTexture() or SComponent::unbindMaterial().
		If you want to clear all textures and material, the right way is to
		call SContainer::unbindMaterialsFromAllComponents() (or unbindMaterial() on all components and child components), and then use
		unloadTexture() and unregisterMaterial() (no matter in what order). Make sure that this texture is not used by a custom shader
		(unloadCompiledShaderFromGPU() first).
		It's recommended to use this function in loading moments of your application (ex. loading screen)
		as this function may drop the framerate a little.
		*/
		bool            unloadTextureFromGPU                   (STextureHandle& textureHandle);


	// Shaders

		//@@Function
		/*
		* desc: used to compile and prepare the custom shader (VS and PS, see basic.hlsl for reference).
		* param "customProps": (optional) pass 'SCustomShaderProperties()' for defaults.
		* param "customProps.vCustomMaterialNames": (optional) pass the names of the new custom materials to create a material bundle.
		* param "customProps.bWillUseTextures": (optional) use 'true' if ALL created materials will use textures.
		You can either have textures for all materials or don't have textures for all materials.
		You can't have some material with textures and some without textures.
		Textures should be loaded sequentially (don't have any other textures in between).
		* param "pOutCreatedMaterials": (optional) pass an empty pointer to get a reference to the created resources (if successful).
		* return: valid pointer if the shader was found and compiled, nullptr otherwise.
		* remarks: you will need to call unloadCompiledShaderFromGPU() later when you don't need this shader anymore -
		this will not happen automatically, but all shaders will be unloaded in the SApplication destructor function.
		It's recommended to use this function in loading moments of your application (ex. loading screen)
		as this function may drop the framerate a little.
		*/
		SShader*        compileCustomShader                   (const std::wstring& sPathToShaderFile,
			const SCustomShaderProperties& customProps, SCustomShaderResources** pOutCustomResources = nullptr);

		//@@Function
		/*
		* desc: used to retrieve all compiled custom shaders.
		*/
		std::vector<SShader*>* getCompiledCustomShaders       ();

		//@@Function
		/*
		* desc: used to unload and unprepare the custom shader from the GPU.
		* return: false if successful, true otherwise.
		* remarks: if any spawned object is using this shader, it will switch to the default engine shader. You can do it manually
		by calling the SComponent::setUseDefaultShader() function when the object is not spawned.
		*/
		bool            unloadCompiledShaderFromGPU           (SShader* pShader);

		//@@Function
		/*
		* desc: used to register custom compute shader, use returned shader functions to configure it.
		* return: valid pointer if the shader name is unique, nullptr otherwise.
		* remarks: You will need to call unregisterCustomComputeShader() later when you don't need this shader anymore -
		this will not happen automatically, but all shaders will be unloaded in the SApplication destructor function.
		It's recommended to use this function in loading moments of your application (ex. loading screen)
		as this function may drop the framerate a little.
		*/
		SComputeShader* registerCustomComputeShader           (const std::string& sUniqueShaderName);

		//@@Function
		/*
		* desc: used to retrieve all custom registered compute shaders.
		*/
		std::vector<SComputeShader*>* getRegisteredComputeShaders();

		//@@Function
		/*
		* desc: used to unregister custom compute shader. The passed pointer will be deleted.
		* remarks: It's recommended to use this function in loading moments of your application (ex. loading screen)
		as this function may drop the framerate a little.
		*/
		void            unregisterCustomComputeShader(SComputeShader* pComputeShader);


	// Level

		//@@Function
		/*
		* desc: returns the currently opened level, if one is opened.
		* return: valid pointer if the current level exists, nullptr otherwise.
		*/
		SLevel*         getCurrentLevel                        () const;


	// "Set" functions

		//@@Function
		/*
		* desc: "If you hold down a key long enough to start the keyboard's repeat feature, the system sends multiple key-down messages,
		followed by a single key-up message."
		* remarks: disabled by default.
		*/
		void setDisableKeyboardRepeat(bool bDisable);

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
		//@@Function
		/*
		* desc: used to set the number of physics ticks per second (i.e. the number of times the onPhysicsTick() function will be called per second)
		(default: 60).
		* return: false if successful, true otherwise.
		* remarks: should be called before calling the SApplication::init().
		*/
		bool            setInitPhysicsTicksPerSecond(int iTicksPerSecond = 60);



	// "Get" functions

		// Camera

		//@@Function
		/*
		* desc: returns the camera (eyes).
		*/
		SCamera*               getCamera                       ();

		// Other

		//@@Function
		/*
		* desc: returns the audio engine that can be used to control master volume, listener position and
		create sound mixes.
		* remarks: listener position is controlled automatically and set to camera's position on every frame, same with emitters.
		*/
		SAudioEngine*          getAudioEngine                  ();

		//@@Function
		/*
		* desc: returns the pointer to the SApplication instance if one was created.
		*/
		static SApplication*   getApp                          ();
		//@@Function
		/*
		* desc: used to retrieve the cursor position.
		* param "vPos": position in normalized range [0; 1], may contain negative values if the cursor is outside of the window.
		* return: false if successful, true otherwise.
		* remarks: this function will return an error if it was called when the cursor is hidden, or if init() was not called before.
		*/
		bool                   getCursorPos                    (SVector& vPos);
		//@@Function
		/*
		* desc: used to retrieve the cursor position in 3D space (camera position) and it's direction from the camera (eyes).
		* return: false if successful, true otherwise.
		* remarks: this function will return an error if it was called when the cursor is hidden, or if run() was not called before.
		*/
		bool                   getCursor3DPosAndDir            (SVector& vPos, SVector& vDir);
		//@@Function
		/*
		* desc: used to retrieve the main window size.
		* param "vSize": window size in pixels.
		* return: false if successful, true otherwise.
		* remarks: should be called after SApplication::init().
		*/
		bool                   getWindowSize                   (SVector& vSize);
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

	// Other

		//@@Function
		/*
		* desc: used to show a usual Windows message box.
		*/
		void                   showMessageBox                   (const std::wstring& sMessageBoxTitle, const std::wstring& sMessageText) const;

		//@@Function
		/*
		* desc: used to open a URL in the default browser.
		* remarks: example "https://www.google.com/".
		*/
		void                   openInternetURL                  (const std::wstring& sURL);

		//@@Function
		/*
		* desc: enabling this will copy screen pixels, for next frame only, to specified pPixels buffer.
		* param "pPixels": unsigned char array with the size equal to (width x height x 4), use getWindowSize() to get width and height.
		* remarks: you can access the pixels in onTick() function, each pixel is in 'rgba' format where each component is 'unsigned char'.
		*/
		void                   makeOneCopyOfScreenPixelsToCustomBuffer(unsigned char* pPixels);


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
		//@@Function
		/*
		* desc: an overridable function, called fixed amount of times in second (60 by default).
		* param "fDeltaTime": the time that has passed since the last onPhysicsTick() call.
		*/
		virtual void onPhysicsTick(float fDeltaTime) {};


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
		//@@Function
		/*
		* desc: an overridable function, called just before the window loses focus.
		*/
		virtual void onLoseFocus    () {};
		//@@Function
		/*
		* desc: an overridable function, called after the window gains focus.
		*/
		virtual void onGainFocus    () {};

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
		* desc: creates CBV/SRV/UAV descriptor heap.
		* return: false if successful, true otherwise.
		*/
		bool createCBVSRVUAVHeap             ();
		//@@Function
		/*
		* desc: creates CBVs/SRVs/UAVs.
		*/
		void createViews                     ();
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
		bool createRootSignature             (SCustomShaderResources* pCustomShaderResources = nullptr, bool bUseTextures = false, bool bUseInstancing = false);
		bool createBlurRootSignature         ();
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
		bool createPSO                       (SShader* pPSOsForCustomShader = nullptr);
		//@@Function
		/*
		* desc: resets command list so it can be used.
		* return: false if successful, true otherwise.
		*/
		bool resetCommandList                ();
		//@@Function
		/*
		* desc: creates the default material, all objects without a material will have default material.
		*/
		bool createDefaultMaterial            ();
		//@@Function
		/*
		* desc: executes the commands from list.
		* return: false if successful, true otherwise.
		*/
		bool executeCommandList              ();

		void updateMaterialInFrameResource   (SMaterial* pMaterial, SUploadBuffer<SMaterialConstants>* pCustomResource = nullptr,
			size_t iElementIndexInResource = 0);


	// Window

		//@@Function
		/*
		* desc: called when the window size was changed. Resizes the swap chain and buffers.
		* return: false if successful, true otherwise.
		* remarks: only call under mtxDraw.
		*/
		bool onResize                        ();


	// Game

		//@@Function
		/*
		* desc: updates the camera and buffers.
		*/
		void update                          ();
		void updateMaterials                 ();
		//@@Function
		/*
		* desc: updates the constant buffers.
		*/
		void updateObjectCBs                 ();
		//@@Function
		/*
		* desc: updates the objects' constant buffers.
		*/
		void updateComponentAndChilds        (SComponent* pComponent, SUploadBuffer<SObjectConstants>* pCurrentObjectCB);
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
		void drawOpaqueComponents            ();
		void drawTransparentComponents       ();
		void drawGUIObjects                  ();
		void drawComponent                   (SComponent* pComponent, bool bUsingCustomResources = false);
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
		//@@Function
		/*
		* desc: enable/disable GUI rendering.
		*/
		void setDrawGUI                      (bool bDraw);


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

		//bool setFullscreen   (bool bFullScreen);
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
		void setBackBufferFillColor               (const SVector& vColor);
		//@@Function
		/*
		* desc: used to enable/disable wireframe display mode.
		* param "bEnable": true to enable, false to disable.
		*/
		void setEnableWireframeMode               (bool bEnable);

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


	// Display adapters.

		//@@Function
		/*
		* desc: returns the list of the display adapters (i.e. GPUs) on your PC that engine supports.
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


	// Level

		//@@Function
		/*
		* desc: used to spawn container in level.
		* param "pContainer": the pointer to a container that you want to spawn.
		* return: false if successful, true otherwise.
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
		* remarks: only call under mtxDraw
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

		std::array<const CD3DX12_STATIC_SAMPLER_DESC, 3> getStaticSamples();

		void internalPhysicsTickThread();



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


	// only call this under mtxDraw
	void releaseShader(SShader* pShader);
	void removeShaderFromObjects(SShader* pShader, std::vector<SShaderObjects>* pObjectsByShader);
	void forceChangeMeshShader(SShader* pOldShader, SShader* pNewShader, SComponent* pComponent, bool bUsesTransparency);

	void saveBackBufferPixels();

	// Compute shaders.
	// only call this under mtxDraw
	void executeCustomComputeShaders(bool bBeforeDraw);
	void executeCustomComputeShader(SComputeShader* pComputeShader);
	void doOptionalPauseForUserComputeShaders();
	void copyUserComputeResults(SComputeShader* pComputeShader);

	// Checks.
	bool doesComponentExists(SComponent* pComponent);
	bool doesComputeShaderExists(SComputeShader* pShader);

	// Material bundles.
	std::vector<SUploadBuffer<SMaterialConstants>*> createBundledMaterialResource(SShader* pShader, size_t iMaterialsCount);
	SMaterial* registerMaterialBundleElement(const std::string& sMaterialName, bool& bErrorOccurred);

	// Frustum culling.
	bool doFrustumCulling(SComponent* pComponent);
	void doFrustumCullingOnInstancedMesh(SMeshComponent* pMeshComponent, UINT64& iOutVisibleInstanceCount);

	// Other.
	void showDeviceRemovedReason();
	bool nanosleep(long long ns);
	void setTransparentPSO();
	void removeComponentsFromGlobalVectors(SContainer* pContainer);
	void moveGUIObjectToLayer(SGUIObject* pObject, int iNewLayer);
	void refreshHeap();


	// -----------------------------------------------------------------

	friend class SComponent;
	friend class SMeshComponent;
	friend class SRuntimeMeshComponent;
	friend class SComputeShader;
	friend class SCameraComponent;
	friend class SMaterial;
	friend class SLevel;
	friend class SError;
	friend class SCustomShaderResources;
	friend class SGUIObject;
	friend class SGUIImage;
	friend class SGUISimpleText;


	static SApplication* pApp;

	
	// Main DX objects.
	Microsoft::WRL::ComPtr<IDXGIFactory4>   pFactory;
	Microsoft::WRL::ComPtr<ID3D12Device>    pDevice;
	Microsoft::WRL::ComPtr<IDXGIAdapter3>   pAdapter;
	Microsoft::WRL::ComPtr<IDXGIOutput>     pOutput;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> pSwapChain;


	// GUI.
	std::vector<SGUILayer>                   vGUILayers;
	std::unique_ptr<DirectX::GraphicsMemory> pDXTKGraphicsMemory;
	bool                                     bDrawGUI = true;
	std::wstring                             sPathToDefaultFont = L"res/default_font.spritefont";


	// PSOs
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pOpaquePSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pOpaqueLineTopologyPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pTransparentPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pTransparentAlphaToCoveragePSO;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pOpaqueWireframePSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pTransparentWireframePSO;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pBlurHorizontalPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pBlurVerticalPSO;


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
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pCBVSRVUAVHeap;
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
	Microsoft::WRL::ComPtr<ID3D12RootSignature>  pBlurRootSignature;
	std::unordered_map<std::string, ATL::CComPtr<IDxcBlob>> mShaders;


	// CB constants.
	SRenderPassConstants mainRenderPassCB;
	int iPerFrameResEndOffset = 0;
	size_t iActualObjectCBCount = 0;


	// Materials / Textures / Shaders
	std::vector<SMaterial*> vRegisteredMaterials;
	std::string sDefaultEngineMaterialName = "Default Engine Material";
	std::vector<STextureInternal*> vLoadedTextures;
	TEX_FILTER_MODE textureFilterIndex = TEX_FILTER_MODE::TFM_ANISOTROPIC;
	std::vector<SShader*> vCompiledUserShaders;
	std::vector<SShaderObjects> vOpaqueMeshesByCustomShader;
	std::vector<SShaderObjects> vTransparentMeshesByCustomShader;
	std::vector<SComputeShader*> vUserComputeShaders;


	// Buffer formats.
	DXGI_FORMAT BackBufferFormat   = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;


	// Video settings.
	friend class SVideoSettings;
	SVideoSettings* pVideoSettings;


	// Profiler.
	friend class SProfiler;
	SProfiler*   pProfiler;

	
	// Level / Objects
	SLevel*        pCurrentLevel = nullptr;
	std::vector<SContainer*> vAllRenderableSpawnedContainers;
	std::vector<SContainer*> vAllNonrenderableSpawnedContainers;
	std::vector<SComponent*> vAllRenderableSpawnedOpaqueComponents;
	std::vector<SComponent*> vAllRenderableSpawnedTransparentComponents;


	// MSAA.
	Microsoft::WRL::ComPtr<ID3D12Resource>  pMSAARenderTarget;
	bool           MSAA_Enabled             = true;
	UINT           MSAA_SampleCount         = 4;
	UINT           MSAA_Quality             = 0;

	
	// Blur.
	std::unique_ptr<SBlurEffect> pBlurEffect;


	// Screen.
	bool           bFullscreen              = true;
	bool           bSaveBackBufferPixelsForUser = false;
	unsigned char* pPixels = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pPixelsReadBackBuffer;
	UINT64         iPixelsBufferSize        = 0;
	int            iMainWindowWidth         = 800;
	int            iMainWindowHeight        = 600;
	UINT           iRefreshRateNumerator    = 60;
	UINT           iRefreshRateDenominator  = 1;
	DXGI_MODE_SCANLINE_ORDER iScanlineOrder = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	DXGI_MODE_SCALING        iScaling       = DXGI_MODE_SCALING_UNSPECIFIED;


	// Viewport / Camera
	D3D12_VIEWPORT ScreenViewport; 
	D3D12_RECT     ScissorRect;
	float          fMinDepth = 0.0f;
	float          fMaxDepth = 1.0f;
	SCamera        camera;
	DirectX::BoundingFrustum cameraBoundingFrustumOnLastMainPassUpdate;

	
	// Windows stuff.
	std::wstring   sMainWindowClassName;
	HINSTANCE      hApplicationInstance     = nullptr;
	HWND           hMainWindow              = nullptr;
	SMouseKey      pressedMouseKey = SMouseKey();
	bool           bMouseCursorShown        = true;
	bool           bRawInputReady           = false;
	bool           bHideTitleBar            = true;
	long           iWindowCenterX           = 0;
	long           iWindowCenterY           = 0;


	// VSunc
	bool           bVSyncEnabled            = false;


	// Graphics
	SGlobalVisualSettings renderPassVisualSettings;
	float          backBufferFillColor[4]   = {0.0f, 0.0f, 0.0f, 1.0f};
	bool           bUseFillModeWireframe    = false;


	// Performance
#if defined(DEBUG) || defined(_DEBUG)
	SFrameStats frameStats;
#endif
	unsigned long long iLastFrameDrawCallCount  = 0;
	int            iFPS                     = 0;
	float          fTimeToRenderFrame       = 0.0f;
	float          fFPSLimit                = 0.0f;
	double         dDelayBetweenFramesInNS  = 0.0;
	bool           bShowFrameStatsInTitle   = false;
	bool           bCompileShadersInRelease = false;


	// Audio.
	SAudioEngine*  pAudioEngine = nullptr;

	
	// Physics.
	int            iPhysicsTicksPerSecond = 60;
	bool           bTerminatePhysics = false;
	std::promise<bool> promiseFinishedPhysics;
	std::future<bool> futureFinishedPhysics;


	SGameTimer     gameTimer;
	SGameTimer     gamePhysicsTimer;

	
	std::mutex     mtxDraw;
	std::mutex     mtxFenceUpdate;


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
	bool           bExitCalled              = false;

	bool           bCallTick                = false;
	bool           bDisableKeyboardRepeat   = true;

	bool           bD3DDebugLayerEnabled    = true;
};