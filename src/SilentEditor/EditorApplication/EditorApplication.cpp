// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "EditorApplication.h"

EditorApplication::EditorApplication(HINSTANCE hInstance) : SApplication(hInstance)
{
	setCallTick(true);
}

void EditorApplication::onRun()
{
	// Remember: 1 unit - 1 meter (near/far clip planes was picked so that 1 unit - 1 meter).

	// If you're making a space game for example, then you might consider 1 unit as 10 meters (for example), but this will
	// require you to change the near clip plane (in this example to 1.0 at least) to avoid z-fighting.
	// In the case of a space game also do not forget about float precision in terms of position (as it's stored in floats).

	// If you will use camera roll (roll axis), use setDontFlipCamera() to false.


}

void EditorApplication::onMouseMove(int iMouseXMove, int iMouseYMove)
{
	if (bRMBPressed)
	{
		getCamera()->rotateCamera(iMouseYMove * fMouseSensitivity, iMouseXMove * fMouseSensitivity, 0.0f);
	}
}

void EditorApplication::onMouseDown(SMouseKey mouseKey, int iMouseXPos, int iMouseYPos)
{
	if (mouseKey.getButton() == SMB_LEFT)
	{
		bLMBPressed = true;
	}
	else if (mouseKey.getButton() == SMouseButton::SMB_RIGHT)
	{
		bRMBPressed = true;

		setShowMouseCursor(false);
	}
	else if (mouseKey.getButton() == SMouseButton::SMB_MIDDLE)
	{
		bMMBPressed = true;

		getVideoSettings()->setEnableWireframeMode(!getVideoSettings()->isWireframeModeEnabled());
	}
}


void EditorApplication::onMouseUp(SMouseKey mouseKey, int iMouseXPos, int iMouseYPos)
{
	if (mouseKey.getButton() == SMouseButton::SMB_LEFT)
	{
		bLMBPressed = false;
	}
	else if (mouseKey.getButton() == SMouseButton::SMB_RIGHT)
	{
		bRMBPressed = false;
		setShowMouseCursor(true);
	}
	else if (mouseKey.getButton() == SMB_MIDDLE)
	{
		bMMBPressed = false;
	}
}

void EditorApplication::onKeyboardButtonDown(SKeyboardKey keyboardKey)
{
	if (keyboardKey.getButton() == SKB_W)
	{
		bWPressed = true;
	}
	else if (keyboardKey.getButton() == SKB_S)
	{
		bSPressed = true;
	}
	else if (keyboardKey.getButton() == SKB_D)
	{
		bDPressed = true;
	}
	else if (keyboardKey.getButton() == SKB_A)
	{
		bAPressed = true;
	}
	else if (keyboardKey.getButton() == SKB_E)
	{
		bEPressed = true;
	}
	else if (keyboardKey.getButton() == SKB_Q)
	{
		bQPressed = true;
	}
	else if (keyboardKey.getButton() == SKB_ESC)
	{
		close();
	}
	else if (keyboardKey.getButton() == SKB_LCTRL)
	{
		bCtrlPressed = true;
	}
	else if (keyboardKey.getButton() == SKB_LALT)
	{
		bAltPressed = true;
	}
	else if (keyboardKey.getButton() == SKB_LSHIFT)
	{
		bShiftPressed = true;
	}

	if (bRMBPressed && keyboardPressTimer.isTimerRunning() == false)
	{
		if (keyboardKey.getButton() == SKB_W || keyboardKey.getButton() == SKB_S
			|| keyboardKey.getButton() == SKB_D || keyboardKey.getButton() == SKB_A
			|| keyboardKey.getButton() == SKB_Q || keyboardKey.getButton() == SKB_E)
		{
			std::function<void(void)> f = std::bind(&EditorApplication::autoRepeatKeyPress, this);
			keyboardPressTimer.setCallbackOnTimeout(f, fMoveTimeStep, true, fMoveTimeStep);
			keyboardPressTimer.start();
		}
	}
}

void EditorApplication::autoRepeatKeyPress()
{
	float fSpeedMult = 1.0f;

	if (bShiftPressed)
	{
		fSpeedMult = fShiftSpeedMult;
	}
	else if (bCtrlPressed)
	{
		fSpeedMult = fCtrlSpeedMult;
	}

	float fSpeed = fMoveStepSize * fSpeedMult;

	getCamera()->moveCameraForward((bWPressed - bSPressed) * fSpeed);
	getCamera()->moveCameraRight((bDPressed - bAPressed) * fSpeed);
	getCamera()->moveCameraUp((bEPressed - bQPressed) * fSpeed);

	checkAutoRepeatStop();
}

void EditorApplication::checkAutoRepeatStop()
{
	if (!bWPressed && !bSPressed && !bDPressed && !bAPressed && !bEPressed && !bQPressed)
	{
		// Nothing is pressed.
		keyboardPressTimer.stop();
	}
}

void EditorApplication::onKeyboardButtonUp(SKeyboardKey keyboardKey)
{
	if (keyboardKey.getButton() == SKB_W)
	{
		bWPressed = false;
	}
	else if (keyboardKey.getButton() == SKB_S)
	{
		bSPressed = false;
	}
	else if (keyboardKey.getButton() == SKB_D)
	{
		bDPressed = false;
	}
	else if (keyboardKey.getButton() == SKB_A)
	{
		bAPressed = false;
	}
	else if (keyboardKey.getButton() == SKB_E)
	{
		bEPressed = false;
	}
	else if (keyboardKey.getButton() == SKB_Q)
	{
		bQPressed = false;
	}
	else if (keyboardKey.getButton() == SKB_LCTRL)
	{
		bCtrlPressed = false;
	}
	else if (keyboardKey.getButton() == SKB_LALT)
	{
		bAltPressed = false;
	}
	else if (keyboardKey.getButton() == SKB_LSHIFT)
	{
		bShiftPressed = false;
	}
}

void EditorApplication::onTick(float fDelta)
{
	setWindowTitleText(
		+L"Silent Editor. CPU wait for GPU (ms): " + std::to_wstring(getProfiler()->getTimeSpentWaitingForGPUBetweenFramesInMS()));
}

EditorApplication::~EditorApplication()
{
	keyboardPressTimer.stop();

	// If the container was spawned (spawnContainerInLevel()) then
	// here DO NOT:
	// - delete pContainer;
	// - pContainer->despawnFromLevel();
	// the spawned container will be deleted on level destroy.
}