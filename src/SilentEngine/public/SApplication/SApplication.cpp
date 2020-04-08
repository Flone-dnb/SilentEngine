// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#include "SApplication.h"

// STL
#include <array>

// Custom
#include "SilentEngine/private/SError/SError.h"
#include "SilentEngine/public/STimer/STimer.h"


SApplication* SApplication::pApp = nullptr;


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------



bool SApplication::setInitMainWindowTitle(std::wstring sMainWindowTitle)
{
	if (bInitCalled == false)
	{
		this->sMainWindowTitle = sMainWindowTitle;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setInitMainWindowTitle(). Error: this function should be called before init() call.", L"Error", 0);
		return true;
	}
}

bool SApplication::setInitPreferredDisplayAdapter(std::wstring sPreferredDisplayAdapter)
{
	if (bInitCalled == false)
	{
		this->sPreferredDisplayAdapter = sPreferredDisplayAdapter;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setInitPreferredDisplayAdapter(). Error: this function should be called before init() call.", L"Error", 0);
		return true;
	}
}

bool SApplication::setInitPreferredOutputAdapter(std::wstring sPreferredOutputAdapter)
{
	if (bInitCalled == false)
	{
		this->sPreferredOutputAdapter = sPreferredOutputAdapter;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setInitPreferredOutputAdapter(). Error: this function should be called before init() call.", L"Error", 0);
		return true;
	}
}

bool SApplication::setInitBackBufferFormat(DXGI_FORMAT format)
{
	if (bInitCalled == false)
	{
		BackBufferFormat = format;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setInitBackBufferFormat(). Error: this function should be called before init() call.", L"Error", 0);
		return true;
	}
}

bool SApplication::setInitDepthStencilBufferFormat(DXGI_FORMAT format)
{
	if (bInitCalled == false)
	{
		DepthStencilFormat = format;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setInitDepthStencilBufferFormat(). Error: this function should be called before init() call.", L"Error", 0);
		return true;
	}
}

bool SApplication::setInitFullscreen(bool bFullscreen)
{
	if (bInitCalled == false)
	{
		this->bFullscreen = bFullscreen;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setInitFullscreen(). Error: this function should be called before init() call.", L"Error", 0);
		return true;
	}
}

bool SApplication::setInitEnableVSync(bool bEnable)
{
	if (bInitCalled == false)
	{
		bVSyncEnabled = bEnable;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setInitEnableVSync(). Error: this function should be called before init() call.", L"Error", 0);
		return true;
	}
}

void SApplication::setMSAAEnabled(bool bEnable)
{
	if (MSAA_Enabled != bEnable)
	{
		MSAA_Enabled = bEnable;

		mtxDraw.lock();

		createSwapChain();
		onResize();

		mtxDraw.unlock();
	}
}

bool SApplication::setMSAASampleCount(MSAASampleCount eSampleCount)
{
	if (pDevice)
	{
		if (MSAA_SampleCount != eSampleCount)
		{
			MSAA_SampleCount = eSampleCount;

			if (checkMSAASupport())
			{
				return true;
			}

			if (MSAA_Enabled)
			{
				mtxDraw.lock();

				if (createSwapChain())
				{
					mtxDraw.unlock();

					return true;
				}

				onResize();

				mtxDraw.unlock();
			}
		}

		return false;
	}
	else
	{
		return true;
	}
}

bool SApplication::setFullscreenWithCurrentResolution(bool bFullscreen)
{
	if (bInitCalled)
	{
		if (this->bFullscreen != bFullscreen)
		{
			mtxDraw.lock();

			this->bFullscreen = bFullscreen;

			HRESULT hresult;

			if (bFullscreen)
			{
				hresult = pSwapChain->SetFullscreenState(bFullscreen, pOutput.Get());
			}
			else
			{
				// From docs: "pTarget - If you pass FALSE to Fullscreen, you must set this parameter to NULL."
				hresult = pSwapChain->SetFullscreenState(bFullscreen, NULL);
			}

			if (FAILED(hresult))
			{
				SError::showErrorMessageBox(hresult, L"SApplication::setFullscreen::IDXGISwapChain::SetFullscreenState()");
				mtxDraw.unlock();
				return true;
			}
			else
			{
				// Resize the buffers.
				onResize();
			}

			mtxDraw.unlock();
		}

		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setFullscreen(). Error: init() should be called first.", L"Error", 0);
		return true;
	}
}

bool SApplication::setScreenResolution(UINT iWidth, UINT iHeight)
{
	if (bInitCalled)
	{
		if (iMainWindowWidth != iWidth || iMainWindowHeight != iHeight)
		{
			iMainWindowWidth  = iWidth;
			iMainWindowHeight = iHeight;

			bCustomWindowSize = true;

			getScreenParams(true);

			DXGI_MODE_DESC desc;
			desc.Format = BackBufferFormat;
			desc.Width  = iMainWindowWidth;
			desc.Height = iMainWindowHeight;
			desc.RefreshRate.Numerator   = iRefreshRateNumerator;
			desc.RefreshRate.Denominator = iRefreshRateDenominator;
			desc.Scaling = iScaling;
			desc.ScanlineOrdering = iScanlineOrder;

			mtxDraw.lock();

			pSwapChain->ResizeTarget(&desc);

			// Resize the buffers.
			onResize();

			mtxDraw.unlock();
		}
		
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::setScreenResolution(). Error: init() should be called first.", L"Error", 0);
		return true;
	}
}

bool SApplication::setFOV(float fFOVInGrad)
{
	if (fFOVInGrad > 150 || fFOVInGrad < 60)
	{
		MessageBox(0, L"An error occurred at SApplication::setFOV(). Error: the FOV value should be in the range [60; 150].", L"Error", 0);
		return true;
	}
	else
	{
		this->fFOVInGrad = fFOVInGrad;
		return false;
	}
}

bool SApplication::setNearClipPlane(float fNearClipPlaneValue)
{
	if ( (fNearClipPlaneValue < 0.0f) || (bInitCalled == false) )
	{
		MessageBox(0, L"An error occurred at SApplication::setNearClipPlane(). Error: the fNearClipPlaneValue value should be"
			" more than 0 and the init() function should be called first.", L"Error", 0);
		return true;
	}
	else
	{
		this->fNearClipPlaneValue = fNearClipPlaneValue;

		if (bInitCalled)
		{
			mtxDraw.lock();

			onResize();

			mtxDraw.unlock();
		}

		return false;
	}
}

