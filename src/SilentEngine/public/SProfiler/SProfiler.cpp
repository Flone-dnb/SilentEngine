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

	lastFrameStats = { 0 };
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
	// Register layouts.

	pNameLayout = new SGUILayout("profiler name layout", 0.4f, 0.5f, SLayoutType::SLT_VERTICAL, false);
	pNameLayout->bIsSProfilerObject = true;
	pNameLayout->setPosition(SVector(0.4f, 0.5f));
	pNameLayout->setZLayer(INT_MAX);

	pValueLayout = new SGUILayout("profiler value layout", 0.1f, 0.5f, SLayoutType::SLT_VERTICAL, false);
	pValueLayout->bIsSProfilerObject = true;
	pValueLayout->setPosition(SVector(0.7f, 0.5f));
	pValueLayout->setZLayer(INT_MAX);

	pApp->registerGUIObject(pNameLayout, false);
	pApp->registerGUIObject(pValueLayout, false);


	// Register FPS label.
	pFPSText = new SGUISimpleText("profiler GUI: fFPS");
	pFPSText->bIsSProfilerObject = true;
	pFPSText->setFont(pApp->sPathToDefaultFont);
	pFPSText->setZLayer(INT_MAX);
	pFPSText->setDrawTextOutline(true, SVector());
	pFPSText->setText(L"FPS: 0");
	pFPSText->setCustomOrigin(SVector(0.0f, 0.0f));
	pFPSText->setPosition(SVector(0.2f, 0.20f));
	pFPSText->setSizeToKeep(SVector(0.1f, 0.06f));
	pApp->registerGUIObject(pFPSText, false);


	// Register names.
	
	// fTimeSpentOnWindowsMessagesInMS
	SGUISimpleText* pTimeSpentOnWindowsMessages = new SGUISimpleText("profiler GUI: fTimeSpentOnWindowsMessagesInMS");
	pTimeSpentOnWindowsMessages->bIsSProfilerObject = true;
	pTimeSpentOnWindowsMessages->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOnWindowsMessages->setZLayer(INT_MAX);
	pTimeSpentOnWindowsMessages->setDrawTextOutline(true, SVector());
	pTimeSpentOnWindowsMessages->setText(L"Time spent on Windows messages (ms):");
	pApp->registerGUIObject(pTimeSpentOnWindowsMessages, true);
	pNameLayout->addChild(pTimeSpentOnWindowsMessages, 1);
	pTimeSpentOnWindowsMessages->setVisible(true);

	// fTimeSpentOnUserOnTickFunctionInMS
	SGUISimpleText* pTimeSpentOnUserOnTickFunction = new SGUISimpleText("profiler GUI: fTimeSpentOnUserOnTickFunctionInMS");
	pTimeSpentOnUserOnTickFunction->bIsSProfilerObject = true;
	pTimeSpentOnUserOnTickFunction->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOnUserOnTickFunction->setZLayer(INT_MAX);
	pTimeSpentOnUserOnTickFunction->setDrawTextOutline(true, SVector());
	pTimeSpentOnUserOnTickFunction->setText(L"Time spent on user Tick function (ms):");
	pApp->registerGUIObject(pTimeSpentOnUserOnTickFunction, true);
	pNameLayout->addChild(pTimeSpentOnUserOnTickFunction, 1);
	pTimeSpentOnUserOnTickFunction->setVisible(true);

	// fTimeSpentOn3DAudioUpdateInMS
	SGUISimpleText* pTimeSpentOn3DAudioUpdate = new SGUISimpleText("profiler GUI: fTimeSpentOn3DAudioUpdateInMS");
	pTimeSpentOn3DAudioUpdate->bIsSProfilerObject = true;
	pTimeSpentOn3DAudioUpdate->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOn3DAudioUpdate->setZLayer(INT_MAX);
	pTimeSpentOn3DAudioUpdate->setDrawTextOutline(true, SVector());
	pTimeSpentOn3DAudioUpdate->setText(L"Time spent on 3D audio update (ms):");
	pApp->registerGUIObject(pTimeSpentOn3DAudioUpdate, true);
	pNameLayout->addChild(pTimeSpentOn3DAudioUpdate, 1);
	pTimeSpentOn3DAudioUpdate->setVisible(true);

	// fTimeSpentWaitingForGPUInUpdateInMS
	SGUISimpleText* pTimeSpentWaitingForGPUInUpdate = new SGUISimpleText("profiler GUI: fTimeSpentWaitingForGPUInUpdateInMS");
	pTimeSpentWaitingForGPUInUpdate->bIsSProfilerObject = true;
	pTimeSpentWaitingForGPUInUpdate->setFont(pApp->sPathToDefaultFont);
	pTimeSpentWaitingForGPUInUpdate->setZLayer(INT_MAX);
	pTimeSpentWaitingForGPUInUpdate->setDrawTextOutline(true, SVector());
	pTimeSpentWaitingForGPUInUpdate->setText(L"Time spent waiting for GPU in 'update()' (ms):");
	pApp->registerGUIObject(pTimeSpentWaitingForGPUInUpdate, true);
	pNameLayout->addChild(pTimeSpentWaitingForGPUInUpdate, 1);
	pTimeSpentWaitingForGPUInUpdate->setVisible(true);

	// fTimeSpentOnUpdateInMS
	SGUISimpleText* pTimeSpentOnUpdate = new SGUISimpleText("profiler GUI: fTimeSpentOnUpdateInMS");
	pTimeSpentOnUpdate->bIsSProfilerObject = true;
	pTimeSpentOnUpdate->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOnUpdate->setZLayer(INT_MAX);
	pTimeSpentOnUpdate->setDrawTextOutline(true, SVector());
	pTimeSpentOnUpdate->setText(L"Time spent on 'update()' (ms):");
	pApp->registerGUIObject(pTimeSpentOnUpdate, true);
	pNameLayout->addChild(pTimeSpentOnUpdate, 1);
	pTimeSpentOnUpdate->setVisible(true);

	// fTimeSpentOnCPUDrawInMS
	SGUISimpleText* pTimeSpentOnCPUDraw = new SGUISimpleText("profiler GUI: fTimeSpentOnCPUDrawInMS");
	pTimeSpentOnCPUDraw->bIsSProfilerObject = true;
	pTimeSpentOnCPUDraw->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOnCPUDraw->setZLayer(INT_MAX);
	pTimeSpentOnCPUDraw->setDrawTextOutline(true, SVector());
	pTimeSpentOnCPUDraw->setText(L"Time spent on CPU 'draw()' (ms):");
	pApp->registerGUIObject(pTimeSpentOnCPUDraw, true);
	pNameLayout->addChild(pTimeSpentOnCPUDraw, 1);
	pTimeSpentOnCPUDraw->setVisible(true);

	// fTimeSpentOnFPSCalcInMS
	SGUISimpleText* pTimeSpentOnFPSCalc = new SGUISimpleText("profiler GUI: fTimeSpentOnFPSCalcInMS");
	pTimeSpentOnFPSCalc->bIsSProfilerObject = true;
	pTimeSpentOnFPSCalc->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOnFPSCalc->setZLayer(INT_MAX);
	pTimeSpentOnFPSCalc->setDrawTextOutline(true, SVector());
	pTimeSpentOnFPSCalc->setText(L"Time spent on FPS calc (ms):");
	pApp->registerGUIObject(pTimeSpentOnFPSCalc, true);
	pNameLayout->addChild(pTimeSpentOnFPSCalc, 1);
	pTimeSpentOnFPSCalc->setVisible(true);

	// fTimeSpentInFPSLimitSleepInMS
	SGUISimpleText* pTimeSpentInFPSLimitSleep = new SGUISimpleText("profiler GUI: fTimeSpentInFPSLimitSleepInMS");
	pTimeSpentInFPSLimitSleep->bIsSProfilerObject = true;
	pTimeSpentInFPSLimitSleep->setFont(pApp->sPathToDefaultFont);
	pTimeSpentInFPSLimitSleep->setZLayer(INT_MAX);
	pTimeSpentInFPSLimitSleep->setDrawTextOutline(true, SVector());
	pTimeSpentInFPSLimitSleep->setText(L"Time spent in FPS limit sleep (ms):");
	pApp->registerGUIObject(pTimeSpentInFPSLimitSleep, true);
	pNameLayout->addChild(pTimeSpentInFPSLimitSleep, 1);
	pTimeSpentInFPSLimitSleep->setVisible(true);

	// fTimeSpentOnUserPhysicsTickFunctionInMS
	SGUISimpleText* pTimeSpentOnUserOnPhysicsTickFunction = new SGUISimpleText("profiler GUI: fTimeSpentOnUserPhysicsTickFunctionInMS");
	pTimeSpentOnUserOnPhysicsTickFunction->bIsSProfilerObject = true;
	pTimeSpentOnUserOnPhysicsTickFunction->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOnUserOnPhysicsTickFunction->setZLayer(INT_MAX);
	pTimeSpentOnUserOnPhysicsTickFunction->setDrawTextOutline(true, SVector());
	pTimeSpentOnUserOnPhysicsTickFunction->setText(L"[i] Time spent on user physics tick (ms):");
	pApp->registerGUIObject(pTimeSpentOnUserOnPhysicsTickFunction, true);
	pNameLayout->addChild(pTimeSpentOnUserOnPhysicsTickFunction, 1);
	pTimeSpentOnUserOnPhysicsTickFunction->setVisible(true);


	// Register values.

	// fTimeSpentOnWindowsMessagesInMS
	SGUISimpleText* pTimeSpentOnWindowsMessagesValue = new SGUISimpleText("profiler GUI: fTimeSpentOnWindowsMessagesInMS (value)");
	pTimeSpentOnWindowsMessagesValue->bIsSProfilerObject = true;
	pTimeSpentOnWindowsMessagesValue->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOnWindowsMessagesValue->setZLayer(INT_MAX);
	pTimeSpentOnWindowsMessagesValue->setDrawTextOutline(true, SVector());
	pTimeSpentOnWindowsMessagesValue->setText(L"Time spent on Windows messages (ms):");
	pApp->registerGUIObject(pTimeSpentOnWindowsMessagesValue, true);
	pValueLayout->addChild(pTimeSpentOnWindowsMessagesValue, 1);
	pTimeSpentOnWindowsMessagesValue->setVisible(true);

	// fTimeSpentOnUserOnTickFunctionInMS
	SGUISimpleText* pTimeSpentOnUserOnTickFunctionValue = new SGUISimpleText("profiler GUI: fTimeSpentOnUserOnTickFunctionInMS (value)");
	pTimeSpentOnUserOnTickFunctionValue->bIsSProfilerObject = true;
	pTimeSpentOnUserOnTickFunctionValue->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOnUserOnTickFunctionValue->setZLayer(INT_MAX);
	pTimeSpentOnUserOnTickFunctionValue->setDrawTextOutline(true, SVector());
	pTimeSpentOnUserOnTickFunctionValue->setText(L"Time spent on user Tick function (ms):");
	pApp->registerGUIObject(pTimeSpentOnUserOnTickFunctionValue, true);
	pValueLayout->addChild(pTimeSpentOnUserOnTickFunctionValue, 1);
	pTimeSpentOnUserOnTickFunctionValue->setVisible(true);

	// fTimeSpentOn3DAudioUpdateInMS
	SGUISimpleText* pTimeSpentOn3DAudioUpdateValue = new SGUISimpleText("profiler GUI: fTimeSpentOn3DAudioUpdateInMS (value)");
	pTimeSpentOn3DAudioUpdateValue->bIsSProfilerObject = true;
	pTimeSpentOn3DAudioUpdateValue->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOn3DAudioUpdateValue->setZLayer(INT_MAX);
	pTimeSpentOn3DAudioUpdateValue->setDrawTextOutline(true, SVector());
	pTimeSpentOn3DAudioUpdateValue->setText(L"Time spent on 3D audio update (ms):");
	pApp->registerGUIObject(pTimeSpentOn3DAudioUpdateValue, true);
	pValueLayout->addChild(pTimeSpentOn3DAudioUpdateValue, 1);
	pTimeSpentOn3DAudioUpdateValue->setVisible(true);

	// fTimeSpentWaitingForGPUInUpdateInMS
	SGUISimpleText* pTimeSpentWaitingForGPUInUpdateValue = new SGUISimpleText("profiler GUI: fTimeSpentWaitingForGPUInUpdateInMS (value)");
	pTimeSpentWaitingForGPUInUpdateValue->bIsSProfilerObject = true;
	pTimeSpentWaitingForGPUInUpdateValue->setFont(pApp->sPathToDefaultFont);
	pTimeSpentWaitingForGPUInUpdateValue->setZLayer(INT_MAX);
	pTimeSpentWaitingForGPUInUpdateValue->setDrawTextOutline(true, SVector());
	pTimeSpentWaitingForGPUInUpdateValue->setText(L"Time spent waiting for GPU in 'update()' (ms):");
	pApp->registerGUIObject(pTimeSpentWaitingForGPUInUpdateValue, true);
	pValueLayout->addChild(pTimeSpentWaitingForGPUInUpdateValue, 1);
	pTimeSpentWaitingForGPUInUpdateValue->setVisible(true);

	// fTimeSpentOnUpdateInMS
	SGUISimpleText* pTimeSpentOnUpdateValue = new SGUISimpleText("profiler GUI: fTimeSpentOnUpdateInMS (value)");
	pTimeSpentOnUpdateValue->bIsSProfilerObject = true;
	pTimeSpentOnUpdateValue->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOnUpdateValue->setZLayer(INT_MAX);
	pTimeSpentOnUpdateValue->setDrawTextOutline(true, SVector());
	pTimeSpentOnUpdateValue->setText(L"Time spent on 'update()' (ms):");
	pApp->registerGUIObject(pTimeSpentOnUpdateValue, true);
	pValueLayout->addChild(pTimeSpentOnUpdateValue, 1);
	pTimeSpentOnUpdateValue->setVisible(true);

	// fTimeSpentOnCPUDrawInMS
	SGUISimpleText* pTimeSpentOnCPUDrawValue = new SGUISimpleText("profiler GUI: fTimeSpentOnCPUDrawInMS (value)");
	pTimeSpentOnCPUDrawValue->bIsSProfilerObject = true;
	pTimeSpentOnCPUDrawValue->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOnCPUDrawValue->setZLayer(INT_MAX);
	pTimeSpentOnCPUDrawValue->setDrawTextOutline(true, SVector());
	pTimeSpentOnCPUDrawValue->setText(L"Time spent on CPU 'draw()' (ms):");
	pApp->registerGUIObject(pTimeSpentOnCPUDrawValue, true);
	pValueLayout->addChild(pTimeSpentOnCPUDrawValue, 1);
	pTimeSpentOnCPUDrawValue->setVisible(true);

	// fTimeSpentOnFPSCalcInMS
	SGUISimpleText* pTimeSpentOnFPSCalcValue = new SGUISimpleText("profiler GUI: fTimeSpentOnFPSCalcInMS (value)");
	pTimeSpentOnFPSCalcValue->bIsSProfilerObject = true;
	pTimeSpentOnFPSCalcValue->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOnFPSCalcValue->setZLayer(INT_MAX);
	pTimeSpentOnFPSCalcValue->setDrawTextOutline(true, SVector());
	pTimeSpentOnFPSCalcValue->setText(L"Time spent on FPS calc (ms):");
	pApp->registerGUIObject(pTimeSpentOnFPSCalcValue, true);
	pValueLayout->addChild(pTimeSpentOnFPSCalcValue, 1);
	pTimeSpentOnFPSCalcValue->setVisible(true);

	// fTimeSpentInFPSLimitSleepInMS
	SGUISimpleText* pTimeSpentInFPSLimitSleepValue = new SGUISimpleText("profiler GUI: fTimeSpentInFPSLimitSleepInMS (value)");
	pTimeSpentInFPSLimitSleepValue->bIsSProfilerObject = true;
	pTimeSpentInFPSLimitSleepValue->setFont(pApp->sPathToDefaultFont);
	pTimeSpentInFPSLimitSleepValue->setZLayer(INT_MAX);
	pTimeSpentInFPSLimitSleepValue->setDrawTextOutline(true, SVector());
	pTimeSpentInFPSLimitSleepValue->setText(L"Time spent in FPS limit sleep (ms):");
	pApp->registerGUIObject(pTimeSpentInFPSLimitSleepValue, true);
	pValueLayout->addChild(pTimeSpentInFPSLimitSleepValue, 1);
	pTimeSpentInFPSLimitSleepValue->setVisible(true);

	// fTimeSpentOnUserPhysicsTickFunctionInMS
	SGUISimpleText* pTimeSpentOnUserOnPhysicsTickFunctionValue = new SGUISimpleText("profiler GUI: fTimeSpentOnUserPhysicsTickFunctionInMS (value)");
	pTimeSpentOnUserOnPhysicsTickFunctionValue->bIsSProfilerObject = true;
	pTimeSpentOnUserOnPhysicsTickFunctionValue->setFont(pApp->sPathToDefaultFont);
	pTimeSpentOnUserOnPhysicsTickFunctionValue->setZLayer(INT_MAX);
	pTimeSpentOnUserOnPhysicsTickFunctionValue->setDrawTextOutline(true, SVector());
	pTimeSpentOnUserOnPhysicsTickFunctionValue->setText(L"[i] Time spent on user physics tick (ms):");
	pApp->registerGUIObject(pTimeSpentOnUserOnPhysicsTickFunctionValue, true);
	pValueLayout->addChild(pTimeSpentOnUserOnPhysicsTickFunctionValue, 1);
	pTimeSpentOnUserOnPhysicsTickFunctionValue->setVisible(true);

	return false;
}

