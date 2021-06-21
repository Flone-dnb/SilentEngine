// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SGUILayout.h"

// Custom
#include "SilentEngine/Private/SError/SError.h"
#include "SilentEngine/Public/SApplication/SApplication.h"

SGUILayout::SGUILayout(const std::string& sObjectName, float fWidth, float fHeight, SLayoutType layoutType, bool bStretchItems) : SGUIObject(sObjectName)
{
	objectType = SGUIType::SGT_LAYOUT;
	this->layoutType = layoutType;
	this->bStretchItems = bStretchItems;

	if (fWidth < 0.0f || fWidth > 1.0f || fHeight < 0.0f || fHeight > 1.0f)
	{
		SError::showErrorMessageBoxAndLog("the size values should be in the normalized range: [0, 1].");
	}

	this->fWidth = fWidth;
	this->fHeight = fHeight;

	this->layoutType = layoutType;

	vSizeToKeep = SVector(fWidth, fHeight);
}

SGUILayout::~SGUILayout()
{
}

void SGUILayout::addChild(SGUIObject* pChildObject, int iRatio)
{
	std::lock_guard<std::mutex> guard(mtxChilds);

	if (bIsRegistered == false)
	{
		SError::showErrorMessageBoxAndLog("the layout is not registered, register it first via SApplication::registerGUIObject() and only then add childs to it.");
		return;
	}

	if (pChildObject->bIsRegistered == false)
	{
		SError::showErrorMessageBoxAndLog("only registered GUI object can be added as childs to a layout.");
		return;
	}

	if (pChildObject->layoutData.pLayout)
	{
		SError::showErrorMessageBoxAndLog("the object already has a parent layout.");
		return;
	}

	if (pChildObject->objectType == SGUIType::SGT_LAYOUT)
	{
		SError::showErrorMessageBoxAndLog("sorry, but a layout inside of another layout is not implemented yet.");
		return;
	}

	// See if already added.
	for (size_t i = 0; i < vChilds.size(); i++)
	{
		if (vChilds[i].pChild == pChildObject)
		{
			SError::showErrorMessageBoxAndLog("the object is already a child of this layout.");
			return;
		}
	}

	vChilds.push_back(SLayoutChild{ pChildObject, iRatio });

	pChildObject->layoutData.pLayout = this;
	pChildObject->origin = DirectX::SimpleMath::Vector2(0.5f, 0.5f);

	if (bIsRegistered)
	{
		recalculateSizeToKeepScaling();
	}
}

bool SGUILayout::removeChild(SGUIObject* pChildObject)
{
	std::lock_guard<std::mutex> guard(mtxChilds);

	// See if already added.
	for (size_t i = 0; i < vChilds.size(); i++)
	{
		if (vChilds[i].pChild == pChildObject)
		{
			vChilds.erase(vChilds.begin() + i);
			pChildObject->layoutData.pLayout = nullptr;

			if (bIsRegistered)
			{
				recalculateSizeToKeepScaling();
			}

			return false;
		}
	}

	return true;
}

void SGUILayout::removeAllChilds()
{
	std::lock_guard<std::mutex> guard(mtxChilds);

	for (size_t i = 0; i < vChilds.size(); i++)
	{
		vChilds[i].pChild->layoutData.pLayout = nullptr;
		vChilds[i].pChild->screenScale = DirectX::XMFLOAT2(1.0f, 1.0f);
	}

	vChilds.clear();
}

SVector SGUILayout::getSizeInPixels()
{
	SScreenResolution res;
	SApplication::getApp()->getVideoSettings()->getCurrentScreenResolution(&res);

	float fFullWidth = static_cast<float>(fWidth * res.iWidth);
	float fFullHeight = static_cast<float>(fHeight * res.iHeight);

	return SVector(fFullWidth, fFullHeight);
}

std::vector<SLayoutChild>* SGUILayout::getChilds()
{
	return &vChilds;
}

void SGUILayout::setViewport(D3D12_VIEWPORT viewport)
{
	std::lock_guard<std::mutex> guard(mtxChilds);
	recalculateSizeToKeepScaling();
}

void SGUILayout::onMSAAChange()
{
}

bool SGUILayout::checkRequiredResourcesBeforeRegister()
{
	return false;
}