bool SApplication::setFarClipPlane(float fFarClipPlaneValue)
{
	if ( (fFarClipPlaneValue < 0.0f) || (bInitCalled == false) )
	{
		MessageBox(0, L"An error occurred at SApplication::setFarClipPlane(). Error: the fFarClipPlaneValue value should be"
			" more than 0 and the init() function should be called first.", L"Error", 0);
		return true;
	}
	else
	{
		this->fFarClipPlaneValue = fFarClipPlaneValue;

		if (bInitCalled)
		{
			mtxDraw.lock();

			onResize();

			mtxDraw.unlock();
		}

		return false;
	}
}

void SApplication::setCallTick(bool bTickCanBeCalled)
{
	bCallTick = bTickCanBeCalled;
}

void SApplication::setFPSLimit(float fFPSLimit)
{
	if (fFPSLimit <= 0.0f)
	{
		this->fFPSLimit = 0.0f;
		fDelayBetweenFramesInMS = 0.0f;
	}
	else
	{
		this->fFPSLimit = fFPSLimit;
		fDelayBetweenFramesInMS = 1000.0 / fFPSLimit;
	}
}

void SApplication::setShowFrameStatsInTitle(bool bShow)
{
	bShowFrameStatsInTitle = bShow;
}

SApplication* SApplication::getApp()
{
	return pApp;
}

std::vector<std::wstring> SApplication::getSupportedDisplayAdapters()
{
	std::vector<std::wstring> vSupportedAdapters;

	if (pFactory)
	{
		for(UINT adapterIndex = 0; ; ++adapterIndex)
		{
			IDXGIAdapter3* pAdapter1 = nullptr;
			if (pFactory->EnumAdapters(adapterIndex, reinterpret_cast<IDXGIAdapter**>(&pAdapter1)) == DXGI_ERROR_NOT_FOUND)
			{
				// No more adapters to enumerate.
				break;
			} 



			// Check to see if the adapter supports Direct3D version, but don't create the actual device yet.

			if (SUCCEEDED(D3D12CreateDevice(pAdapter1, ENGINE_D3D_FEATURE_LEVEL, _uuidof(ID3D12Device), nullptr)))
			{
				DXGI_ADAPTER_DESC desc;
				pAdapter1->GetDesc(&desc);

				vSupportedAdapters.push_back(desc.Description);

				pAdapter1->Release();
				pAdapter1 = nullptr;
			}
		}
	}
	else
	{
		vSupportedAdapters.push_back(L"Error. DXGIFactory was not created.");
	}

	return vSupportedAdapters;
}

std::wstring SApplication::getCurrentDisplayAdapter()
{
	if (pAdapter)
	{
		if (bUsingWARPAdapter)
		{
			return L"WARP software adapter.";
		}
		else
		{
			DXGI_ADAPTER_DESC desc;
			pAdapter.Get()->GetDesc(&desc);

			std::wstring sCurrentAdapter = desc.Description;

			return sCurrentAdapter;
		}
	}
	else
	{
		return L"Adapter is not created.";
	}
}

bool SApplication::getVideoMemorySizeInBytesOfCurrentDisplayAdapter(SIZE_T* pSizeInBytes)
{
	if (pAdapter)
	{
		DXGI_ADAPTER_DESC desc;
		pAdapter.Get()->GetDesc(&desc);

		*pSizeInBytes = desc.DedicatedVideoMemory;

		return false;
	}
	else
	{
		return true;
	}
}

bool SApplication::getVideoMemoryUsageInBytesOfCurrentDisplayAdapter(UINT64* pSizeInBytes)
{
	if (pAdapter)
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
		pAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);

		*pSizeInBytes = videoMemoryInfo.CurrentUsage;

		return false;
	}
	else
	{
		return true;
	}
}

std::vector<std::wstring> SApplication::getOutputDisplaysOfCurrentDisplayAdapter()
{
	std::vector<std::wstring> vOutputAdapters;

	if (pFactory)
	{
		if (pAdapter)
		{
			for(UINT outputIndex = 0; ; ++outputIndex)
			{
				IDXGIOutput* pOutput1 = nullptr;
				if (pAdapter->EnumOutputs(outputIndex, &pOutput1) == DXGI_ERROR_NOT_FOUND)
				{
					// No more displays to enumerate.
					break;
				} 

				DXGI_OUTPUT_DESC desc;
				pOutput1->GetDesc(&desc);

				vOutputAdapters.push_back(desc.DeviceName);

				pOutput1->Release();
				pOutput1 = nullptr;
			}
		}
		else
		{
			vOutputAdapters.push_back(L"Error. DXGIAdapter was not created.");
		}
	}
	else
	{
		vOutputAdapters.push_back(L"Error. DXGIFactory was not created.");
	}

	return vOutputAdapters;
}

bool SApplication::getAvailableScreenResolutionsOfCurrentOutputDisplay(std::vector<SScreenResolution>* vResolutions)
{
	if (pOutput)
	{
		UINT numModes = 0;

		HRESULT hresult = pOutput->GetDisplayModeList(BackBufferFormat, 0, &numModes, NULL);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::getSupportedScreenResolutionsOfCurrentOutputDisplay::IDXGIOutput::GetDisplayModeList() (count)");
			return true;
		}

		std::vector<DXGI_MODE_DESC> vDisplayModes(numModes);

		hresult = pOutput->GetDisplayModeList(BackBufferFormat, 0, &numModes, &vDisplayModes[0]);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::getSupportedScreenResolutionsOfCurrentOutputDisplay::IDXGIOutput::GetDisplayModeList() (list)");
			return true;
		}


		
		// Get result.

		for (size_t i = 0; i < vDisplayModes.size(); i++)
		{
			if ((vDisplayModes[i].ScanlineOrdering == iScanlineOrder)
				&&
				(vDisplayModes[i].Scaling == iScaling))
			{
				SScreenResolution sr;
				sr.iWidth  = vDisplayModes[i].Width;
				sr.iHeight = vDisplayModes[i].Height;

				vResolutions->push_back(sr);
			}
		}

		return false;
	}
	else
	{
		return true;
	}
}

std::wstring SApplication::getCurrentOutputDisplay()
{
	if (pOutput)
	{
		DXGI_OUTPUT_DESC desc;
		pOutput.Get()->GetDesc(&desc);

		std::wstring sCurrentAdapter = desc.DeviceName;

		return sCurrentAdapter;
	}
	else
	{
		return L"Adapter is not created.";
	}
}