void SProfiler::showOnScreen(bool bShow, float fDataUpdateIntervalInMS)
{
	bFrameStatsShownOnScreen = bShow;
	fUpdateGUIIntervalInMS = fDataUpdateIntervalInMS;

	updateFrameStatsOnScreen();

	pNameLayout->setVisible(bShow);
	pValueLayout->setVisible(bShow);
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

			std::vector<SLayoutChild>* pvValues = pValueLayout->getChilds();

			const size_t iPrecision = 1;

			dynamic_cast<SGUISimpleText*>(pvValues->operator[](0).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOnWindowsMessagesInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pvValues->operator[](1).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOnUserOnTickFunctionInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pvValues->operator[](2).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOn3DAudioUpdateInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pvValues->operator[](3).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentWaitingForGPUInUpdateInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pvValues->operator[](4).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOnUpdateInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pvValues->operator[](5).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOnCPUDrawInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pvValues->operator[](6).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOnFPSCalcInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pvValues->operator[](7).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentInFPSLimitSleepInMS, iPrecision));
			dynamic_cast<SGUISimpleText*>(pvValues->operator[](8).pChild)->setText(floatToWstring(lastFrameStats.fTimeSpentOnUserPhysicsTickFunctionInMS, iPrecision));
			
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