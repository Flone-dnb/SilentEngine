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

	virtual void onKeyboardButtonDown (SKeyboardKey keyboardKey) override;
	virtual void onKeyboardButtonUp   (SKeyboardKey keyboardKey) override;

	~EditorApplication() override;

private:

	int mousePosX = 0, mousePosY = 0;

	bool bCtrlPressed = false;
	bool bAltPressed  = false;

	bool bLMBPressed  = false;
	bool bRMBPressed  = false;
	bool bMMBPressed  = false;
};