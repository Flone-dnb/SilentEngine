// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SGUIObject.h"



// Custom
#include "SilentEngine/Public/SApplication/SApplication.h"
#include "SilentEngine/Private/SError/SError.h"

SGUIObject::SGUIObject(const std::string& sObjectName)
{
	this->sObjectName = sObjectName;
	iZLayer = 0;
	fRotationInRad = 0.0f;
	scale = DirectX::XMFLOAT2(1.0f, 1.0f);
	color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pos = DirectX::SimpleMath::Vector2(0.5f, 0.5f);
	origin = DirectX::SimpleMath::Vector2(0.5f, 0.5f);
	sourceRect = { 0.0f, 0.0f, 1.0f, 1.0f };
	effects = DirectX::SpriteEffects::SpriteEffects_None;
	bIsVisible = false;
	bIsRegistered = false;

	objectType = SGUIType::SGT_NONE;
}

void SGUIObject::setVisible(bool bIsVisible)
{
	this->bIsVisible = bIsVisible;
}

void SGUIObject::setPosition(const SVector& vPos)
{
	if (vPos.getX() < 0.0f || vPos.getX() > 1.0f || vPos.getY() < 0.0f || vPos.getY() > 1.0f)
	{
		SError::showErrorMessageBoxAndLog("position values should be normalized.");
		return;
	}

	pos = DirectX::SimpleMath::Vector2(vPos.getX(), vPos.getY());
}

void SGUIObject::setRotation(float fRotationInDeg)
{
	this->fRotationInRad = DirectX::XMConvertToRadians(fRotationInDeg);
}

void SGUIObject::setScale(const SVector& vScale)
{
	scale = DirectX::XMFLOAT2(vScale.getX(), vScale.getY());
}

void SGUIObject::setTint(const SVector& vColor)
{
	color = DirectX::XMFLOAT4(vColor.getX(), vColor.getY(), vColor.getZ(), vColor.getW());
}

void SGUIObject::setFlip(bool bFlipHorizontally, bool bFlipVertically)
{
	if (bFlipHorizontally && bFlipVertically)
	{
		effects = DirectX::SpriteEffects::SpriteEffects_FlipBoth;
	}
	else if (bFlipHorizontally)
	{
		effects = DirectX::SpriteEffects::SpriteEffects_FlipHorizontally;
	}
	else if (bFlipVertically)
	{
		effects = DirectX::SpriteEffects::SpriteEffects_FlipVertically;
	}
	else
	{
		effects = DirectX::SpriteEffects::SpriteEffects_None;
	}
}

void SGUIObject::setCut(const SVector& vSourceRect)
{
	if (vSourceRect.getX() < 0.0f || vSourceRect.getX() > 1.0f || vSourceRect.getY() < 0.0f || vSourceRect.getY() > 1.0f ||
		vSourceRect.getZ() < 0.0f || vSourceRect.getZ() > 1.0f || vSourceRect.getW() < 0.0f || vSourceRect.getW() > 1.0f)
	{
		SError::showErrorMessageBoxAndLog("cut values should be normalized.");
		return;
	}

	sourceRect = vSourceRect;
}

void SGUIObject::setCustomOrigin(const SVector& vOrigin)
{
	if (vOrigin.getX() < 0.0f || vOrigin.getX() > 1.0f || vOrigin.getY() < 0.0f || vOrigin.getY() > 1.0f)
	{
		SError::showErrorMessageBoxAndLog("origin values should be normalized.");
		return;
	}

	origin = DirectX::SimpleMath::Vector2(vOrigin.getX(), vOrigin.getY());
}

void SGUIObject::setZLayer(int iZLayer)
{
	if (this->iZLayer == iZLayer)
	{
		return;
	}

	if (bIsRegistered)
	{
		SApplication::getApp()->moveGUIObjectToLayer(this, iZLayer);
	}
	else
	{
		this->iZLayer = iZLayer; // will be moved to this layer in registerGUIObject()
	}
}

bool SGUIObject::isVisible() const
{
	return bIsVisible;
}

SVector SGUIObject::getPosition() const
{
	return SVector(pos.x, pos.y);
}

float SGUIObject::getRotation() const
{
	return DirectX::XMConvertToDegrees(fRotationInRad);
}

SVector SGUIObject::getScaling() const
{
	return SVector(scale.x, scale.y);
}

SVector SGUIObject::getTint() const
{
	return SVector(color.x, color.y, color.z, color.w);
}

SVector SGUIObject::getOrigin() const
{
	return SVector(origin.x, origin.y);
}

void SGUIObject::getFlip(bool& bFlippedHorizontally, bool& bFlippedVertically) const
{
	if (effects == DirectX::SpriteEffects::SpriteEffects_None)
	{
		bFlippedHorizontally = false;
		bFlippedVertically = false;
	}
	else if (effects == DirectX::SpriteEffects::SpriteEffects_FlipBoth)
	{
		bFlippedHorizontally = true;
		bFlippedVertically = true;
	}
	else if (effects == DirectX::SpriteEffects::SpriteEffects_FlipHorizontally)
	{
		bFlippedHorizontally = true;
		bFlippedVertically = false;
	}
	else if (effects == DirectX::SpriteEffects::SpriteEffects_FlipVertically)
	{
		bFlippedHorizontally = false;
		bFlippedVertically = true;
	}
}

int SGUIObject::getZLayer() const
{
	return iZLayer;
}

SGUIObject::~SGUIObject()
{
}