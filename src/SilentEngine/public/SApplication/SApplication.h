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

// DirectX
#include <wrl.h> // smart pointers
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include "SilentEngine/private/d3dx12.h"
#include <DirectXColors.h>
#include <DirectXMath.h>

// Custom
#include "SilentEngine/private/SGameTimer/SGameTimer.h"
#include "SilentEngine/private/SGeometry/SGeometry.h"
#include "SilentEngine/private/SUploadBuffer/SUploadBuffer.h"

// Other
#include <Windows.h>
#include <Windowsx.h>


// Libs
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")



#define ENGINE_D3D_FEATURE_LEVEL D3D_FEATURE_LEVEL_12_1



// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

enum SMouseButton
{
	SMB_NONE   = 0,
	SMB_LEFT   = 1,
	SMB_RIGHT  = 2,
	SMB_MIDDLE = 3,
	SMB_X1     = 4,
	SMB_X2     = 5,
};

class SMouseKey
{
public:

	SMouseKey(SMouseButton mouseButton, WPARAM wParam)
	{
		bCtrlPressed  = false;
		bShiftPressed = false;

		this->mouseButton  = SMB_NONE;


		if (wParam & MK_CONTROL)
		{
			bCtrlPressed = true;
		}

		if (wParam & MK_SHIFT)
		{
			bShiftPressed = true;
		}

		this->mouseButton = mouseButton;
	}

	SMouseKey(WPARAM wParam)
	{
		bCtrlPressed  = false;
		bShiftPressed = false;

		mouseButton  = SMB_NONE;


		if (wParam & MK_CONTROL)
		{
			bCtrlPressed = true;
		}

		if (wParam & MK_SHIFT)
		{
			bShiftPressed = true;
		}

		if (wParam & MK_LBUTTON)
		{
			mouseButton = SMouseButton::SMB_LEFT;
		}
		else if (wParam & MK_MBUTTON)
		{
			mouseButton = SMouseButton::SMB_MIDDLE;
		}
		else if (wParam & MK_RBUTTON)
		{
			mouseButton = SMouseButton::SMB_RIGHT;
		}
		else if (GET_XBUTTON_WPARAM(wParam) & XBUTTON1)
		{
			mouseButton = SMouseButton::SMB_X1;
		}
		else if (GET_XBUTTON_WPARAM(wParam) & XBUTTON2)
		{
			mouseButton = SMouseButton::SMB_X2;
		}
	}

	SMouseButton getButton()
	{
		return mouseButton;
	}

	bool isCtrlPressed()
	{
		return bCtrlPressed;
	}

	bool isShiftPressed()
	{
		return bShiftPressed;
	}

private:

	SMouseButton mouseButton;

	bool bCtrlPressed;
	bool bShiftPressed;
};

struct SScreenResolution
{
	UINT iWidth;
	UINT iHeight;
};

enum MSAASampleCount
{
	SC_2  = 2,
	SC_4  = 4,
	SC_16 = 16,
	SC_32 = 32,
};

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



class SApplication
{
protected:

	SApplication(HINSTANCE hInstance);
	SApplication(const SApplication&) = delete;
	SApplication& operator= (const SApplication&) = delete;
	virtual ~SApplication();

public:

	// Public, because we call it like this: SilentApplication::getSilentApp()->msgProc()
	virtual LRESULT msgProc									   (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);



	// Main functions

		bool            init								   ();
		int             run                                    ();



	// "Set" functions

		// Should be called before init():

		bool            setInitMainWindowTitle				   (std::wstring sMainWindowTitle);
		bool            setInitPreferredDisplayAdapter		   (std::wstring sPreferredDisplayAdapter);
		bool            setInitPreferredOutputAdapter		   (std::wstring sPreferredOutputAdapter);
		bool            setInitBackBufferFormat				   (DXGI_FORMAT format);
		bool            setInitDepthStencilBufferFormat		   (DXGI_FORMAT format);
		bool            setInitFullscreen					   (bool bFullscreen);
		bool            setInitEnableVSync                     (bool bEnable);

		// Multisampling

		void            setMSAAEnabled                         (bool bEnable);
		bool            setMSAASampleCount                     (MSAASampleCount eSampleCount);

		// Screen

		bool            setFullscreenWithCurrentResolution     (bool bFullScreen);
		bool            setScreenResolution                    (UINT iWidth, UINT iHeight);
		bool            setFOV                                 (float fFOVInGrad);

		// Game

		void            setCallTick                            (bool bTickCanBeCalled);



	// "Get" functions

		static SApplication*  getApp                ();

		// Display adapters.

		std::vector<std::wstring>  getSupportedDisplayAdapters ();
		std::wstring               getCurrentDisplayAdapter    ();
		bool                       getVideoMemorySizeInBytesOfCurrentDisplayAdapter(SIZE_T* pSizeInBytes);
		bool                       getVideoMemoryUsageInBytesOfCurrentDisplayAdapter(UINT64* pSizeInBytes);

		// Output displays.