void SGUILayout::recalculateSizeToKeepScaling()
{
	if (vChilds.size() == 0)
	{
		return;
	}

	SScreenResolution res;
	SApplication::getApp()->getVideoSettings()->getCurrentScreenResolution(&res);

	float fFullWidth = static_cast<float>(fWidth * res.iWidth);
	float fFullHeight = static_cast<float>(fHeight * res.iHeight);

	fFullWidth *= scale.x;
	fFullHeight *= scale.y;

	screenScale.x = 1.0f; // no additional scaling
	screenScale.y = 1.0f; // no additional scaling

	// Calculate screenScale for every child.
	int iFullRatio = 0;
	for (size_t i = 0; i < vChilds.size(); i++)
	{
		iFullRatio += vChilds[i].iRatio;
	}

	float fPosBefore = 0.0f;

	for (size_t i = 0; i < vChilds.size(); i++)
	{
		DirectX::XMFLOAT2 vChildPos = pos;
		vChildPos.x -= fWidth / 2.0f;
		vChildPos.y -= fHeight / 2.0f;

		DirectX::XMFLOAT2 vScreenScale = DirectX::XMFLOAT2(1.0f, 1.0f);

		if (bStretchItems)
		{
			SVector vChildSize = vChilds[i].pChild->getFullSizeInPixels();

			if (layoutType == SLayoutType::SLT_HORIZONTAL)
			{
				vScreenScale.y = fFullHeight / vChildSize.getY();
				vScreenScale.x = (fFullWidth * (vChilds[i].iRatio / static_cast<float>(iFullRatio))) / vChildSize.getX();

				vChildPos.y += fHeight / 2.0f;
				vChildPos.x += fPosBefore + fWidth * (vChilds[i].iRatio / static_cast<float>(iFullRatio)) / 2.0f;

				fPosBefore = fPosBefore + fWidth * (vChilds[i].iRatio / static_cast<float>(iFullRatio));
			}
			else
			{
				vScreenScale.x = fFullWidth / vChildSize.getX();
				vScreenScale.y = (fFullHeight * (vChilds[i].iRatio / static_cast<float>(iFullRatio))) / vChildSize.getY();

				vChildPos.y += fPosBefore + fHeight * (vChilds[i].iRatio / static_cast<float>(iFullRatio)) / 2.0f;
				vChildPos.x += fWidth / 2.0f;

				fPosBefore = fPosBefore + fHeight * (vChilds[i].iRatio / static_cast<float>(iFullRatio));
			}

			vChilds[i].pChild->layoutScreenScale = vScreenScale;
		}
		else
		{
			SVector texSize = vChilds[i].pChild->getFullSizeInPixels();
			texSize.setX(texSize.getX() * vChilds[i].pChild->scale.x);
			texSize.setY(texSize.getY() * vChilds[i].pChild->scale.y);

			if (layoutType == SLayoutType::SLT_HORIZONTAL)
			{
				if (i == 0)
				{
					fPosBefore = pos.x - (fWidth / 2.0f);
				}

				//vScreenScale.y = fFullHeight / texSize.getY();
				//vScreenScale.x = vScreenScale.y;

				//vChildPos.y += fHeight / 2.0f; // y center of the layout
				vChildPos.x = fPosBefore;

				texSize = vChilds[i].pChild->getFullSizeInPixels();
				texSize.setX(texSize.getX() / res.iWidth);
				texSize.setY(texSize.getY() / res.iHeight);
				texSize.setX(texSize.getX() * vChilds[i].pChild->scale.x);
				texSize.setY(texSize.getY() * vChilds[i].pChild->scale.y);

				vChildPos.x += texSize.getX() / 2.0f; // origin

				fPosBefore += texSize.getX();// *vScreenScale.x;
			}
			else
			{
				if (i == 0)
				{
					fPosBefore = pos.y - (fHeight / 2.0f);
				}

				//vScreenScale.x = fFullWidth / texSize.getX();
				//vScreenScale.y = vScreenScale.x;

				//vChildPos.x += fWidth / 2.0f; // x center of the layout
				vChildPos.y = fPosBefore;

				texSize = vChilds[i].pChild->getFullSizeInPixels();
				texSize.setX(texSize.getX() / res.iWidth);
				texSize.setY(texSize.getY() / res.iHeight);
				texSize.setX(texSize.getX() * vChilds[i].pChild->scale.x);
				texSize.setY(texSize.getY() * vChilds[i].pChild->scale.y);

				vChildPos.y += texSize.getY() / 2.0f; // origin

				fPosBefore += texSize.getY();// *vScreenScale.y;
			}

			vChilds[i].pChild->origin = DirectX::SimpleMath::Vector2(0.0f, 0.0f);
			vChilds[i].pChild->layoutScreenScale = DirectX::XMFLOAT2(1.0f, 1.0f);
		}

		vChilds[i].pChild->pos = vChildPos - pos; // offset from the center of the layout
	}
}

SVector SGUILayout::getFullSizeInPixels()
{
	SScreenResolution res;
	SApplication::getApp()->getVideoSettings()->getCurrentScreenResolution(&res);

	float fFullWidth = static_cast<float>(fWidth * res.iWidth);
	float fFullHeight = static_cast<float>(fHeight * res.iHeight);

	fFullWidth *= scale.x;
	fFullHeight *= scale.y;

	return SVector(fFullWidth, fFullHeight);
}