float SApplication::getCurrentRefreshRate()
{
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC desc;

	HRESULT hresult = pSwapChain->GetFullscreenDesc(&desc);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::getCurrentScreenResolution::IDXGISwapChain1::GetFullscreenDesc()");
		return 0.0f;
	}
	else
	{
		return desc.RefreshRate.Numerator / static_cast<float>(desc.RefreshRate.Denominator);
	}
}

bool SApplication::getCurrentScreenResolution(SScreenResolution* pScreenResolution)
{
	if (bInitCalled)
	{
		DXGI_SWAP_CHAIN_DESC1 desc;

		HRESULT hresult = pSwapChain->GetDesc1(&desc);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::getCurrentScreenResolution::IDXGISwapChain1::GetDesc1()");
			return true;
		}
		else
		{
			pScreenResolution->iWidth  = desc.Width;
			pScreenResolution->iHeight = desc.Height;

			return false;
		}
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::getCurrentScreenResolution(). Error: init() should be called first.", L"Error", 0);
		return true;
	}
}

bool SApplication::isFullscreen()
{
	return bFullscreen;
}

float SApplication::getNearClipPlaneValue()
{
	return fNearClipPlaneValue;
}

float SApplication::getFarClipPlaneValue()
{
	return fFarClipPlaneValue;
}

bool SApplication::getTimeElapsedFromStart(float* fTimeInSec)
{
	if (bRunCalled)
	{
		*fTimeInSec = gameTimer.getTimeElapsedInSec();

		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::getTimeElapsedNonPausedFromStart(). Error: run() should be called first.", L"Error", 0);
		return true;
	}
}

bool SApplication::getFPS(int* iFPS)
{
	if (bRunCalled)
	{
		*iFPS = this->iFPS;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::getFPS(). Error: run() should be called first.", L"Error", 0);
		return true;
	}
}

bool SApplication::getAvrTimeToRenderFrame(float* fTimeInMS)
{
	if (bRunCalled)
	{
		*fTimeInMS = fAvrTimeToRenderFrame;
		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::getFPS(). Error: run() should be called first.", L"Error", 0);
		return true;
	}
}

float SApplication::getScreenAspectRatio() const
{
	return static_cast<float>(iMainWindowWidth) / iMainWindowHeight;
}

HWND SApplication::getMainWindowHandle() const
{
	return hMainWindow;
}

bool SApplication::onResize()
{
	if (bInitCalled)
	{
		// Flush before changing any resources.
		if (flushCommandQueue())
		{
			return true;
		}

		HRESULT hresult = pCommandList->Reset(pCommandListAllocator.Get(), nullptr);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::onResize::ID3D12GraphicsCommandList::Reset()");
			return true;
		}




		// Release the previous resources we will be recreating.

		for (int i = 0; i < iSwapChainBufferCount; i++)
		{
			pSwapChainBuffer[i].Reset();
		}

		pDepthStencilBuffer.Reset();




		// Resize the swap chain.

		if (bVSyncEnabled)
		{
			hresult = pSwapChain->ResizeBuffers(iSwapChainBufferCount, iMainWindowWidth, iMainWindowHeight, BackBufferFormat,
				DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		}
		else
		{
			hresult = pSwapChain->ResizeBuffers(iSwapChainBufferCount, iMainWindowWidth, iMainWindowHeight, BackBufferFormat,
				DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
		}

		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::onResize::IDXGISwapChain::ResizeBuffers()");
			return true;
		}


		iCurrentBackBuffer = 0;




		// Create RTV.

		CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(pRTVHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT i = 0; i < iSwapChainBufferCount; i++)
		{
			hresult = pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pSwapChainBuffer[i]));
			if (FAILED(hresult))
			{
				SError::showErrorMessageBox(hresult, L"SApplication::onResize::IDXGISwapChain::GetBuffer() (i = " + std::to_wstring(i) + L")");
				return true;
			}

			pDevice->CreateRenderTargetView(pSwapChainBuffer[i].Get(), nullptr, RTVHeapHandle);

			RTVHeapHandle.Offset(1, iRTVDescriptorSize);
		}



		// Create the depth/stencil buffer and view.

		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment          = 0;
		depthStencilDesc.Width              = iMainWindowWidth;
		depthStencilDesc.Height             = iMainWindowHeight;
		depthStencilDesc.DepthOrArraySize   = 1;
		depthStencilDesc.MipLevels          = 1;
		depthStencilDesc.Format             = DXGI_FORMAT_R24G8_TYPELESS;
		depthStencilDesc.SampleDesc.Count   = MSAA_Enabled ? MSAA_SampleCount : 1;
		depthStencilDesc.SampleDesc.Quality = MSAA_Enabled ? (MSAA_Quality - 1) : 0;
		depthStencilDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;



		D3D12_CLEAR_VALUE optClear;
		optClear.Format               = DepthStencilFormat;
		optClear.DepthStencil.Depth   = 1.0f;
		optClear.DepthStencil.Stencil = 0;

		hresult = pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			IID_PPV_ARGS(pDepthStencilBuffer.GetAddressOf()));
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::onResize::ID3D12Device::CreateCommittedResource()");
			return true;
		}



		// Create DSV.

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags              = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format             = DepthStencilFormat;
		dsvDesc.Texture2D.MipSlice = 0;

		pDevice->CreateDepthStencilView(pDepthStencilBuffer.Get(), &dsvDesc, getDepthStencilViewHandle());



		// Transition the resource from its initial state to be used as a depth buffer.

		pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDepthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));



		// Execute the resize commands.

		hresult = pCommandList->Close();
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::onResize::ID3D12GraphicsCommandList::Close()");
			return true;
		}

		ID3D12CommandList* commandLists[] = { pCommandList.Get() };

		pCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);



		// Wait until resize is complete.

		if (flushCommandQueue())
		{
			return true;
		}



		// Update the viewport transform to cover the new window size.

		ScreenViewport.TopLeftX = 0;
		ScreenViewport.TopLeftY = 0;
		ScreenViewport.Width    = static_cast<float>(iMainWindowWidth);
		ScreenViewport.Height   = static_cast<float>(iMainWindowHeight);
		ScreenViewport.MinDepth = 0.0f;
		ScreenViewport.MaxDepth = 1.0f;

		ScissorRect = { 0, 0, iMainWindowWidth, iMainWindowHeight };



		// Update aspect ratio and recompute the projection matrix.

		DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(fFOVInGrad), getScreenAspectRatio(),
			fNearClipPlaneValue, fFarClipPlaneValue);
		XMStoreFloat4x4(&vProj, P);


		return false;
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::onResize(). Error: init() should be called first.", L"Error", 0);

		return true;
	}
}

