// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "EditorApplication.h"

#include "SilentEditor/MyContainer/MyContainer.h"

EditorApplication::EditorApplication(HINSTANCE hInstance) : SApplication(hInstance)
{
	// Use onRun().
}

void EditorApplication::onRun()
{
	// Remember: 1 unit - 1 meter (near/far clip planes were picked so that 1 unit - 1 meter).

	// If you're making a space game, for example, then you might consider 1 unit as 10 meters (for example), but this will
	// require you to change the near clip plane (in this example to 1.0 at least) to avoid z-fighting.
	// In the case of a space game also do not forget about float precision in terms of position (as it's stored in floats).

	// If you will use camera roll (roll axis), use setDontFlipCamera() to false.

	// ... prepare stuff here ...

	// Add ambient light.
	auto settings = getGlobalVisualSettings();
	settings.vAmbientLightRGB = SVector(0.1f, 0.1f, 0.2f);
	setGlobalVisualSettings(settings);
	
	// Spawn my container;
	pMyContainer = new MyContainer("My Container");
	getCurrentLevel()->spawnContainerInLevel(pMyContainer);
	pMyContainer->setLocation(SVector(0.0f, 0.0f, -1.0f));
	getCamera()->setCameraLocationInWorld(SVector(0.0f, -10.0f, 0.0f));

	// ... prepare stuff here ...

	bEnableInput = true;
}

void EditorApplication::onMouseMove(int iMouseXMove, int iMouseYMove)
{
	if (bEnableInput == false)
	{
		return;
	}

	if (bRMBPressed)
	{
		getCamera()->rotateCamera(iMouseYMove * fMouseSensitivity, iMouseXMove * fMouseSensitivity, 0.0f);
	}
}

void EditorApplication::onMouseDown(SMouseKey mouseKey, int iMouseXPos, int iMouseYPos)
{
	if (bEnableInput == false)
	{
		return;
	}

	if (mouseKey.getButton() == SMouseButton::SMB_LEFT)
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
	if (bEnableInput == false)
	{
		return;
	}

	if (mouseKey.getButton() == SMouseButton::SMB_LEFT)
	{
		bLMBPressed = false;
	}
	else if (mouseKey.getButton() == SMouseButton::SMB_RIGHT)
	{
		bRMBPressed = false;
		setShowMouseCursor(true);
	}
	else if (mouseKey.getButton() == SMouseButton::SMB_MIDDLE)
	{
		bMMBPressed = false;
	}
}

void EditorApplication::onMouseWheelMove(bool bUp, int iMouseXPos, int iMouseYPos)
{
	if (bEnableInput == false)
	{
		return;
	}
}

void EditorApplication::onKeyboardButtonDown(SKeyboardKey keyboardKey)
{
	if (bEnableInput == false)
	{
		return;
	}

	if (keyboardKey.getButton() == SKeyboardButton::SKB_W)
	{
		bWPressed = true;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_S)
	{
		bSPressed = true;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_D)
	{
		bDPressed = true;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_A)
	{
		bAPressed = true;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_E)
	{
		bEPressed = true;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_Q)
	{
		bQPressed = true;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_ESC)
	{
		close();
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_LCTRL)
	{
		bCtrlPressed = true;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_LALT)
	{
		bAltPressed = true;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_LSHIFT)
	{
		bShiftPressed = true;
	}
}

void EditorApplication::onKeyboardButtonUp(SKeyboardKey keyboardKey)
{
	if (bEnableInput == false)
	{
		return;
	}

	if (keyboardKey.getButton() == SKeyboardButton::SKB_W)
	{
		bWPressed = false;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_S)
	{
		bSPressed = false;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_D)
	{
		bDPressed = false;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_A)
	{
		bAPressed = false;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_E)
	{
		bEPressed = false;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_Q)
	{
		bQPressed = false;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_LCTRL)
	{
		bCtrlPressed = false;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_LALT)
	{
		bAltPressed = false;
	}
	else if (keyboardKey.getButton() == SKeyboardButton::SKB_LSHIFT)
	{
		bShiftPressed = false;
	}
}

void EditorApplication::onPhysicsTick(float fDeltaTime)
{
	if (bRMBPressed && (bWPressed || bSPressed || bDPressed || bAPressed || bQPressed || bEPressed))
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

		fSpeedMult *= fDeltaTime;

		float fSpeed = fMoveStepSize * fSpeedMult;

		getCamera()->moveCameraForward((bWPressed - bSPressed) * fSpeed);
		getCamera()->moveCameraRight((bDPressed - bAPressed) * fSpeed);
		getCamera()->moveCameraUp((bEPressed - bQPressed) * fSpeed);
	}

	if (pMyContainer) pMyContainer->onTick(fDeltaTime);
}

EditorApplication::~EditorApplication()
{
	// If the container was spawned (spawnContainerInLevel()) then
	// here DO NOT:
	// - delete pContainer;
	// - pContainer->despawnFromLevel();
	// the spawned container will be deleted on level destroy.
}