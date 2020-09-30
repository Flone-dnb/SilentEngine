// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "EditorApplication.h"

EditorApplication::EditorApplication(HINSTANCE hInstance) : SApplication(hInstance)
{
}

void EditorApplication::onRun()
{
}

void EditorApplication::onMouseMove(int iMouseXMove, int iMouseYMove)
{
	if (bMMBPressed && bCtrlPressed)
	{
		float dy = 0.01f * static_cast<float>(iMouseYMove);

		setFixedCameraZoom(getFixedCameraZoom() + dy);
	}
	else if (bMMBPressed)
	{
		setFixedCameraRotationShift(static_cast<float>(iMouseXMove), static_cast<float>(iMouseYMove));
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

		getVideoSettings()->setEnableWireframeMode(!getVideoSettings()->isWireframeModeEnabled());
	}
	else if (mouseKey.getButton() == SMouseButton::SMB_MIDDLE)
	{
		bMMBPressed = true;
		setShowMouseCursor(false);
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
	}
	else if (mouseKey.getButton() == SMB_MIDDLE)
	{
		bMMBPressed = false;
		setShowMouseCursor(true);
	}
}

void EditorApplication::onKeyboardButtonDown(SKeyboardKey keyboardKey)
{
	if (keyboardKey.getButton() == SKB_ESC)
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
}

void EditorApplication::onKeyboardButtonUp(SKeyboardKey keyboardKey)
{
	if (keyboardKey.getButton() == SKB_LCTRL)
	{
		bCtrlPressed = false;
	}
	else if (keyboardKey.getButton() == SKB_LALT)
	{
		bAltPressed = false;
	}
}

EditorApplication::~EditorApplication()
{
	// If the container was spawned (spawnContainerInLevel()) then
	// here DO NOT:
	// - delete pContainer;
	// - pContainer->despawnFromLevel();
	// the spawned container will be deleted on level destroy.
}
