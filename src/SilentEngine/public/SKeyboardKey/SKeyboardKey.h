// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#pragma once

#include <windows.h>

enum class SKeyboardButton
{
	SKB_NONE  = 0,
	SKB_ESC   = 3,
	SKB_TAB   = 4,
	SKB_BACKSPACE = 5,
	SKB_ENTER     = 6,
	SKB_CAPSLOCK  = 7,
	SKB_SPACEBAR  = 8,
	SKB_PAGEUP    = 9,
	SKB_PAGEDOWN  = 10,
	SKB_END       = 11,
	SKB_HOME      = 12,
	SKB_LEFT      = 13,
	SKB_UP        = 14,
	SKB_RIGHT     = 15,
	SKB_DOWN      = 16,
	SKB_PRINTSCREEN = 17,
	SKB_INSERT    = 18,
	SKB_DELETE    = 19,
	SKB_0 = 20,
	SKB_1 = 21,
	SKB_2 = 22,
	SKB_3 = 23,
	SKB_4 = 24,
	SKB_5 = 25,
	SKB_6 = 26,
	SKB_7 = 27,
	SKB_8 = 28,
	SKB_9 = 29,
	SKB_A = 30,
	SKB_B = 31,
	SKB_C = 32,
	SKB_D = 33,
	SKB_E = 34,
	SKB_F = 35,
	SKB_G = 36,
	SKB_H = 37,
	SKB_I = 38,
	SKB_J = 39,
	SKB_K = 40,
	SKB_L = 41,
	SKB_M = 42,
	SKB_N = 43,
	SKB_O = 44,
	SKB_P = 45,
	SKB_Q = 46,
	SKB_R = 47,
	SKB_S = 48,
	SKB_T = 49,
	SKB_U = 50,
	SKB_V = 51,
	SKB_W = 52,
	SKB_X = 53,
	SKB_Y = 54,
	SKB_Z = 55,
	SKB_NUM0 = 56,
	SKB_NUM1 = 57,
	SKB_NUM2 = 58,
	SKB_NUM3 = 59,
	SKB_NUM4 = 60,
	SKB_NUM5 = 61,
	SKB_NUM6 = 62,
	SKB_NUM7 = 63,
	SKB_NUM8 = 64,
	SKB_NUM9 = 65,
	SKB_F1 = 66,
	SKB_F2 = 67,
	SKB_F3 = 68,
	SKB_F4 = 69,
	SKB_F5 = 70,
	SKB_F6 = 71,
	SKB_F7 = 72,
	SKB_F8 = 73,
	SKB_F9 = 74,
	SKB_F10 = 75,
	SKB_F11 = 76,
	SKB_F12 = 77,
	SKB_LSHIFT = 78,
	SKB_RSHIFT = 79,
	SKB_LCTRL = 80,
	SKB_RCTRL = 81,
	SKB_LALT  = 82,
	SKB_RALT  = 83
};

//@@Class
/*
The class holds a keyboard button in it.
*/
class SKeyboardKey
{
public:

	SKeyboardKey() = delete;

	//@@Function
	/*
	* desc: this constructor is trying to determine the key from the given WPARAM and LPARAM
	received in the WM_KEYDOWN or WM_SYSKEYDOWN windows messages.
	* param "wParam": WPARAM arg from the windows message.
	* param "lParam": LPARAM arg from the windows message.
	*/
	SKeyboardKey(WPARAM wParam, LPARAM lParam)
	{
		keyboardButton = SKeyboardButton::SKB_NONE;

		determineKey(wParam, lParam);
	}

	//@@Function
	/*
	* desc: returns the element of the SKeyboardButton enumeration which holds the key value.
	*/
	SKeyboardButton getButton()
	{
		return keyboardButton;
	}

private:

	//@@Function
	/*
	* desc: this function is trying to determine the key from the given WPARAM and LPARAM
	received in the WM_KEYDOWN or WM_SYSKEYDOWN windows messages.
	* param "wParam": WPARAM arg from the windows message.
	* param "lParam": LPARAM arg from the windows message.
	*/
	void determineKey(WPARAM wParam, LPARAM lParam)
	{
		switch (wParam)
		{
		case(VK_ESCAPE):
		{
			keyboardButton = SKeyboardButton::SKB_ESC;
			break;
		}
		case(VK_MENU):
		{
			int extended  = (lParam & 0x01000000) != 0;

			if (extended)
			{
				keyboardButton = SKeyboardButton::SKB_RALT;
			}
			else
			{
				keyboardButton = SKeyboardButton::SKB_LALT;
			}

			break;
		}
		case(VK_CONTROL):
		{
			int extended  = (lParam & 0x01000000) != 0;

			if (extended)
			{
				keyboardButton = SKeyboardButton::SKB_RCTRL;
			}
			else
			{
				keyboardButton = SKeyboardButton::SKB_LCTRL;
			}

			break;
		}
		case(VK_SHIFT):
		{
			UINT scancode = (lParam & 0x00ff0000) >> 16;
			WPARAM w = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
			
			if (w == VK_LSHIFT)
			{
				keyboardButton = SKeyboardButton::SKB_LSHIFT;
			}
			else
			{
				keyboardButton = SKeyboardButton::SKB_RSHIFT;
			}

			break;
		}
		case(VK_TAB):
		{
			keyboardButton = SKeyboardButton::SKB_TAB;
			break;
		}
		case(VK_BACK):
		{
			keyboardButton = SKeyboardButton::SKB_BACKSPACE;
			break;
		}
		case(VK_RETURN):
		{
			keyboardButton = SKeyboardButton::SKB_ENTER;
			break;
		}
		case(VK_CAPITAL):
		{
			keyboardButton = SKeyboardButton::SKB_CAPSLOCK;
			break;
		}
		case(VK_SPACE):
		{
			keyboardButton = SKeyboardButton::SKB_SPACEBAR;
			break;
		}
		case(VK_PRIOR):
		{
			keyboardButton = SKeyboardButton::SKB_PAGEUP;
			break;
		}
		case(VK_NEXT):
		{
			keyboardButton = SKeyboardButton::SKB_PAGEDOWN;
			break;
		}
		case(VK_END):
		{
			keyboardButton = SKeyboardButton::SKB_END;
			break;
		}
		case(VK_HOME):
		{
			keyboardButton = SKeyboardButton::SKB_HOME;
			break;
		}
		case(VK_LEFT):
		{
			keyboardButton = SKeyboardButton::SKB_LEFT;
			break;
		}
		case(VK_UP):
		{
			keyboardButton = SKeyboardButton::SKB_UP;
			break;
		}
		case(VK_RIGHT):
		{
			keyboardButton = SKeyboardButton::SKB_RIGHT;
			break;
		}
		case(VK_DOWN):
		{
			keyboardButton = SKeyboardButton::SKB_DOWN;
			break;
		}
		case(VK_SNAPSHOT):
		{
			keyboardButton = SKeyboardButton::SKB_PRINTSCREEN;
			break;
		}
		case(VK_INSERT):
		{
			keyboardButton = SKeyboardButton::SKB_INSERT;
			break;
		}
		case(VK_DELETE):
		{
			keyboardButton = SKeyboardButton::SKB_DELETE;
			break;
		}
		case(0x30):
		{
			keyboardButton = SKeyboardButton::SKB_0;
			break;
		}
		case(0x31):
		{
			keyboardButton = SKeyboardButton::SKB_1;
			break;
		}
		case(0x32):
		{
			keyboardButton = SKeyboardButton::SKB_2;
			break;
		}
		case(0x33):
		{
			keyboardButton = SKeyboardButton::SKB_3;
			break;
		}
		case(0x34):
		{
			keyboardButton = SKeyboardButton::SKB_4;
			break;
		}
		case(0x35):
		{
			keyboardButton = SKeyboardButton::SKB_5;
			break;
		}
		case(0x36):
		{
			keyboardButton = SKeyboardButton::SKB_6;
			break;
		}
		case(0x37):
		{
			keyboardButton = SKeyboardButton::SKB_7;
			break;
		}
		case(0x38):
		{
			keyboardButton = SKeyboardButton::SKB_8;
			break;
		}
		case(0x39):
		{
			keyboardButton = SKeyboardButton::SKB_9;
			break;
		}
		case(0x41):
		{
			keyboardButton = SKeyboardButton::SKB_A;
			break;
		}
		case(0x42):
		{
			keyboardButton = SKeyboardButton::SKB_B;
			break;
		}
		case(0x43):
		{
			keyboardButton = SKeyboardButton::SKB_C;
			break;
		}
		case(0x44):
		{
			keyboardButton = SKeyboardButton::SKB_D;
			break;
		}
		case(0x45):
		{
			keyboardButton = SKeyboardButton::SKB_E;
			break;
		}
		case(0x46):
		{
			keyboardButton = SKeyboardButton::SKB_F;
			break;
		}
		case(0x47):
		{
			keyboardButton = SKeyboardButton::SKB_G;
			break;
		}
		case(0x48):
		{
			keyboardButton = SKeyboardButton::SKB_H;
			break;
		}
		case(0x49):
		{
			keyboardButton = SKeyboardButton::SKB_I;
			break;
		}
		case(0x4A):
		{
			keyboardButton = SKeyboardButton::SKB_J;
			break;
		}
		case(0x4B):
		{
			keyboardButton = SKeyboardButton::SKB_K;
			break;
		}
		case(0x4C):
		{
			keyboardButton = SKeyboardButton::SKB_L;
			break;
		}
		case(0x4D):
		{
			keyboardButton = SKeyboardButton::SKB_M;
			break;
		}
		case(0x4E):
		{
			keyboardButton = SKeyboardButton::SKB_N;
			break;
		}
		case(0x4F):
		{
			keyboardButton = SKeyboardButton::SKB_O;
			break;
		}
		case(0x50):
		{
			keyboardButton = SKeyboardButton::SKB_P;
			break;
		}
		case(0x51):
		{
			keyboardButton = SKeyboardButton::SKB_Q;
			break;
		}
		case(0x52):
		{
			keyboardButton = SKeyboardButton::SKB_R;
			break;
		}
		case(0x53):
		{
			keyboardButton = SKeyboardButton::SKB_S;
			break;
		}
		case(0x54):
		{
			keyboardButton = SKeyboardButton::SKB_T;
			break;
		}
		case(0x55):
		{
			keyboardButton = SKeyboardButton::SKB_U;
			break;
		}
		case(0x56):
		{
			keyboardButton = SKeyboardButton::SKB_V;
			break;
		}
		case(0x57):
		{
			keyboardButton = SKeyboardButton::SKB_W;
			break;
		}
		case(0x58):
		{
			keyboardButton = SKeyboardButton::SKB_X;
			break;
		}
		case(0x59):
		{
			keyboardButton = SKeyboardButton::SKB_Y;
			break;
		}
		case(0x5A):
		{
			keyboardButton = SKeyboardButton::SKB_Z;
			break;
		}
		case(0x60):
		{
			keyboardButton = SKeyboardButton::SKB_NUM0;
			break;
		}
		case(0x61):
		{
			keyboardButton = SKeyboardButton::SKB_NUM1;
			break;
		}
		case(0x62):
		{
			keyboardButton = SKeyboardButton::SKB_NUM2;
			break;
		}
		case(0x63):
		{
			keyboardButton = SKeyboardButton::SKB_NUM3;
			break;
		}
		case(0x64):
		{
			keyboardButton = SKeyboardButton::SKB_NUM4;
			break;
		}
		case(0x65):
		{
			keyboardButton = SKeyboardButton::SKB_NUM5;
			break;
		}
		case(0x66):
		{
			keyboardButton = SKeyboardButton::SKB_NUM6;
			break;
		}
		case(0x67):
		{
			keyboardButton = SKeyboardButton::SKB_NUM7;
			break;
		}
		case(0x68):
		{
			keyboardButton = SKeyboardButton::SKB_NUM8;
			break;
		}
		case(0x69):
		{
			keyboardButton = SKeyboardButton::SKB_NUM9;
			break;
		}
		case(VK_F1):
		{
			keyboardButton = SKeyboardButton::SKB_F1;
			break;
		}
		case(VK_F2):
		{
			keyboardButton = SKeyboardButton::SKB_F2;
			break;
		}
		case(VK_F3):
		{
			keyboardButton = SKeyboardButton::SKB_F3;
			break;
		}
		case(VK_F4):
		{
			keyboardButton = SKeyboardButton::SKB_F4;
			break;
		}
		case(VK_F5):
		{
			keyboardButton = SKeyboardButton::SKB_F5;
			break;
		}
		case(VK_F6):
		{
			keyboardButton = SKeyboardButton::SKB_F6;
			break;
		}
		case(VK_F7):
		{
			keyboardButton = SKeyboardButton::SKB_F7;
			break;
		}
		case(VK_F8):
		{
			keyboardButton = SKeyboardButton::SKB_F8;
			break;
		}
		case(VK_F9):
		{
			keyboardButton = SKeyboardButton::SKB_F9;
			break;
		}
		case(VK_F10):
		{
			keyboardButton = SKeyboardButton::SKB_F10;
			break;
		}
		case(VK_F11):
		{
			keyboardButton = SKeyboardButton::SKB_F11;
			break;
		}
		case(VK_F12):
		{
			keyboardButton = SKeyboardButton::SKB_F12;
			break;
		}
		}
	}

	//@@Variable
	/* keyboard button, element of the SKeyboardButton enumeration which holds the key value. */
	SKeyboardButton keyboardButton;
};