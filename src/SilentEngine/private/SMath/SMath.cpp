// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SMath.h"


const float SMath::fPi = 3.1415926535f;

DirectX::XMFLOAT4X4 SMath::getIdentityMatrix4x4()
{
	static DirectX::XMFLOAT4X4 I(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	return I;
}

unsigned int SMath::makeMultipleOf256(unsigned int iNumber)
{
	// We are adding 255 and then masking off
	// the lower 2 bytes which store all bits < 256.

	// Example: Suppose iNumber = 300.
	// (300 + 255) & ~255
	// 555 & ~255
	// 0x022B & ~0x00ff
	// 0x022B & 0xff00
	// 0x0200
	// 512
	return (iNumber + 255) & ~255;
}
