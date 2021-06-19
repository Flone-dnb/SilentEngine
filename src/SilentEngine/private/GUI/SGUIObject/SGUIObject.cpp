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
#include "SilentEngine/Public/GUI/SGUILayout/SGUILayout.h"

SGUILayout* SGUIObject::getLayout() const
{
	return layoutData.pLayout;
}

SVector SGUIObject::getFullScreenScaling()
{
	if (layoutData.pLayout)
	{
		auto scaling = SVector(layoutScreenScale.x, layoutScreenScale.y) * layoutData.pLayout->getFullScreenScaling();
		return scaling;
	}
	else
	{
		return SVector(screenScale.x, screenScale.y);
	}
}

SVector SGUIObject::getFullPosition()
{
	if (layoutData.pLayout)
	{
		SVector offsetFromLayoutCenter(pos.x, pos.y);
		SVector layoutCenter = layoutData.pLayout->getFullPosition();
		return offsetFromLayoutCenter + layoutCenter;
	}
	else
	{
		return SVector(pos.x, pos.y);
	}
}

SGUIObject::SGUIObject(const std::string& sObjectName)
{
	this->sObjectName = sObjectName;

	layoutData.pLayout = nullptr;
	layoutData.iRatio = 0;

	iZLayer = 0;
	fRotationInRad = 0.0f;

	vSizeToKeep = SVector(-1.0f, -1.0f);

	scale = DirectX::XMFLOAT2(1.0f, 1.0f);
	screenScale = DirectX::XMFLOAT2(1.0f, 1.0f);
	layoutScreenScale = DirectX::XMFLOAT2(1.0f, 1.0f);
	color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pos = DirectX::SimpleMath::Vector2(0.5f, 0.5f);
	origin = DirectX::SimpleMath::Vector2(0.5f, 0.5f);

	bIsVisible = false;
	bIsRegistered = false;
	bToBeUsedInLayout = false;

	objectType = SGUIType::SGT_NONE;
}

void SGUIObject::setSizeToKeep(const SVector& vSizeToKeep)
{
	if (objectType == SGUIType::SGT_LAYOUT)
	{
		SError::showErrorMessageBoxAndLog("this function does nothing for layout, the size to keep is taken from the size passed in constructor.");
		return;
	}

	if (vSizeToKeep.getX() < 0.0f || vSizeToKeep.getX() > 1.0f || vSizeToKeep.getY() < 0.0f || vSizeToKeep.getY() > 1.0f)
	{
		SError::showErrorMessageBoxAndLog("size values should be normalized.");
		return;
	}

	if (bToBeUsedInLayout)
	{
		SError::showErrorMessageBoxAndLog("size values are controlled by the parent layout.");
		return;
	}

	this->vSizeToKeep = vSizeToKeep;

	recalculateSizeToKeepScaling();
}

void SGUIObject::setVisible(bool bIsVisible)
{
	this->bIsVisible = bIsVisible;

	if (bIsRegistered && bToBeUsedInLayout && layoutData.pLayout == nullptr)
	{
		SError::showErrorMessageBoxAndLog("the object is said to be used in the layout but no layout was set (GUI object '" + sObjectName + "').");
		return;
	}
}

void SGUIObject::setPosition(const SVector& vPos)
{
	if (layoutData.pLayout)
	{
		SError::showErrorMessageBoxAndLog("can't change position of this object because it's in the layout, the layout controlls the position of this object.");
	}

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
	if (objectType == SGUIType::SGT_LAYOUT)
	{
		SError::showErrorMessageBoxAndLog("can't change the origin of the layout.");
	}

	if (layoutData.pLayout)
	{
		SError::showErrorMessageBoxAndLog("can't change the origin of this object because it's in the layout, the layout controlls the position of this object.");
	}

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
	if (layoutData.pLayout)
	{
		return bIsVisible && layoutData.pLayout->isVisible();
	}
	else
	{
		return bIsVisible;
	}
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