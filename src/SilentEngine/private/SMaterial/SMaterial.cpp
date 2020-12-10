// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SMaterial.h"

// Custom
#include "SilentEngine/Private/SRenderItem/SRenderItem.h"
#include "SilentEngine/Public/SApplication/SApplication.h"
#include "SilentEngine/Private/SMath/SMath.h"

SMaterial::SMaterial()
{
	sMaterialName = "";

	bRegistered = false;
	bLastFrameResourceIndexValid = false;

	iMatCBIndex = 0;

	iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;
	iFrameResourceIndexLastUpdated = 0;

	vMatTransform = SMath::getIdentityMatrix4x4();

	vMatUVOffset = SVector(1.0f, 1.0f, 1.0f);
	vMatUVScale  = SVector(1.0f, 1.0f, 1.0f);
	fMatRotation = 0.0f;
}

SMaterial::SMaterial(std::string sMaterialName) : SMaterial()
{
	this->sMaterialName = sMaterialName;
}

SMaterial::SMaterial(const SMaterial& mat) : SMaterial()
{
	matProps = mat.matProps;

	vMatUVOffset = mat.vMatUVOffset;
	fMatRotation = mat.fMatRotation;
	vMatUVScale = mat.vMatUVScale;

	vMatTransform = mat.vMatTransform;
}

SMaterial& SMaterial::operator=(const SMaterial& mat)
{
	matProps = mat.matProps;

	vMatUVOffset = mat.vMatUVOffset;
	fMatRotation = mat.fMatRotation;
	vMatUVScale = mat.vMatUVScale;

	vMatTransform = mat.vMatTransform;

	return *this;
}

void SMaterial::setMaterialProperties(const SMaterialProperties & matProps)
{
	if (bRegistered)
	{
		SApplication::getApp()->mtxUpdateMat.lock();

		this->matProps = matProps;

		iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

		SApplication::getApp()->mtxUpdateMat.unlock();
	}
}

bool SMaterial::setMaterialUVOffset(const SVector& vMaterialUVOffset)
{
	if (bRegistered)
	{
		if (vMaterialUVOffset.getX() < 0.0f || vMaterialUVOffset.getX() > 1.0f)
		{
			return true;
		}

		if (vMaterialUVOffset.getY() < 0.0f || vMaterialUVOffset.getY() > 1.0f)
		{
			return true;
		}

		vMatUVOffset = vMaterialUVOffset;

		updateMatTransform();

		iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;

		return false;
	}

	return true;
}

void SMaterial::setMaterialUVScale(const SVector& vMaterialUVScale)
{
	if (bRegistered)
	{
		vMatUVScale = vMaterialUVScale;

		updateMatTransform();

		iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;
	}
}

void SMaterial::setMaterialUVRotation(float fRotation)
{
	if (bRegistered)
	{
		fMatRotation = fRotation;

		updateMatTransform();

		iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;
	}
}

SVector SMaterial::getMaterialUVOffset() const
{
	if (bRegistered)
	{
		return vMatUVOffset;
	}
	else
	{
		return SVector();
	}
}

SVector SMaterial::getMaterialUVScale() const
{
	if (bRegistered)
	{
		return vMatUVScale;
	}
	else
	{
		return SVector();
	}
}

float SMaterial::getMaterialUVRotation() const
{
	if (bRegistered)
	{
		return fMatRotation;
	}
	else
	{
		return 0.0f;
	}
}

std::string SMaterial::getMaterialName() const
{
	return sMaterialName;
}

SMaterialProperties SMaterial::getMaterialProperties() const
{
	if (bRegistered)
	{
		return matProps;
	}
	else
	{
		return SMaterialProperties();
	}
}

void SMaterial::updateMatTransform()
{
	SApplication::getApp()->mtxUpdateMat.lock();

	DirectX::XMMATRIX transform = DirectX::XMMatrixIdentity() *
		DirectX::XMMatrixTranslation(-0.5f, -0.5f, 0.0f) * // move center to the origin point
		DirectX::XMMatrixScaling(vMatUVScale.getX(), vMatUVScale.getY(), vMatUVScale.getZ()) *
		DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(fMatRotation)) *
		DirectX::XMMatrixTranslation(vMatUVOffset.getX(), vMatUVOffset.getY(), vMatUVOffset.getZ()) *
		DirectX::XMMatrixTranslation(0.5f, 0.5f, 0.0f); // move center back.

	DirectX::XMStoreFloat4x4(&vMatTransform, transform);

	SApplication::getApp()->mtxUpdateMat.unlock();
}

