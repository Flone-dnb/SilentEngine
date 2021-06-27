// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SProfiler.h"

// Custom
#include "SilentEngine/Public/SApplication/SApplication.h"

#if defined(DEBUG) || defined(_DEBUG)
// for showOnScreen()
#include <sstream>
#include "SilentEngine/Public/GUI/SGUILayout/SGUILayout.h"
#include "SilentEngine/Public/GUI/SGUISimpleText/SGUISimpleText.h"
#endif

SProfiler::SProfiler(SApplication* pApp)
{
	this->pApp = pApp;

#if defined(DEBUG) || defined(_DEBUG)
	lastFrameStats = { 0 };
#endif
}

bool SProfiler::getTimeElapsedFromStart(float* fTimeInSec) const
{
	return pApp->getTimeElapsedFromStart(fTimeInSec);
}

bool SProfiler::getFPS(int* iFPS) const
{
	return pApp->getFPS(iFPS);
}

bool SProfiler::getTimeToRenderFrame(float* fTimeInMS) const
{
	return pApp->getTimeToRenderFrame(fTimeInMS);
}

#if defined(DEBUG) || defined(_DEBUG)
SFrameStats SProfiler::getFrameStats()
{
	std::lock_guard<std::mutex> lock(mtxFrameStats);

	return lastFrameStats;
}

bool SProfiler::initNeededGUIObjects()
{
	const float fNameWidth = 0.5f;
	const float fItemHeight = 0.06f;

	const float fValueWidth = 0.2f;

	const float fNameXPos = 0.15f;
	const float fValueXPos = 0.7f;

	const float fYStartPos = 0.3f;

	// Register FPS label.
	pFPSText = new SGUISimpleText("profiler GUI: FPS");
	pFPSText->bIsSystemObject = true;
	pFPSText->setFont(pApp->sPathToDefaultFont);
	pFPSText->setZLayer(INT_MAX);
	pFPSText->setDrawTextOutline(true, SVector());
	pFPSText->setText(L"FPS: 0");
	pFPSText->setCustomOrigin(SVector(0.0f, 0.0f));
	pFPSText->setPosition(SVector(fNameXPos, 0.1f));
	pFPSText->setSizeToKeep(SVector(0.1f, fItemHeight));
	pApp->registerGUIObject(pFPSText, false);

	
	// Fill names.

	std::vector<std::wstring> vText = {
		L"Time spent on Windows messages (ms):",
		L"Time spent on user Tick function (ms):",
		L"Time spent on 3D audio update (ms):",
		L"Time spent waiting for GPU in 'update()' (ms):",
		L"Time spent on 'update()' (ms):",
		L"Time spent on CPU 'draw()' (ms):",
		L"Time spent on FPS calc (ms):",
		L"Time spent in FPS limit sleep (ms):",
		L"[i] Time spent on user physics tick (ms):"
	};

	float fCurrentPosY = fYStartPos;

	pLayoutNames = new SGUILayout("Layout", fNameWidth, 0.6f, SLayoutType::SLT_VERTICAL, false);
	pLayoutNames->setPosition(SVector(fNameXPos + fNameWidth / 2, 0.5f));
	pApp->registerGUIObject(pLayoutNames, false);

	pLayoutValues = new SGUILayout("Layout", fValueWidth, 0.6f, SLayoutType::SLT_VERTICAL, false);
	pLayoutValues->setPosition(SVector(fValueXPos + fValueWidth / 2, 0.5f));
	pApp->registerGUIObject(pLayoutValues, false);

	for (size_t i = 0; i < vText.size(); i++)
	{
		SGUISimpleText* pItem = new SGUISimpleText("profiler GUI: name");
		pItem->bIsSystemObject = true;
		pItem->setFont(pApp->sPathToDefaultFont);
		pItem->setZLayer(INT_MAX);
		pItem->setDrawTextOutline(true, SVector());
		pItem->setText(vText[i]);
		pApp->registerGUIObject(pItem, true);

		pLayoutNames->addChild(pItem, 1);
		pItem->setVisible(true);

		fCurrentPosY += fItemHeight;
	}

	fCurrentPosY = fYStartPos;

	for (size_t i = 0; i < vText.size(); i++)
	{
		SGUISimpleText* pItem = new SGUISimpleText("profiler GUI: value");
		pItem->bIsSystemObject = true;
		pItem->setFont(pApp->sPathToDefaultFont);
		pItem->setZLayer(INT_MAX);
		pItem->setDrawTextOutline(true, SVector());
		pItem->setText(L"0.0");
		pApp->registerGUIObject(pItem, true);

		pLayoutValues->addChild(pItem, 1);
		pItem->setVisible(true);

		fCurrentPosY += fItemHeight;
	}
	
	return false;
}