void SApplication::updateViewMatrix()
{
	// Convert Spherical to Cartesian coordinates.
	float x = fRadius * sinf(fPhi) * cosf(fTheta);
	float y = fRadius * sinf(fPhi) * sinf(fTheta);
	float z = fRadius * cosf(fPhi);

	// Build the view matrix.
	DirectX::XMVECTOR pos    = DirectX::XMVectorSet(x, y, z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up     = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	DirectX::XMStoreFloat4x4(&vView, view);

	DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&vWorld);
	DirectX::XMMATRIX proj  = DirectX::XMLoadFloat4x4(&vProj);
	DirectX::XMMATRIX worldViewProj = world * view * proj;

	// Update the constant buffer with the latest worldViewProj matrix.
	SObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.vWorldViewProj, XMMatrixTranspose(worldViewProj));
	pObjectConstantBuffer->copyDataToElement(0, objConstants);
}

void SApplication::draw()
{
	mtxDraw.lock();

	// Should be only called if the GPU is not using it (i.e. command queue is empty).

	HRESULT hresult = pCommandListAllocator->Reset();
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::draw::ID3D12CommandAllocator::Reset()");
		mtxDraw.unlock();
		return;
	}



	// A command list can be reset after it has been added to the command queue via ExecuteCommandList (was added in init()).

	hresult = pCommandList->Reset(pCommandListAllocator.Get(), pPSO.Get());
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::draw::ID3D12GraphicsCommandList::Reset()");
		mtxDraw.unlock();
		return;
	}



	// Record new commands in the command list:

	// Set the viewport and scissor rect. This needs to be reset whenever the command list is reset.

	pCommandList->RSSetViewports(1, &ScreenViewport);
	pCommandList->RSSetScissorRects(1, &ScissorRect);



	// Translate back buffer state from present state to render target state.

	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBufferResource(), 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));



	// Clear buffers.

	// nullptr - "ClearRenderTargetView clears the entire resource view".
	pCommandList->ClearRenderTargetView(getCurrentBackBufferViewHandle(), DirectX::Colors::Black, 0, nullptr);
	pCommandList->ClearDepthStencilView(getDepthStencilViewHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);


	
	// Binds the RTV and DSV to the rendering pipeline.

	pCommandList->OMSetRenderTargets(1, &getCurrentBackBufferViewHandle(), true, &getDepthStencilViewHandle());



	// Graphics.

	ID3D12DescriptorHeap* descriptorHeaps[] = { pCBVHeap.Get() };
	pCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	pCommandList->SetGraphicsRootSignature(pRootSignature.Get());

	pCommandList->IASetVertexBuffers(0, 1, &pBoxGeometry->getVertexBufferView());
	pCommandList->IASetIndexBuffer(&pBoxGeometry->getIndexBufferView());

	pCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pCommandList->SetGraphicsRootDescriptorTable(0, pCBVHeap->GetGPUDescriptorHandleForHeapStart());

	pCommandList->DrawIndexedInstanced(pBoxGeometry->mDrawArgs[L"Cube"].iIndexCount, 1, 0, 0, 0);



	// Translate back buffer state from render target state to present state.

	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBufferResource(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));



	// Stop recording commands.

	hresult = pCommandList->Close();
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::draw::ID3D12GraphicsCommandList::Close()");
		mtxDraw.unlock();
		return;
	}



	// Add the command list to the command queue for execution.

	ID3D12CommandList* commandLists[] = { pCommandList.Get() };
	pCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);



	// Swap back & front buffers.

	UINT SyncInterval = 0;

	if (bVSyncEnabled)
	{
		// Max: 4.
		SyncInterval = 1;
	}

	if (bFullscreen)
	{
		// The DXGI_PRESENT_ALLOW_TEARING flag cannot be used in an application that is currently in full screen
		// exclusive mode (set by calling SetFullscreenState(TRUE)). It can only be used in windowed mode.
		hresult = pSwapChain->Present(SyncInterval, 0);
	}
	else
	{
		if (bVSyncEnabled)
		{
			hresult = pSwapChain->Present(SyncInterval, 0);
		}
		else
		{
			hresult = pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
		}
	}
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::draw::IDXGISwapChain1::Present()");
		mtxDraw.unlock();
		return;
	}
	
	if (iCurrentBackBuffer == (iSwapChainBufferCount - 1))
	{
		iCurrentBackBuffer = 0;
	}
	else
	{
		iCurrentBackBuffer++;
	}



	flushCommandQueue();

	mtxDraw.unlock();
}

bool SApplication::flushCommandQueue()
{
	iCurrentFence++;

	HRESULT hresult = pCommandQueue->Signal(pFence.Get(), iCurrentFence);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::flushCommandQueue::ID3D12CommandQueue::Signal()");
		return true;
	}


	// Wait until the GPU has completed commands up to this fence point.
	if (pFence->GetCompletedValue() < iCurrentFence)
	{
		HANDLE hEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		hresult = pFence->SetEventOnCompletion(iCurrentFence, hEvent);
		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::flushCommandQueue::ID3D12Fence::SetEventOnCompletion()");
			return true;
		}

		// Wait until event is fired.
		WaitForSingleObject(hEvent, INFINITE);
		CloseHandle(hEvent);
	}

	return false;
}

void SApplication::calculateFrameStats()
{
	static int iFrameCount = 0;
	static float fTimeElapsed = 0.0f;

	iFrameCount++;

	if ((gameTimer.getTimeElapsedInSec() - fTimeElapsed) >= 1.0f)
	{
		float fAvrTimeToRenderFrame = 1000.0f / iFrameCount;

		if (bShowFrameStatsInTitle)
		{
			std::wstring sFPS = L"FPS: " + std::to_wstring(iFrameCount);
			std::wstring sAvrTimeToRenderFrame = L"Avr. time to render a frame: " + std::to_wstring(fAvrTimeToRenderFrame);

			std::wstring sWindowTitleText = sMainWindowTitle + L" (" + sFPS + L", " + sAvrTimeToRenderFrame + L")";

			SetWindowText(hMainWindow, sWindowTitleText.c_str());
		}

		iFPS = iFrameCount;
		this->fAvrTimeToRenderFrame = fAvrTimeToRenderFrame;

		iFrameCount = 0;
		fTimeElapsed = gameTimer.getTimeElapsedInSec();
	}
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return SApplication::getApp()->msgProc(hwnd, msg, wParam, lParam);
}