void SMaterialProperties::setCustomTransparency(float fCustomTransparency)
{
	this->fCustomTransparency = SMath::clamp(fCustomTransparency, 0.0f, 1.0f);
}

void SMaterialProperties::setAddDiffuseMultiplierToFinalColor(const SVector& vRGBAMultiplier)
{
	vFinalDiffuseMult.x = vRGBAMultiplier.getX();
	vFinalDiffuseMult.y = vRGBAMultiplier.getY();
	vFinalDiffuseMult.z = vRGBAMultiplier.getZ();
	vFinalDiffuseMult.w = vRGBAMultiplier.getW();
}

void SMaterialProperties::setRoughness(float fRoughness)
{
	this->fRoughness = fRoughness;
}

bool SMaterialProperties::getDiffuseTexture(STextureHandle* pTextureHandle)
{
	if (diffuseTexture.getTextureName() == "")
	{
		return true;
	}
	else
	{
		*pTextureHandle = diffuseTexture;

		return false;
	}
}

float SMaterialProperties::getRoughness() const
{
	return fRoughness;
}

SVector SMaterialProperties::getDiffuseColor() const
{
	SVector vRGBA;
	vRGBA.setX(vDiffuseColor.x);
	vRGBA.setY(vDiffuseColor.y);
	vRGBA.setZ(vDiffuseColor.z);
	vRGBA.setW(vDiffuseColor.w);

	return vRGBA;
}

SVector SMaterialProperties::getSpecularColor() const
{
	SVector vRGB;
	vRGB.setX(vSpecularColor.x);
	vRGB.setY(vSpecularColor.y);
	vRGB.setZ(vSpecularColor.z);

	return vRGB;
}

void SMaterialProperties::setDiffuseColor(const SVector & vRGBA)
{
	vDiffuseColor.x = vRGBA.getX();
	vDiffuseColor.y = vRGBA.getY();
	vDiffuseColor.z = vRGBA.getZ();
	vDiffuseColor.w = vRGBA.getW();
}

void SMaterialProperties::setSpecularColor(const SVector & vRGB)
{
	vSpecularColor.x = vRGB.getX();
	vSpecularColor.y = vRGB.getY();
	vSpecularColor.z = vRGB.getZ();
}

bool SMaterialProperties::setDiffuseTexture(STextureHandle textureHandle)
{
	if (textureHandle.bRegistered)
	{
		diffuseTexture = textureHandle;

		bHasDiffuseTexture = true;

		return false;
	}
	else
	{
		return true;
	}
}

void SMaterialProperties::unbindTexture(STextureHandle textureHandle)
{
	if (diffuseTexture.getTextureName() == textureHandle.getTextureName())
	{
		diffuseTexture = STextureHandle();

		bHasDiffuseTexture = false;

		return;
	}
}

float SMaterialProperties::getCustomTransparency() const
{
	return fCustomTransparency;
}

SVector SMaterialProperties::getDiffuseMultiplierToFinalColor() const
{
	SVector vRGBAMult;

	vRGBAMult.setX(vFinalDiffuseMult.x);
	vRGBAMult.setY(vFinalDiffuseMult.y);
	vRGBAMult.setZ(vFinalDiffuseMult.z);
	vRGBAMult.setW(vFinalDiffuseMult.w);

	return vRGBAMult;
}

std::string STextureHandle::getTextureName() const
{
	return sTextureName;
}

std::wstring STextureHandle::getPathToTextureFile() const
{
	return sPathToTexture;
}

unsigned long long STextureHandle::getTextureSizeInBytesOnGPU() const
{
	if (pRefToTexture)
	{
		return pRefToTexture->iResourceSizeInBytesOnGPU;
	}
	else
	{
		return 0;
	}
}
