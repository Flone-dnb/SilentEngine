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

#if defined(DEBUG) || defined(_DEBUG)
#include <fstream>
#include "SilentEngine/Public/GUI/SGUIImage/SGUIImage.h"
#endif

SGUILayout::SGUILayout(const std::string& sObjectName, float fWidth, float fHeight, SLayoutType layoutType, bool bExpandItems) : SGUIObject(sObjectName)
{
	objectType = SGUIType::SGT_LAYOUT;
	this->layoutType = layoutType;
	this->bExpandItems = bExpandItems;

	if (fWidth < 0.0f || fWidth > 1.0f || fHeight < 0.0f || fHeight > 1.0f)
	{
		SError::showErrorMessageBoxAndLog("the size values should be in the normalized range: [0, 1].");
	}

	this->fWidth = fWidth;
	this->fHeight = fHeight;

	this->layoutType = layoutType;

	vSizeToKeep = SVector(fWidth, fHeight);

#if defined(DEBUG) || defined(_DEBUG)
	pDebugLayoutFillImage = new SGUIImage("layout debug image");

	const std::wstring sSampleTexPath = L"res/square_tex.png";

	std::ifstream debugImage(sSampleTexPath);
	if (debugImage.is_open() == false)
	{
		SError::showErrorMessageBoxAndLog("could not find the 'square_tex.png' texture in the 'res' folder.");
	}
	debugImage.close();

	pDebugLayoutFillImage->loadImage(sSampleTexPath);
	pDebugLayoutFillImage->setSizeToKeep(SVector(fWidth, fHeight));
	pDebugLayoutFillImage->bIsSystemObject = true;
#endif
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

void SGUILayout::setPosition(const SVector& vPos)
{
	SGUIObject::setPosition(vPos);

#if defined(DEBUG) || defined(_DEBUG)
	pDebugLayoutFillImage->setPosition(vPos);
#endif
}

void SGUILayout::setScale(const SVector& vScale)
{
	SGUIObject::setScale(vScale);

#if defined(DEBUG) || defined(_DEBUG)
	pDebugLayoutFillImage->setScale(vScale);
#endif
}

#if defined(DEBUG) || defined(_DEBUG)
void SGUILayout::setDrawDebugLayoutFillImage(bool bDraw, const SVector& vFillImageColor)
{
	if (bIsRegistered == false)
	{
		SError::showErrorMessageBoxAndLog("this function can only be called after the layout is registered.");
	}

	SVector vColor = vFillImageColor;
	vColor.setW(0.5f);
	pDebugLayoutFillImage->setTint(vColor);
	pDebugLayoutFillImage->setVisible(bDraw);
}
#endif

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
			pChildObject->bIsVisible = false;

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
		vChilds[i].pChild->bIsVisible = false;
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
	// do nothing
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
		if (bExpandItems == false)
		{
			vChilds[i].iRatio = 1; // ignore ratios
		}

		iFullRatio += vChilds[i].iRatio;
	}

	float fPosBefore = 0.0f;

	for (size_t i = 0; i < vChilds.size(); i++)
	{
		DirectX::XMFLOAT2 vChildPos = pos;
		vChildPos.x -= fWidth / 2.0f;
		vChildPos.y -= fHeight / 2.0f;

		DirectX::XMFLOAT2 vScreenScale = DirectX::XMFLOAT2(1.0f, 1.0f);

		SVector vChildSize = vChilds[i].pChild->getFullSizeInPixels();

		if (layoutType == SLayoutType::SLT_HORIZONTAL)
		{
			vScreenScale.y = fFullHeight / vChildSize.getY();
			vScreenScale.x = (fFullWidth * (vChilds[i].iRatio / static_cast<float>(iFullRatio))) / vChildSize.getX();

			if (bExpandItems)
			{
				vChildPos.x += fPosBefore + fWidth * (vChilds[i].iRatio / static_cast<float>(iFullRatio)) / 2.0f;
				vChildPos.y += fHeight / 2.0f;
			}
			else
			{
				vChildPos.x += fPosBefore;

				vChilds[i].pChild->vSizeToKeep = SVector(fWidth * (vChilds[i].iRatio / static_cast<float>(iFullRatio)), fHeight);
				vChilds[i].pChild->recalculateSizeToKeepScaling();
			}

			fPosBefore += fWidth * (vChilds[i].iRatio / static_cast<float>(iFullRatio));
		}
		else
		{
			vScreenScale.x = fFullWidth / vChildSize.getX();
			vScreenScale.y = (fFullHeight * (vChilds[i].iRatio / static_cast<float>(iFullRatio))) / vChildSize.getY();

			if (bExpandItems)
			{
				vChildPos.x += fWidth / 2.0f;
				vChildPos.y += fPosBefore + fHeight * (vChilds[i].iRatio / static_cast<float>(iFullRatio)) / 2.0f;
			}
			else
			{
				vChildPos.y += fPosBefore;

				vChilds[i].pChild->vSizeToKeep = SVector(fWidth, fHeight * (vChilds[i].iRatio / static_cast<float>(iFullRatio)));
				vChilds[i].pChild->recalculateSizeToKeepScaling();
			}

			fPosBefore += fHeight * (vChilds[i].iRatio / static_cast<float>(iFullRatio));
		}

		if (bExpandItems)
		{
			vChilds[i].pChild->layoutScreenScale = vScreenScale;
		}
		else
		{
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