bool SApplication::createMainWindow()
{
	WNDCLASSEX wc;
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = MainWindowProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hApplicationInstance;
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = L"MainWindow";
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
	wc.cbSize        = sizeof(wc);

	if( !RegisterClassEx(&wc) )
	{
		std::wstring sOutputText = L"An error occurred at SApplication::createMainWindow::RegisterClass(). Error code: " + std::to_wstring(GetLastError());
		MessageBox(0, sOutputText.c_str(), L"Error", 0);
		return true;
	}


	RECT R = { 0, 0, iMainWindowWidth, iMainWindowHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int iWidth  = R.right - R.left;
	int iHeight = R.bottom - R.top;


	hMainWindow = CreateWindow(L"MainWindow", sMainWindowTitle.c_str(), 
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, iWidth, iHeight, 0, 0, hApplicationInstance, 0); 
	if( !hMainWindow )
	{
		std::wstring sOutputText = L"An error occurred at SApplication::createMainWindow::CreateWindow(). Error code: " + std::to_wstring(GetLastError());
		MessageBox(0, sOutputText.c_str(), L"Error", 0);
		return true;
	}


	ShowWindow(hMainWindow, SW_SHOW);
	UpdateWindow(hMainWindow);


	SetWindowText(hMainWindow, sMainWindowTitle.c_str());


	return false;
}

bool SApplication::initD3DSecondStage()
{
	if (createSwapChain())
	{
		return true;
	}



	if (createRTVAndDSVDescriptorHeaps())
	{
		return true;
	}



	// Disable alt + enter.

	HRESULT hresult = pFactory->MakeWindowAssociation(hMainWindow, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::initD3DSecondStage::IDXGIFactory4::MakeWindowAssociation()");
		return true;
	}


	return false;
}

bool SApplication::initD3DFirstStage()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

		debugController->EnableDebugLayer();
	}
#endif

	HRESULT hresult;



	// Create DXGI Factory

	hresult = CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::initD3DFirstStage::CreateDXGIFactory1()");
		return true;
	}




	// Get supported hardware display adapter.

	if (getFirstSupportedDisplayAdapter(&pAdapter))
	{
		MessageBox(0, 
			L"An error occurred at SApplication::initD3DFirstStage::getFirstSupportedDisplayAdapter(). Error: Can't find a supported display adapter.",
			L"Error",
			0);

		return true;
	}




	// Create device.

	hresult = D3D12CreateDevice(
		pAdapter.Get(),
		ENGINE_D3D_FEATURE_LEVEL,
		IID_PPV_ARGS(&pDevice));

	if (FAILED(hresult))
	{
		// Try to create device with WARP (software) adapter.

		Microsoft::WRL::ComPtr<IDXGIAdapter> WarpAdapter;
		pFactory->EnumWarpAdapter(IID_PPV_ARGS(&WarpAdapter));

		hresult = D3D12CreateDevice(
			WarpAdapter.Get(),
			ENGINE_D3D_FEATURE_LEVEL,
			IID_PPV_ARGS(&pDevice));

		if (FAILED(hresult))
		{
			SError::showErrorMessageBox(hresult, L"SApplication::initD3DFirstStage::D3D12CreateDevice() (WARP adapter)");
			return true;
		}
		else
		{
			bUsingWARPAdapter = true;
		}
	}




	// Create Fence and descriptor sizes.

	hresult = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::initD3DFirstStage::ID3D12Device::CreateFence()");
		return true;
	}

	iRTVDescriptorSize       = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	iDSVDescriptorSize       = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	iCBVSRVUAVDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);



	if (checkMSAASupport())
	{
		return true;
	}



	if (createCommandObjects())
	{
		return true;
	}



	if (getFirstOutputDisplay(&pOutput))
	{
		MessageBox(0, 
			L"An error occurred at SApplication::initDirect3D::getFirstOutputAdapter(). Error: Can't find any output adapter.",
			L"Error",
			0);

		return true;
	}


	if (getScreenParams(true))
	{
		return true;
	}

	return false;
}

bool SApplication::getFirstSupportedDisplayAdapter(IDXGIAdapter3** ppAdapter)
{
	*ppAdapter = nullptr;

	if (sPreferredDisplayAdapter != L"")
	{
		for (UINT adapterIndex = 0; ; ++adapterIndex)
		{
			IDXGIAdapter3* pAdapter1 = nullptr;

			if (pFactory->EnumAdapters(adapterIndex, reinterpret_cast<IDXGIAdapter**>(&pAdapter1)) == DXGI_ERROR_NOT_FOUND)
			{
				// No more adapters to enumerate.
				break;
			} 



			// Check to see if the adapter supports Direct3D version, but don't create the actual device yet.

			if (SUCCEEDED(D3D12CreateDevice(pAdapter1, ENGINE_D3D_FEATURE_LEVEL, _uuidof(ID3D12Device), nullptr)))
			{
				DXGI_ADAPTER_DESC desc;
				pAdapter1->GetDesc(&desc);

				if (desc.Description == sPreferredDisplayAdapter)
				{
					*ppAdapter = pAdapter1;
					return false;
				}
			}

			pAdapter1->Release();
			pAdapter1 = nullptr;
		}
	}

	for (UINT adapterIndex = 0; ; ++adapterIndex)
	{
		IDXGIAdapter3* pAdapter1 = nullptr;

		if (pFactory->EnumAdapters(adapterIndex, reinterpret_cast<IDXGIAdapter**>(&pAdapter1)) == DXGI_ERROR_NOT_FOUND)
		{
			// No more adapters to enumerate.
			break;
		} 



		// Check to see if the adapter supports Direct3D version, but don't create the actual device yet.

		if (SUCCEEDED(D3D12CreateDevice(pAdapter1, ENGINE_D3D_FEATURE_LEVEL, _uuidof(ID3D12Device), nullptr)))
		{
			*ppAdapter = pAdapter1;
			return false;
		}

		pAdapter1->Release();
		pAdapter1 = nullptr;
	}

	return true;
}

bool SApplication::getFirstOutputDisplay(IDXGIOutput** ppOutput)
{
	*ppOutput = nullptr;

	if (sPreferredOutputAdapter != L"")
	{
		for (UINT outputIndex = 0; ; ++outputIndex)
		{
			IDXGIOutput* pOutput1 = nullptr;

			if (pAdapter->EnumOutputs(outputIndex, &pOutput1) == DXGI_ERROR_NOT_FOUND)
			{
				// No more displays to enumerate.
				break;
			} 



			DXGI_OUTPUT_DESC desc;
			pOutput1->GetDesc(&desc);

			if (desc.DeviceName == sPreferredDisplayAdapter)
			{
				*ppOutput = pOutput1;

				return false;
			}

			pOutput1->Release();
			pOutput1 = nullptr;
		}
	}

	for (UINT outputIndex = 0; ; ++outputIndex)
	{
		IDXGIOutput* pOutput1 = nullptr;

		if (pAdapter->EnumOutputs(outputIndex, &pOutput1) == DXGI_ERROR_NOT_FOUND)
		{
			// No more adapters to enumerate.
			break;
		} 



		*ppOutput = pOutput1;

		return false;
	}

	return true;
}

