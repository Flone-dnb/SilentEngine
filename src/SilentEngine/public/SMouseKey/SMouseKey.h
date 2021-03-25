// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#pragma once

#include <windows.h>
#include <vector>

enum class SMouseButton
{
	SMB_NONE   = 0,
	SMB_LEFT   = 1,
	SMB_RIGHT  = 2,
	SMB_MIDDLE = 3,
	SMB_X1     = 4,
	SMB_X2     = 5
};

//@@Class
/* The class represents the mouse button key. */
class SMouseKey
{
public:

	//@@Function
	/*
	* desc: constructor which initializes the mouse button to SMB_NONE.
	*/
	SMouseKey() 
	{
		mouseButton  = SMouseButton::SMB_NONE;
	};

	//@@Function
	/*
	* desc: constructor which tries to determine the key from wParam using the determineKey() function.
	*/
	SMouseKey(WPARAM wParam)
	{
		mouseButton  = SMouseButton::SMB_NONE;

		determineKey(wParam);
	}

	//@@Function
	/*
	* desc: sets the mouse button key.
	*/
	void setKey(SMouseButton button)
	{
		mouseButton = button;
	}

	//@@Function
	/*
	* desc: returns the mouse button key.
	*/
	SMouseButton getButton()
	{
		return mouseButton;
	}

private:

	//@@Function
	/*
	* desc: tries to determine the key from wParam and saves it. After this function, you can get determined key using the getButton() function.
	*/
	void determineKey(WPARAM wParam)
	{
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

	//@@Function
	/*
	* desc: determines the key from wParam which is other from the given alreadyPressedKey.
	*/
	void setOtherKey(WPARAM wParam, SMouseKey& alreadyPressedKey)
	{
		mouseButton  = SMouseButton::SMB_NONE;

		if (wParam & MK_LBUTTON && SMouseButton::SMB_LEFT != alreadyPressedKey.getButton())
		{
			mouseButton = SMouseButton::SMB_LEFT;
		}
		else if (wParam & MK_MBUTTON && SMouseButton::SMB_MIDDLE != alreadyPressedKey.getButton())
		{
			mouseButton = SMouseButton::SMB_MIDDLE;
		}
		else if (wParam & MK_RBUTTON && SMouseButton::SMB_RIGHT != alreadyPressedKey.getButton())
		{
			mouseButton = SMouseButton::SMB_RIGHT;
		}
		else if (GET_XBUTTON_WPARAM(wParam) & XBUTTON1 && SMouseButton::SMB_X1 != alreadyPressedKey.getButton())
		{
			mouseButton = SMouseButton::SMB_X1;
		}
		else if (GET_XBUTTON_WPARAM(wParam) & XBUTTON2 && SMouseButton::SMB_X2 != alreadyPressedKey.getButton())
		{
			mouseButton = SMouseButton::SMB_X2;
		}
	}


	
	//@@Variable
	/* mouse button key, an element of the SMouseButton enum. */
	SMouseButton mouseButton;

	friend class SApplication;
};

