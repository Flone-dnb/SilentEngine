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

	vSizeToKeep = SVector(-1.0f, -1.0f);

	scale = DirectX::XMFLOAT2(1.0f, 1.0f);
	screenScale = DirectX::XMFLOAT2(1.0f, 1.0f);
	color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pos = DirectX::SimpleMath::Vector2(0.5f, 0.5f);
	origin = DirectX::SimpleMath::Vector2(0.5f, 0.5f);

	bIsVisible = false;
	bIsRegistered = false;

	objectType = SGUIType::SGT_NONE;
}

void SGUIObject::setSizeToKeep(const SVector& vSizeToKeep)
{
	if (vSizeToKeep.getX() < 0.0f || vSizeToKeep.getX() > 1.0f || vSizeToKeep.getY() < 0.0f || vSizeToKeep.getY() > 1.0f)
	{
		SError::showErrorMessageBoxAndLog("size values should be normalized.");
		return;
	}

	this->vSizeToKeep = vSizeToKeep;

	recalculateSizeToKeepScaling();
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

int SGUIObject::getZLayer() const
{
	return iZLayer;
}

SGUIObject::~SGUIObject()
{
}