bool SApplication::checkMSAASupport()
{
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format           = BackBufferFormat;
	msQualityLevels.SampleCount      = MSAA_SampleCount;
	msQualityLevels.Flags            = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	HRESULT hresult = pDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::checkMSAASupport::ID3D12Device::CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS)");
		return true;
	}

	if (msQualityLevels.NumQualityLevels == 0)
	{
		MessageBox(0, L"An error occurred at SApplication::checkMSAASupport::CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS). "
			"Error: NumQualityLevels == 0.", L"Error", 0);
		return true;
	}

	MSAA_Quality = msQualityLevels.NumQualityLevels;

	return false;
}

bool SApplication::createCommandObjects()
{
	// Create Command Queue.

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	HRESULT hresult = pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createCommandObjects::ID3D12Device::CreateCommandQueue()");
		return true;
	}




	// Create Command Allocator.

	hresult = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(pCommandListAllocator.GetAddressOf()));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createCommandObjects::ID3D12Device::CreateCommandAllocator()");
		return true;
	}




	// Create Command List.

	hresult = pDevice->CreateCommandList(
		0, // Create list for 1 GPU. See pDevice->GetNodeCount() and documentation for more info.
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		pCommandListAllocator.Get(), // Associated command allocator
		nullptr,
		IID_PPV_ARGS(pCommandList.GetAddressOf()));

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createCommandObjects::ID3D12Device::CreateCommandList()");
		return true;
	}



	// Start off in a closed state. This is because the first time we refer 
	// to the command list we will Reset() it, and it needs to be closed before
	// calling Reset().
	pCommandList->Close();


	return false;
}

bool SApplication::createSwapChain()
{
	// Release the previous swapchain.
	pSwapChain.Reset();


	DXGI_SWAP_CHAIN_DESC1 desc;
	desc.Width  = iMainWindowWidth;
	desc.Height = iMainWindowHeight;
	desc.Format = BackBufferFormat;
	desc.Stereo = false;

	desc.SampleDesc.Count   = MSAA_Enabled ? MSAA_SampleCount : 1;
	desc.SampleDesc.Quality = MSAA_Enabled ? (MSAA_Quality - 1) : 0;

	desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount        = iSwapChainBufferCount;

	// If the size of the back buffer is not equal to the target output: stretch.
	desc.Scaling    = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode  = DXGI_ALPHA_MODE_UNSPECIFIED;

	if (bVSyncEnabled)
	{
		desc.Flags      = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	}
	else
	{
		desc.Flags      = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	}

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fdesc;
	fdesc.RefreshRate.Numerator   = iRefreshRateNumerator;
	fdesc.RefreshRate.Denominator = iRefreshRateDenominator;
	fdesc.Scaling = iScaling;
	fdesc.ScanlineOrdering = iScanlineOrder;
	fdesc.Windowed = !bFullscreen;

	// Note: Swap chain uses queue to perform flush.
	HRESULT hresult = pFactory->CreateSwapChainForHwnd(pCommandQueue.Get(), hMainWindow, &desc, &fdesc, pOutput.Get(), pSwapChain.GetAddressOf());
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createSwapChain::IDXGIFactory4::CreateSwapChainForHwnd()");
		return true;
	}

	return false;
}

bool SApplication::getScreenParams(bool bApplyResolution)
{
	UINT numModes = 0;

	HRESULT hresult = pOutput->GetDisplayModeList(BackBufferFormat, 0, &numModes, NULL);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::initDirect3D::IDXGIOutput::GetDisplayModeList() (count)");
		return true;
	}

	std::vector<DXGI_MODE_DESC> vDisplayModes(numModes);

	hresult = pOutput->GetDisplayModeList(BackBufferFormat, 0, &numModes, &vDisplayModes[0]);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::initDirect3D::IDXGIOutput::GetDisplayModeList() (list)");
		return true;
	}


	// Save params.
	
	bool bSetResolutionToDefault = true;

	if (bCustomWindowSize)
	{
		// Not default params. Look if this resolution is supported.

		for (int i = vDisplayModes.size() - 1; i > 0; i--)
		{
			if ((vDisplayModes[i].Width == iMainWindowWidth)
				&&
				(vDisplayModes[i].Height == iMainWindowHeight))
			{
				if (bFullscreen && (iMainWindowWidth != vDisplayModes.back().Width) && (iMainWindowHeight != vDisplayModes.back().Height))
				{
					if (vDisplayModes[i].Scaling == DXGI_MODE_SCALING_STRETCHED)
					{
						// Found. Good.
						bSetResolutionToDefault = false;

						iRefreshRateNumerator   = vDisplayModes[i].RefreshRate.Numerator;
						iRefreshRateDenominator = vDisplayModes[i].RefreshRate.Denominator;

						iScanlineOrder          = vDisplayModes[i].ScanlineOrdering;
						iScaling                = vDisplayModes[i].Scaling;

						break;
					}
				}
				else if (vDisplayModes[i].Scaling == vDisplayModes.back().Scaling)
				{
					// Found. Good.
					bSetResolutionToDefault = false;

					iRefreshRateNumerator   = vDisplayModes[i].RefreshRate.Numerator;
					iRefreshRateDenominator = vDisplayModes[i].RefreshRate.Denominator;

					iScanlineOrder          = vDisplayModes[i].ScanlineOrdering;
					iScaling                = vDisplayModes[i].Scaling;

					break;
				}
			}
		}
	}
	
	if (bSetResolutionToDefault)
	{
		// Set default params for this output.

		if (bFullscreen)
		{
			// Use the last element in the list because it has the highest resolution.

			if (bApplyResolution)
			{
				iMainWindowWidth        = vDisplayModes.back().Width;
				iMainWindowHeight       = vDisplayModes.back().Height;
			}

			iRefreshRateNumerator   = vDisplayModes.back().RefreshRate.Numerator;
			iRefreshRateDenominator = vDisplayModes.back().RefreshRate.Denominator;

			iScanlineOrder          = vDisplayModes.back().ScanlineOrdering;
			iScaling                = vDisplayModes.back().Scaling;
		}
		else
		{
			// Find previous element in the list with the same vDisplayModes.back().ScanlineOrdering && vDisplayModes.back().Scaling.

			for (int i = vDisplayModes.size() - 2; i > 0; i--)
			{
				if ((vDisplayModes[i].ScanlineOrdering == vDisplayModes.back().ScanlineOrdering)
					&&
					(vDisplayModes[i].Scaling == vDisplayModes.back().Scaling))
				{
					if (bApplyResolution)
					{
						iMainWindowWidth        = vDisplayModes[i].Width;
						iMainWindowHeight       = vDisplayModes[i].Height;
					}

					iRefreshRateNumerator   = vDisplayModes[i].RefreshRate.Numerator;
					iRefreshRateDenominator = vDisplayModes[i].RefreshRate.Denominator;

					iScanlineOrder          = vDisplayModes[i].ScanlineOrdering;
					iScaling                = vDisplayModes[i].Scaling;

					break;
				}
			}
		}
	}

	

	return false;
}