		std::vector<std::wstring>  getOutputDisplaysOfCurrentDisplayAdapter();
		std::wstring               getCurrentOutputDisplay     ();
		float                      getCurrentRefreshRate       ();
		bool                       getCurrentScreenResolution  (SScreenResolution* pScreenResolution);
		bool                       getAvailableScreenResolutionsOfCurrentOutputDisplay(std::vector<SScreenResolution>* vResolutions);
		bool                       isFullscreen                ();

		// Game

		bool                       getTimeElapsedFromStart     (float* fTimeInSec);
		bool                       getFPS                      (int* iFPS);
		bool                       getAvrTimeToRenderFrame     (float* fTimeInMS);

		// Other

		float                      getScreenAspectRatio        () const;
		HWND                       getMainWindowHandle         () const;


protected:

	// Game

		virtual void onTick() {};


	// Input

		virtual void onMouseDown(SMouseKey mouseKey, int iMouseXPos, int iMouseYPos) {};
		virtual void onMouseUp  (SMouseKey mouseKey, int iMouseXPos, int iMouseYPos) {};
		virtual void onMouseMove(SMouseKey mouseKey, int iMouseXPos, int iMouseYPos) {};


	// Viewport

		float fTheta  = 1.5f * DirectX::XM_PI;
		float fPhi    = DirectX::XM_PIDIV4;
		float fRadius = 5.0f;

private:

	// Initialization

		bool createMainWindow                ();
		bool initD3DSecondStage              ();
		bool initD3DFirstStage               ();
		bool getFirstSupportedDisplayAdapter (IDXGIAdapter3** ppAdapter);
		bool getFirstOutputDisplay           (IDXGIOutput** ppOutput);
		bool checkMSAASupport                ();
		bool createCommandObjects            ();
		bool createSwapChain                 ();
		bool getScreenParams                 (bool bApplyResolution);
		bool createRTVAndDSVDescriptorHeaps  ();
		bool createCBVDescriptorHeap         ();
		void createConstantBuffer            ();
		bool createRootSignature             ();
		bool createShadersAndInputLayout     ();
		bool createBoxGeometry               ();
		bool createPSO                       ();


	// Window

		bool onResize                        ();


	// Game

		void updateViewMatrix                ();
		void draw                            ();


	// Other

		bool flushCommandQueue               ();
		void calculateFrameStats             ();


	// "Get" functions

		ID3D12Resource*             getCurrentBackBufferResource() const;
		D3D12_CPU_DESCRIPTOR_HANDLE getCurrentBackBufferViewHandle() const;
		D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilViewHandle() const;



	// -----------------------------------------------------------------



	static SApplication* pApp;

	
	// Main DX objects.
	Microsoft::WRL::ComPtr<IDXGIFactory4>   pFactory;
	Microsoft::WRL::ComPtr<ID3D12Device>    pDevice;
	Microsoft::WRL::ComPtr<IDXGIAdapter3>   pAdapter;
	Microsoft::WRL::ComPtr<IDXGIOutput>     pOutput;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> pSwapChain;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pPSO;


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


	// Buffers and shaders.
	std::unique_ptr<SUploadBuffer<SObjectConstants>> pObjectConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>  pRootSignature;
	Microsoft::WRL::ComPtr<ID3DBlob>             pVSByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob>             pPSByteCode;
	std::vector<D3D12_INPUT_ELEMENT_DESC>        vInputLayout;
	std::unique_ptr<SMeshGeometry>               pBoxGeometry;


	// Matrices.
	DirectX::XMFLOAT4X4 vWorld = SMath::getIdentityMatrix4x4();
	DirectX::XMFLOAT4X4 vView  = SMath::getIdentityMatrix4x4();
	DirectX::XMFLOAT4X4 vProj  = SMath::getIdentityMatrix4x4();


	// Buffer formats.
	DXGI_FORMAT BackBufferFormat            = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT DepthStencilFormat          = DXGI_FORMAT_D24_UNORM_S8_UINT;


	// MSAA.
	bool           MSAA_Enabled             = false;
	UINT           MSAA_SampleCount         = 4;
	UINT           MSAA_Quality             = 0;


	// Screen.
	bool           bFullscreen              = false;
	int            iMainWindowWidth         = 800;
	int            iMainWindowHeight        = 600;
	float          fFOVInGrad               = 60;
	UINT           iRefreshRateNumerator    = 60;
	UINT           iRefreshRateDenominator  = 1;
	DXGI_MODE_SCANLINE_ORDER iScanlineOrder = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	DXGI_MODE_SCALING        iScaling       = DXGI_MODE_SCALING_UNSPECIFIED;


	// Viewport
	D3D12_VIEWPORT  ScreenViewport; 
	D3D12_RECT      ScissorRect;

	
	// Windows stuff.
	HINSTANCE      hApplicationInstance     = nullptr;
	HWND           hMainWindow              = nullptr;


	// VSunc
	bool           bVSyncEnabled            = false;


	// Performance
	int            iFPS                     = 0;
	float          fAvrTimeToRenderFrame    = 0.0f;


	SGameTimer     gameTimer;


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
};

