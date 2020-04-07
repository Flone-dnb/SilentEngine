// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

#include <DirectXMath.h>

class SMath
{
public:

	static DirectX::XMFLOAT4X4 getIdentityMatrix4x4 ();

	static unsigned int makeMultipleOf256           (unsigned int iNumber);

	template<typename T>
	static T clamp(const T& x, const T& low, const T& high)
	{
		return x < low ? low : (x > high ? high : x);
	}


	// -------------------------------


	static const float fPi;
};