void SProfiler::showOnScreen(bool bShow, float fDataUpdateIntervalInMS)
{
	bFrameStatsShownOnScreen = bShow;
	fUpdateGUIIntervalInMS = fDataUpdateIntervalInMS;

	updateFrameStatsOnScreen();

	pLayoutNames->setVisible(bShow);
	pLayoutValues->setVisible(bShow);
	pFPSText->setVisible(bShow);
}
bool SProfiler::isShownOnScreen() const
{
	return bFrameStatsShownOnScreen;
}
#endif

float SProfiler::getTimeSpentWaitingForGPUBetweenFramesInMS()
{
	return fTimeSpentWaitingForGPUBetweenFramesInMS;
}

bool SProfiler::getLastFrameDrawCallCount(unsigned long long* iDrawCallCount) const
{
	return pApp->getLastFrameDrawCallCount(iDrawCallCount);
}

bool SProfiler::getVideoMemoryUsageInBytesOfCurrentDisplayAdapter(unsigned long long* pSizeInBytes)
{
	return pApp->getVideoMemoryUsageInBytesOfCurrentDisplayAdapter(pSizeInBytes);
}

#if defined(DEBUG) || defined(_DEBUG)
void SProfiler::setFrameStats(const SFrameStats& frameStats)
{
	std::lock_guard<std::mutex> lock(mtxFrameStats);

	lastFrameStats = frameStats;

	updateFrameStatsOnScreen();
}

void SProfiler::updateFrameStatsOnScreen()
{
	if (bFrameStatsShownOnScreen)
	{
		float fTime = 0.0f;
		getTimeElapsedFromStart(&fTime);
		fTime *= 1000.0f; // to ms

		if (fTime - fUpdateGUIIntervalInMS > fLastCheckTime)
		{
			// Update values.

			int iFPS = 0;
			getFPS(&iFPS);

			pFPSText->setText(L"FPS: " + std::to_wstring(iFPS));

			const size_t iPrecision = 1;

			std::vector<SLayoutChild>* pLayoutChilds = pLayoutValues->getChilds();

			dynamic_cast<SGUISimpleText*>(pLayoutChilds->operator[](0).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOnWindowsMessagesInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pLayoutChilds->operator[](1).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOnUserOnTickFunctionInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pLayoutChilds->operator[](2).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOn3DAudioUpdateInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pLayoutChilds->operator[](3).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentWaitingForGPUInUpdateInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pLayoutChilds->operator[](4).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOnUpdateInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pLayoutChilds->operator[](5).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOnCPUDrawInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pLayoutChilds->operator[](6).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOnFPSCalcInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pLayoutChilds->operator[](7).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentInFPSLimitSleepInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pLayoutChilds->operator[](8).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOnUserPhysicsTickFunctionInMS, iPrecision));
			
			fLastCheckTime = fTime;
		}
	}
}

std::wstring SProfiler::floatToWstring(float fValue, size_t iPrecision)
{
	std::wstringstream stream;
	stream << std::fixed << std::setprecision(iPrecision) << fValue;
	return stream.str();
}
#endif