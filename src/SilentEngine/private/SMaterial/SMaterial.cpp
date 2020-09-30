// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SMaterial.h"

// Custom
#include "SilentEngine/Private/SGeometry/SGeometry.h"
#include "SilentEngine/Private/SMath/SMath.h"

SMaterial::SMaterial()
{
	sMaterialName = "";

	pRegisteredOriginal = nullptr;

	bRegistered = false;
	bLastFrameResourceIndexValid = false;

	iMatCBIndex = 0;
	iDiffuseSRVHeapIndex = -1;
	iNormalSRVHeapIndex = -1;

	iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;
	iFrameResourceIndexLastUpdated = 0;

	mMatTransform = SMath::getIdentityMatrix4x4();
}

SMaterial::SMaterial(std::string sMaterialName) : SMaterial()
{
	this->sMaterialName = sMaterialName;
}

void SMaterial::setMaterialName(const std::string& sName)
{
	sMaterialName = sName;
}

void SMaterial::setMaterialProperties(const SMaterialProperties & matProps)
{
	this->matProps = matProps;

	if (bRegistered)
	{
		pRegisteredOriginal->matProps = matProps;
		pRegisteredOriginal->iUpdateCBInFrameResourceCount = SFRAME_RES_COUNT;
	}
}

std::string SMaterial::getMaterialName() const
{
	return sMaterialName;
}

SMaterialProperties SMaterial::getMaterialProperties() const
{
	return matProps;
}

void SMaterialProperties::setRoughness(float fRoughness)
{
	this->fRoughness = fRoughness;
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