bool SApplication::createRTVAndDSVDescriptorHeaps()
{
	// RTV

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = iSwapChainBufferCount;
	rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask       = 0;

	HRESULT hresult = pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(pRTVHeap.GetAddressOf()));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createRTVAndDSVDescriptorHeaps::ID3D12Device::CreateDescriptorHeap() (RTV)");
		return true;
	}




	// DSV

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask       = 0;

	hresult = pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(pDSVHeap.GetAddressOf()));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createRTVAndDSVDescriptorHeaps::ID3D12Device::CreateDescriptorHeap() (DSV)");
		return true;
	}

	return false;
}

bool SApplication::createCBVDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask       = 0;

	HRESULT hresult = pDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&pCBVHeap));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createCBVDescriptorHeap::ID3D12Device::CreateDescriptorHeap() (CBV)");
		return true;
	}

	return false;
}

void SApplication::createConstantBuffer()
{
	// Create 1 constant buffer.
	pObjectConstantBuffer = std::make_unique<SUploadBuffer<SObjectConstants>>(pDevice.Get(), 1, true);


	UINT iObjectConstantBufferSizeInBytes = SMath::makeMultipleOf256(sizeof(SObjectConstants));


	D3D12_GPU_VIRTUAL_ADDRESS constantBufferAddress = pObjectConstantBuffer->getResource()->GetGPUVirtualAddress();
	// Offset to the ith object constant buffer in the buffer.
	int iConstantBufferIndex = 0;
	constantBufferAddress += iConstantBufferIndex * iObjectConstantBufferSizeInBytes;


	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = constantBufferAddress;
	cbvDesc.SizeInBytes = iObjectConstantBufferSizeInBytes;


	pDevice->CreateConstantBufferView(&cbvDesc, pCBVHeap->GetCPUDescriptorHandleForHeapStart());
}

bool SApplication::createRootSignature()
{
	// The root signature defines the resources the shader programs expect.

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];


	// Create a single descriptor table of CBVs.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);


	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(1, slotRootParameter, 0, nullptr, 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


	// Create a root signature with a single slot that points to a descriptor range consisting of a single constant buffer.
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hresult = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());


	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createRootSignature::D3D12SerializeRootSignature()");
		return true;
	}


	hresult = pDevice->CreateRootSignature(
		0,
		serializedRootSignature->GetBufferPointer(),
		serializedRootSignature->GetBufferSize(),
		IID_PPV_ARGS(&pRootSignature)
	);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createRootSignature::ID3D12Device::CreateRootSignature()");
		return true;
	}


	return false;
}

bool SApplication::createShadersAndInputLayout()
{
	pVSByteCode = SGeometry::compileShader(L"shaders/color.hlsl", nullptr, "VS", "vs_5_0");
	pPSByteCode = SGeometry::compileShader(L"shaders/color.hlsl", nullptr, "PS", "ps_5_0");

	if (pVSByteCode == nullptr || pPSByteCode == nullptr)
	{
		return true;
	}

	vInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	return false;
}

bool SApplication::createBoxGeometry()
{
	std::array<SVertex, 8> vVertices =
	{
		SVertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White) }),
		SVertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
		SVertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) }),
		SVertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) }),
		SVertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue) }),
		SVertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow) }),
		SVertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) }),
		SVertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta) })
	};

	std::array<std::uint16_t, 36> vIndices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};


	const UINT iVertexBufferSizeInBytes = static_cast<UINT>(vVertices.size() * sizeof(SVertex));
	const UINT iIndexBufferSizeInBytes  = static_cast<UINT>(vIndices.size() * sizeof(std::uint16_t));


	pBoxGeometry = std::make_unique<SMeshGeometry>();
	pBoxGeometry->sMeshName = L"Cube Geometry";



	HRESULT hresult = D3DCreateBlob(iVertexBufferSizeInBytes, &pBoxGeometry->pVertexBufferCPU);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createBoxGeometry::D3DCreateBlob() (VB)");
		return true;
	}

	std::memcpy(pBoxGeometry->pVertexBufferCPU->GetBufferPointer(), vVertices.data(), iVertexBufferSizeInBytes);



	hresult = D3DCreateBlob(iIndexBufferSizeInBytes, &pBoxGeometry->pIndexBufferCPU);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createBoxGeometry::D3DCreateBlob() (IB)");
		return true;
	}

	std::memcpy(pBoxGeometry->pIndexBufferCPU->GetBufferPointer(), vIndices.data(), iIndexBufferSizeInBytes);



	pBoxGeometry->pVertexBufferGPU = SGeometry::createDefaultBuffer(pDevice.Get(), pCommandList.Get(), vVertices.data(),
		iVertexBufferSizeInBytes, pBoxGeometry->pVertexBufferUploader);

	pBoxGeometry->pIndexBufferGPU = SGeometry::createDefaultBuffer(pDevice.Get(), pCommandList.Get(), vIndices.data(),
		iIndexBufferSizeInBytes, pBoxGeometry->pIndexBufferUploader);


	pBoxGeometry->iVertexByteStride = sizeof(SVertex);
	pBoxGeometry->iVertexBufferSizeInBytes = iVertexBufferSizeInBytes;
	pBoxGeometry->iIndexBufferSizeInBytes  = iIndexBufferSizeInBytes;
	pBoxGeometry->indexFormat = DXGI_FORMAT_R16_UINT;
	

	SSubmeshGeometry submesh;
	submesh.iIndexCount = static_cast<UINT>(vIndices.size());
	submesh.iStartIndexLocation = 0;
	submesh.iBaseVertexLocation = 0;


	pBoxGeometry->mDrawArgs[L"Cube"] = submesh;


	return false;
}

