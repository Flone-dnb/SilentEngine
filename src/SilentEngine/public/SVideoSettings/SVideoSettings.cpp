// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SVideoSettings.h"

#include "SilentEngine/public/SApplication/SApplication.h"

SVideoSettings::SVideoSettings(SApplication* pApp)
{
	this->pApp = pApp;
}

void SVideoSettings::setFPSLimit(float fFPSLimit)
{
	pApp->setFPSLimit(fFPSLimit);
}

void SVideoSettings::setMSAAEnabled(bool bEnable)
{
	pApp->setMSAAEnabled(bEnable);
}

bool SVideoSettings::setMSAASampleCount(MSAASampleCount sampleCount)
{
	return pApp->setMSAASampleCount(sampleCount);
}

bool SVideoSettings::isMSAAEnabled()
{
	return pApp->isMSAAEnabled();
}

MSAASampleCount SVideoSettings::getMSAASampleCount()
{
	return pApp->getMSAASampleCount();
}

bool SVideoSettings::setScreenResolution(SScreenResolution screenResolution)
{
	return pApp->setScreenResolution(screenResolution);
}

bool SVideoSettings::setFullscreenWithCurrentResolution(bool bFullscreen)
{
	return pApp->setFullscreenWithCurrentResolution(bFullscreen);
}

bool SVideoSettings::getCurrentScreenResolution(SScreenResolution* pScreenResolution)
{
	return pApp->getCurrentScreenResolution(pScreenResolution);
}

bool SVideoSettings::isFullscreen()
{
	return pApp->isFullscreen();
}

float SVideoSettings::getScreenAspectRatio()
{
	return pApp->getScreenAspectRatio();
}

void SVideoSettings::setBackBufferFillColor(SVector vColor)
{
	pApp->setBackBufferFillColor(vColor);
}

void SVideoSettings::setEnableWireframeMode(bool bEnable)
{
	pApp->setEnableWireframeMode(bEnable);
}

void SVideoSettings::setEnableBackfaceCulling(bool bEnable)
{
	pApp->setEnableBackfaceCulling(bEnable);
}

SVector SVideoSettings::getBackBufferFillColor()
{
	return pApp->getBackBufferFillColor();
}

bool SVideoSettings::isWireframeModeEnabled()
{
	return pApp->isWireframeModeEnabled();
}

bool SVideoSettings::isBackfaceCullingEnabled()
{
	return pApp->isBackfaceCullingEnabled();
}

std::vector<std::wstring> SVideoSettings::getSupportedDisplayAdapters()
{
	return pApp->getSupportedDisplayAdapters();
}

std::wstring SVideoSettings::getCurrentDisplayAdapter()
{
	return pApp->getCurrentDisplayAdapter();
}

bool SVideoSettings::getVideoMemorySizeInBytesOfCurrentDisplayAdapter(unsigned long long* pSizeInBytes)
{
    return pApp->getVideoMemorySizeInBytesOfCurrentDisplayAdapter(pSizeInBytes);
}

std::vector<std::wstring> SVideoSettings::getOutputDisplaysOfCurrentDisplayAdapter()
{
	return pApp->getOutputDisplaysOfCurrentDisplayAdapter();
}

std::wstring SVideoSettings::getCurrentOutputDisplay()
{
	return pApp->getCurrentOutputDisplay();
}

float SVideoSettings::getCurrentOutputDisplayRefreshRate()
{
	return pApp->getCurrentOutputDisplayRefreshRate();
}

bool SVideoSettings::getAvailableScreenResolutionsOfCurrentOutputDisplay(std::vector<SScreenResolution>* vResolutions)
{
	return pApp->getAvailableScreenResolutionsOfCurrentOutputDisplay(vResolutions);
}

bool SVideoSettings::setCameraNearClipPlane(float fNearClipPlaneValue)
{
	return pApp->setNearClipPlane(fNearClipPlaneValue);
}

bool SVideoSettings::setCameraFarClipPlane(float fFarClipPlaneValue)
{
	return pApp->setFarClipPlane(fFarClipPlaneValue);
}

float SVideoSettings::getCameraNearClipPlaneValue()
{
	return pApp->getNearClipPlaneValue();
}

float SVideoSettings::getCameraFarClipPlaneValue()
{
	return pApp->getFarClipPlaneValue();
}

bool SVideoSettings::setInitPreferredDisplayAdapter(std::wstring sPreferredDisplayAdapter)
{
	return pApp->setInitPreferredDisplayAdapter(sPreferredDisplayAdapter);
}

bool SVideoSettings::setInitPreferredOutputAdapter(std::wstring sPreferredOutputAdapter)
{
	return pApp->setInitPreferredOutputAdapter(sPreferredOutputAdapter);
}

bool SVideoSettings::setInitFullscreen(bool bFullscreen)
{
	return pApp->setInitFullscreen(bFullscreen);
}

bool SVideoSettings::setInitEnableVSync(bool bEnable)
{
	return pApp->setInitEnableVSync(bEnable);
}