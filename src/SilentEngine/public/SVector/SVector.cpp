// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SVector.h"

SVector::SVector()
{
	DirectX::XMStoreFloat3(&vVector, DirectX::XMVectorZero());
	fW  = 0.0f;
}

SVector::SVector(float fU, float fV)
{
	vVector.x = fU;
	vVector.y = fV;
	vVector.z = 1.0f;
	fW = 0.0f;
}

SVector::SVector(float fX, float fY, float fZ)
{
	vVector.x = fX;
	vVector.y = fY;
	vVector.z = fZ;
	fW  = 0.0f;
}

SVector::SVector(float fX, float fY, float fZ, float fW)
{
	vVector.x = fX;
	vVector.y = fY;
	vVector.z = fZ;
	this->fW = fW;
}

void SVector::setX(float fX)
{
	vVector.x = fX;
}

void SVector::setY(float fY)
{
	vVector.y = fY;
}

void SVector::setZ(float fZ)
{
	vVector.z = fZ;
}

void SVector::setW(float fW)
{
	this->fW = fW;
}

float SVector::getX() const
{
	return vVector.x;
}

float SVector::getY() const
{
	return vVector.y;
}

float SVector::getZ() const
{
	return vVector.z;
}

float SVector::getW() const
{
	return fW;
}

void SVector::normalizeVector()
{
	DirectX::XMVECTOR res = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&vVector));
	DirectX::XMStoreFloat3(&vVector, res);
}

float SVector::length()
{
	DirectX::XMVECTOR vec = DirectX::XMVector3Length(DirectX::XMLoadFloat3(&vVector));
	DirectX::XMFLOAT3 res;
	DirectX::XMStoreFloat3(&res, vec);

	return res.x;
}

float SVector::dotProduct(const SVector& b)
{
	DirectX::XMFLOAT3 vB = {b.getX(), b.getY(), b.getZ()};

	DirectX::XMVECTOR result = DirectX::XMVector3Dot(DirectX::XMLoadFloat3(&vVector), DirectX::XMLoadFloat3(&vB));

	DirectX::XMFLOAT3 vResult;
	DirectX::XMStoreFloat3(&vResult, result);

	return vResult.x;
}

void SVector::crossProduct(const SVector& b)
{
	DirectX::XMFLOAT3 vB = {b.getX(), b.getY(), b.getZ()};

	DirectX::XMVECTOR result = DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&vVector), DirectX::XMLoadFloat3(&vB));

	DirectX::XMStoreFloat3(&vVector, result);
}

float SVector::angleBetweenVectorsInRad(const SVector& b, bool bVectorsNormalized)
{
	DirectX::XMFLOAT3 vB = {b.getX(), b.getY(), b.getZ()};

	DirectX::XMVECTOR vec;
	if (bVectorsNormalized)
	{
		vec = DirectX::XMVector3AngleBetweenNormals(DirectX::XMLoadFloat3(&vVector), DirectX::XMLoadFloat3(&vB));
	}
	else
	{
		vec = DirectX::XMVector3AngleBetweenVectors(DirectX::XMLoadFloat3(&vVector), DirectX::XMLoadFloat3(&vB));
	}

	DirectX::XMFLOAT3 res;
	DirectX::XMStoreFloat3(&res, vec);

	return res.x;
}

float SVector::angleBetweenVectorsInDeg(const SVector& b, bool bVectorsNormalized)
{
	return DirectX::XMConvertToDegrees(angleBetweenVectorsInRad(b, bVectorsNormalized));
}

void SVector::rotateAroundAxis(const SVector& axis, float fAngleInDeg)
{
	DirectX::XMMATRIX rotate = DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(axis.getX(), axis.getY(), axis.getZ(), 0.0f), DirectX::XMConvertToRadians(fAngleInDeg));
	DirectX::XMVECTOR result = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&vVector), rotate);

	DirectX::XMStoreFloat3(&vVector, result);
}

SVector SVector::sphericalToCartesianCoords(float fRadius, float fTheta, float fPhi)
{
	SVector out;
	out.setX(fRadius * sinf(fPhi) * cosf(fTheta));
	out.setY(fRadius * sinf(fPhi) * sinf(fTheta));
	out.setZ(fRadius * cosf(fPhi));

	return out;
}

SVector SVector::operator+(const SVector& b)
{
	return SVector(vVector.x + b.getX(), vVector.y + b.getY(), vVector.z + b.getZ());
}

SVector SVector::operator-(const SVector& b)
{
	return SVector(vVector.x - b.getX(), vVector.y - b.getY(), vVector.z - b.getZ());
}

SVector SVector::operator*(const SVector& b)
{
	return SVector(vVector.x * b.getX(), vVector.y * b.getY(), vVector.z * b.getZ());
}

SVector SVector::operator/(const SVector& b)
{
	return SVector(vVector.x / b.getX(), vVector.y / b.getY(), vVector.z / b.getZ());
}

SVector SVector::operator+(const float& b)
{
	return SVector(vVector.x + b, vVector.y + b, vVector.z + b);
}

SVector SVector::operator-(const float& b)
{
	return SVector(vVector.x - b, vVector.y - b, vVector.z - b);
}

SVector SVector::operator*(const float& b)
{
	return SVector(vVector.x * b, vVector.y * b, vVector.z * b);
}

SVector SVector::operator/(const float& b)
{
	return SVector(vVector.x / b, vVector.y / b, vVector.z / b);
}

bool SVector::operator==(const SVector& b)
{
	DirectX::XMFLOAT3 vB = {b.getX(), b.getY(), b.getZ()};

	return DirectX::XMVector3Equal(DirectX::XMLoadFloat3(&vVector), DirectX::XMLoadFloat3(&vB));
}