bool SApplication::createPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	memset(&psoDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	psoDesc.InputLayout = { vInputLayout.data(), static_cast<UINT>(vInputLayout.size()) };
	psoDesc.pRootSignature = pRootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(pVSByteCode->GetBufferPointer()),
		pVSByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(pPSByteCode->GetBufferPointer()),
		pPSByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState   = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState        = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask        = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets  = 1;
	psoDesc.RTVFormats[0]     = BackBufferFormat;
	psoDesc.SampleDesc.Count  = MSAA_Enabled ? MSAA_SampleCount : 1;
	psoDesc.SampleDesc.Quality = MSAA_Enabled ? (MSAA_Quality - 1) : 0;
	psoDesc.DSVFormat         = DepthStencilFormat;

	HRESULT hresult = pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPSO));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::createPSO::ID3D12Device::CreateGraphicsPipelineState()");
		return true;
	}

	return false;
}

ID3D12Resource* SApplication::getCurrentBackBufferResource() const
{
	return pSwapChainBuffer[iCurrentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE SApplication::getCurrentBackBufferViewHandle() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		pRTVHeap->GetCPUDescriptorHandleForHeapStart(),
		iCurrentBackBuffer,
		iRTVDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE SApplication::getDepthStencilViewHandle() const
{
	return pDSVHeap->GetCPUDescriptorHandleForHeapStart();
}

SApplication::SApplication(HINSTANCE hInstance)
{
	hApplicationInstance = hInstance;
	pApp           = this;
}

SApplication::~SApplication()
{
	if(bInitCalled)
	{
		// Wait for the GPU because it can still reference resources that we will delete (after destructor).
		flushCommandQueue();

		if (bFullscreen)
		{
			// From docs: "Before releasing a swap chain, first switch to windowed mode".
			pSwapChain->SetFullscreenState(false, NULL);
		}
	}
}

bool SApplication::init()
{
	// Create Output and ask it about screen resolution.
	if (initD3DFirstStage())
	{
		return true;
	}

	// Create window with supported resolution.
	if (createMainWindow())
	{
		return true;
	}

	if (initD3DSecondStage())
	{
		return true;
	}

	bInitCalled = true;

	// Do the initial resize code.
	onResize();

	HRESULT hresult = pCommandList->Reset(pCommandListAllocator.Get(), nullptr);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::init::ID3D12GraphicsCommandList::Reset()");
		return true;
	}

	if (createCBVDescriptorHeap())
	{
		return true;
	}

	createConstantBuffer();

	if (createRootSignature())
	{
		return true;
	}

	if (createShadersAndInputLayout())
	{
		return true;
	}

	if (createBoxGeometry())
	{
		return true;
	}

	if (createPSO())
	{
		return true;
	}

	// Execute init commands.

	hresult = pCommandList->Close();
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SApplication::init::ID3D12GraphicsCommandList::Close()");
		return true;
	}

	ID3D12CommandList* vCommandListsToExecute[] = { pCommandList.Get() };
	pCommandQueue->ExecuteCommandLists(_countof(vCommandListsToExecute), vCommandListsToExecute);

	
	// Wait for all command to finish.

	if (flushCommandQueue())
	{
		return true;
	}

	return false;
}

LRESULT SApplication::msgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
	{
		// Save new window size.
		iMainWindowWidth  = LOWORD(lParam);
		iMainWindowHeight = HIWORD(lParam);

		if (bInitCalled)
		{
			if( wParam == SIZE_MINIMIZED )
			{
				bWindowMaximized = false;
				bWindowMinimized = true;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				bWindowMaximized = true;
				bWindowMinimized = false;
				onResize();
			}
			else if( wParam == SIZE_RESTORED )
			{
				if (bWindowMinimized)
				{
					bWindowMinimized = false;
					onResize();
				}
				else if (bWindowMaximized)
				{
					bWindowMaximized = false;
					onResize();
				}
				else if(bResizingMoving == false)
				{
					// API call such as SetWindowPos or pSwapChain->SetFullscreenState.
					onResize();
				}
			}
		}

		return 0;
	}
	case WM_ENTERSIZEMOVE:
	{
		// The user grabs the resize bars.

		bResizingMoving = true;

		gameTimer.pause();

		return 0;
	}
	case WM_EXITSIZEMOVE:
	{
		// The user releases the resize bars.

		bResizingMoving = false;

		onResize();

		gameTimer.unpause();

		return 0;
	}
	case WM_MENUCHAR:
	{
		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
		// Don't make *beep* sound when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);
	}
	case WM_GETMINMAXINFO:
	{
		// Catch this message so to prevent the window from becoming too small.
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;
	}
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_XBUTTONDOWN:
	{
		onMouseDown(SMouseKey(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}
	case WM_LBUTTONUP:
	{
		onMouseUp(SMouseKey(SMB_LEFT, wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}
	case WM_MBUTTONUP:
	{
		onMouseUp(SMouseKey(SMB_MIDDLE, wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}
	case WM_RBUTTONUP:
	{
		onMouseUp(SMouseKey(SMB_RIGHT, wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}
	case WM_XBUTTONUP:
	{
		onMouseUp(SMouseKey(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		onMouseMove(SMouseKey(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}
	case WM_KEYDOWN:
	{

		return 0;
	}
	case WM_KEYUP:
	{
		if(wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}

		return 0;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int SApplication::run()
{
	if (bInitCalled)
	{
		MSG msg = {0};

		gameTimer.reset();

		bRunCalled = true;

		STimer frameTimer;

		while(msg.message != WM_QUIT)
		{
			if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			else
			{
				if (fFPSLimit >= 1.0f)
				{
					frameTimer.start();
				}


				gameTimer.tick();

				if (bCallTick)
				{
					onTick();
				}

				updateViewMatrix();
				draw();

				calculateFrameStats();


				if (fFPSLimit >= 1.0f)
				{
					float fTimeToRenderFrame = frameTimer.getElapsedTimeInMS();

					if (fDelayBetweenFramesInMS > fTimeToRenderFrame)
					{
						Sleep(static_cast<unsigned long>(fDelayBetweenFramesInMS - fTimeToRenderFrame));
					}
				}
			}
		}

		return static_cast<int>(msg.wParam);
	}
	else
	{
		MessageBox(0, L"An error occurred at SApplication::run(). Error: init() should be called first.", L"Error", 0);
		return 1;
	}
}