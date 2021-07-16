// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <vector>

// Custom
#include "SilentEngine/public/SApplication/SApplication.h"

class EditorApplication : public SApplication
{
public:
	EditorApplication(HINSTANCE hInstance);

	void onRun() override;

	virtual void onMouseMove (int iMouseXMove, int iMouseYMove) override;
	virtual void onMouseDown (SMouseKey mouseKey, int iMouseXPos, int iMouseYPos) override;
	virtual void onMouseUp   (SMouseKey mouseKey, int iMouseXPos, int iMouseYPos) override;
	virtual void onMouseWheelMove(bool bUp, int iMouseXPos, int iMouseYPos) override;

	virtual void onKeyboardButtonDown (SKeyboardKey keyboardKey) override;
	virtual void onKeyboardButtonUp   (SKeyboardKey keyboardKey) override;

	virtual void onPhysicsTick(float fDeltaTime) override;

	~EditorApplication() override;

private:

	float fMoveStepSize = 2.0f;
	float fShiftSpeedMult = 5.0f;
	float fCtrlSpeedMult = 0.25f;
	float fMouseSensitivity = 0.1f;

	bool bEnableInput = false;

	bool bWPressed = false, bSPressed = false, bDPressed = false, bAPressed = false, bEPressed = false, bQPressed = false;

	bool bShiftPressed = false;
	bool bCtrlPressed = false;
	bool bAltPressed  = false;

	bool bLMBPressed  = false;
	bool bRMBPressed  = false;
	bool bMMBPressed  = false;
};