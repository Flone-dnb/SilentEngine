// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SCamera.h"
#include <cmath>

SCamera::SCamera()
{
}

SCamera::~SCamera()
{
}

void SCamera::setCameraMode(SCAMERA_MODE mode)
{
	cameraMode = mode;

	resetCameraLocationSettings();

	updateViewMatrix();
}

void SCamera::moveCameraForward(float fValue)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	DirectX::XMVECTOR vMoveValue = DirectX::XMVectorReplicate(fValue);
	DirectX::XMVECTOR vForward = DirectX::XMLoadFloat3(&vForwardVector);
	DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(&vLocation);
	DirectX::XMStoreFloat3(&vLocation, DirectX::XMVectorMultiplyAdd(vMoveValue, vForward, vPos));

	bNeedToUpdateViewMatrix = true;
}

void SCamera::moveCameraRight(float fValue)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	fValue = -fValue;
	
	DirectX::XMVECTOR vMoveValue = DirectX::XMVectorReplicate(fValue);
	DirectX::XMVECTOR vRight = DirectX::XMLoadFloat3(&vRightVector);
	DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(&vLocation);
	DirectX::XMStoreFloat3(&vLocation, DirectX::XMVectorMultiplyAdd(vMoveValue, vRight, vPos));

	bNeedToUpdateViewMatrix = true;
}

void SCamera::moveCameraUp(float fValue)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	DirectX::XMVECTOR vMoveValue = DirectX::XMVectorReplicate(fValue);
	DirectX::XMVECTOR vUp = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	DirectX::XMVECTOR vPos = DirectX::XMLoadFloat3(&vLocation);
	DirectX::XMStoreFloat3(&vLocation, DirectX::XMVectorMultiplyAdd(vMoveValue, vUp, vPos));

	bNeedToUpdateViewMatrix = true;
}

void SCamera::rotateCamera(float fPitch, float fYaw, float fRoll)
{
	rotateCameraPitch(fPitch);
	rotateCameraYaw(fYaw);
	rotateCameraRoll(fRoll);
}

void SCamera::rotateCameraPitch(float fAngleInDeg)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	// make counterclockwise
	fAngleInDeg = -fAngleInDeg;

	if (bDontFlipCamera)
	{
		// Don't flip camera (not working when roll is used).

		SVector forward(vForwardVector.x, vForwardVector.y, vForwardVector.z);

		float fAngleUp = forward.angleBetweenVectorsInDeg(SVector(0.0f, 0.0f, 1.0f));

		if (fAngleInDeg > 0.0f)
		{
			if (fAngleInDeg - fAngleUp > 0.0f)
			{
				// Rotate full up.
				fAngleInDeg = fAngleUp;
			}
		}
		else
		{
			if (fAngleUp - fAngleInDeg > 180.0f)
			{
				// Rotate full down.
				fAngleInDeg = -(180.0f - fAngleUp);
			}
		}
	}



	// Rotate the up and the forward vector around the right vector.

	DirectX::XMMATRIX R = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&vRightVector), DirectX::XMConvertToRadians(fAngleInDeg));

	DirectX::XMStoreFloat3(&vUpVector, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&vUpVector), R));
	DirectX::XMStoreFloat3(&vForwardVector, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&vForwardVector), R));

	bNeedToUpdateViewMatrix = true;
}

void SCamera::rotateCameraYaw(float fAngleInDeg)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	// Rotate the basis vectors around the world up vector.

	DirectX::XMMATRIX R = DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(fAngleInDeg));

	DirectX::XMStoreFloat3(&vRightVector, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&vRightVector), R));
	DirectX::XMStoreFloat3(&vUpVector, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&vUpVector), R));
	DirectX::XMStoreFloat3(&vForwardVector, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&vForwardVector), R));

	bNeedToUpdateViewMatrix = true;
}

void SCamera::rotateCameraRoll(float fAngleInDeg)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	// make counterclockwise
	fAngleInDeg = -fAngleInDeg;

	// Rotate the up and the right vector around the forward vector.

	DirectX::XMMATRIX R = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&vForwardVector), DirectX::XMConvertToRadians(fAngleInDeg));

	DirectX::XMStoreFloat3(&vUpVector, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&vUpVector), R));
	DirectX::XMStoreFloat3(&vRightVector, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&vRightVector), R));

	bNeedToUpdateViewMatrix = true;
}

void SCamera::setFixedCameraRotationShift(float fHorizontalShift, float fVerticalShift)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	// Make each pixel correspond to a quarter of a degree.
	float dx = DirectX::XMConvertToRadians(0.25f * fHorizontalShift);
	float dy = DirectX::XMConvertToRadians(0.25f * fVerticalShift);

	// Update angles based on input to orbit camera around box.
	fTheta += dx;
	fPhi += -dy;

	// Restrict the angle mPhi.
	fPhi = SMath::clamp(fPhi, 0.1f, SMath::fPi - 0.1f);

	bNeedToUpdateViewMatrix = true;
}

void SCamera::getFixedCameraRotation(float* fPhi, float* fTheta) const
{
	*fPhi = this->fPhi;
	*fTheta = this->fTheta;
}

float SCamera::getFixedCameraZoom() const
{
	return fRadius;
}

void SCamera::getFixedCameraLocalAxisVector(SVector* pvXAxis, SVector* pvYAxis, SVector* pvZAxis) const
{
	if (pvXAxis)
	{
		pvXAxis->setX(mView._11);
		pvXAxis->setY(mView._12);
		pvXAxis->setZ(mView._13);
	}

	if (pvYAxis)
	{
		pvYAxis->setX(mView._21);
		pvYAxis->setY(mView._22);
		pvYAxis->setZ(mView._23);
	}

	if (pvZAxis)
	{
		pvZAxis->setX(mView._31);
		pvZAxis->setY(mView._32);
		pvZAxis->setZ(mView._33);
	}
}

SVector SCamera::getCameraLocationInWorld()
{
	mtxLocRotView.lock();

	SVector ret = SVector(vLocation.x, vLocation.y, vLocation.z);

	mtxLocRotView.unlock();

	return ret;
}

float SCamera::getCameraVerticalFOV() const
{
	return fVerticalFOV;
}

float SCamera::getCameraHorizontalFOV() const
{
	float fHalfWidth = 0.5f * getCameraNearClipWindowWidth();
	return 2.0f * atan(fHalfWidth / fNearClipPlane);
}

float SCamera::getCameraNearClipPlane() const
{
	return fNearClipPlane;
}

float SCamera::getCameraFarClipPlane() const
{
	return fFarClipPlane;
}

float SCamera::getCameraAspectRatio() const
{
	return fAspectRatio;
}

float SCamera::getCameraNearClipWindowWidth() const
{
	return fAspectRatio * fNearClipWindowHeight;
}

float SCamera::getCameraNearClipWindowHeight() const
{
	return fNearClipWindowHeight;
}

float SCamera::getCameraFarClipWindowWidth() const
{
	return fAspectRatio * fFarClipWindowHeight;
}

float SCamera::getCameraFarClipWindowHeight() const
{
	return fFarClipWindowHeight;
}

void SCamera::getCameraBasicVectors(SVector* pvForward, SVector* pvRight, SVector* pvUp)
{
	mtxLocRotView.lock();

	if (pvForward)
	{
		*pvForward = SVector(vForwardVector.x, vForwardVector.y, vForwardVector.z);
	}

	if (pvRight)
	{
		*pvRight = SVector(vRightVector.x, vRightVector.y, vRightVector.z);
	}

	if (pvUp)
	{
		*pvUp = SVector(vUpVector.x, vUpVector.y, vUpVector.z);
	}

	mtxLocRotView.unlock();
}

SCameraEffects SCamera::getCameraEffects()
{
	mtxCameraEffects.lock();
	SCameraEffects ret = cameraEffects;
	mtxCameraEffects.unlock();

	return ret;
}

void SCamera::updateViewMatrix()
{
	if (bNeedToUpdateViewMatrix)
	{
		std::lock_guard<std::mutex> lock(mtxLocRotView);

		if (cameraMode == CM_FREE)
		{
			DirectX::XMVECTOR R = DirectX::XMLoadFloat3(&vRightVector);
			DirectX::XMVECTOR U = DirectX::XMLoadFloat3(&vUpVector);
			DirectX::XMVECTOR F = DirectX::XMLoadFloat3(&vForwardVector);
			DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&vLocation);


			// cross product for world space, left-handed coords
			F = DirectX::XMVector3Normalize(F);
			U = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(R, F));
			R = DirectX::XMVector3Cross(F, U);


			// get location along axis vectors (and add minus for view matrix)
			// because they can be changed
			float x = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, R));
			float y = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, F));
			float z = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, U));

			DirectX::XMStoreFloat3(&vRightVector, R);
			DirectX::XMStoreFloat3(&vUpVector, U);
			DirectX::XMStoreFloat3(&vForwardVector, F);

			mView(0, 0) = -vRightVector.x;
			mView(1, 0) = -vRightVector.y;
			mView(2, 0) = -vRightVector.z;
			mView(3, 0) = -x;

			mView(0, 1) = vUpVector.x;
			mView(1, 1) = vUpVector.y;
			mView(2, 1) = vUpVector.z;
			mView(3, 1) = z;

			mView(0, 2) = vForwardVector.x;
			mView(1, 2) = vForwardVector.y;
			mView(2, 2) = vForwardVector.z;
			mView(3, 2) = y;

			mView(0, 3) = 0.0f;
			mView(1, 3) = 0.0f;
			mView(2, 3) = 0.0f;
			mView(3, 3) = 1.0f;
		}
		else
		{
			// Convert Spherical to Cartesian coordinates.
			vLocation.x = fRadius * sinf(fPhi) * cosf(fTheta);
			vLocation.y = fRadius * sinf(fPhi) * sinf(fTheta);
			vLocation.z = fRadius * cosf(fPhi);

			// Build the view matrix.
			DirectX::XMVECTOR pos = DirectX::XMVectorSet(vLocation.x, vLocation.y, vLocation.z, 1.0f);
			DirectX::XMVECTOR target = DirectX::XMVectorSet(vCameraTargetPos.x, vCameraTargetPos.y, vCameraTargetPos.z, 1.0f);
			DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

			DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
			DirectX::XMStoreFloat4x4(&mView, view);
		}

		updateProjectionAndClipWindows();

		bNeedToUpdateViewMatrix = false;
	}
}

void SCamera::resetCameraLocationSettings()
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	vLocation = { 0.0f, 0.0f, 1.0f };
	vUpVector = { 0.0f, 0.0f, 1.0f };
	vRightVector = { 1.0f, 0.0f, 0.0f };
	vForwardVector = { 0.0f, 1.0f, 0.0f };
	vCameraTargetPos = { 0.0f, 0.0f, 0.0f };

	mView = SMath::getIdentityMatrix4x4();
	mProj = SMath::getIdentityMatrix4x4();

	bNeedToUpdateViewMatrix = true;
}

void SCamera::updateProjectionAndClipWindows()
{
	// called in lock mtxLocRotView

	fNearClipWindowHeight = 2.0f * fNearClipPlane * tanf(0.5f * DirectX::XMConvertToRadians(fVerticalFOV));
	fFarClipWindowHeight = 2.0f * fFarClipPlane * tanf(0.5f * DirectX::XMConvertToRadians(fVerticalFOV));

	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(fVerticalFOV), fAspectRatio, fNearClipPlane, fFarClipPlane);
	XMStoreFloat4x4(&mProj, P);

	DirectX::BoundingFrustum::CreateFromMatrix(cameraBoundingFrustum, P);
}

DirectX::XMMATRIX SCamera::getViewMatrix()
{
	mtxLocRotView.lock();

	DirectX::XMMATRIX ret = DirectX::XMLoadFloat4x4(&mView);

	mtxLocRotView.unlock();

	return ret;
}

DirectX::XMMATRIX SCamera::getProjMatrix()
{
	mtxLocRotView.lock();

	DirectX::XMMATRIX ret = DirectX::XMLoadFloat4x4(&mProj);

	mtxLocRotView.unlock();

	return ret;
}

void SCamera::makeCameraLookAt(const SVector& vTargetLocation)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	DirectX::XMVECTOR target = DirectX::XMVectorSet(vTargetLocation.getX(), vTargetLocation.getY(), vTargetLocation.getZ(), 0.0f);
	DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&vLocation);
	DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	DirectX::XMVECTOR F = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target, pos));
	DirectX::XMVECTOR R = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(F, worldUp));
	DirectX::XMVECTOR U = DirectX::XMVector3Cross(R, F);

	DirectX::XMStoreFloat3(&vForwardVector, F);
	DirectX::XMStoreFloat3(&vRightVector, R);
	DirectX::XMStoreFloat3(&vUpVector, U);

	bNeedToUpdateViewMatrix = true;
}

void SCamera::setCameraEffects(SCameraEffects cameraEffects)
{
	mtxCameraEffects.lock();
	this->cameraEffects = cameraEffects;
	mtxCameraEffects.unlock();
}

void SCamera::setDontFlipCamera(bool bDontFlipCamera)
{
	this->bDontFlipCamera = bDontFlipCamera;
}

void SCamera::setCameraLocationInWorld(const SVector& vLocation)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	this->vLocation.x = vLocation.getX();
	this->vLocation.y = vLocation.getY();
	this->vLocation.z = vLocation.getZ();

	bNeedToUpdateViewMatrix = true;
}

void SCamera::setCameraVerticalFOV(float fFOV)
{
	if (fFOV >= 60.0f && fFOV <= 120.0f)
	{
		std::lock_guard<std::mutex> lock(mtxLocRotView);

		fVerticalFOV = fFOV;

		updateProjectionAndClipWindows();
	}
}

void SCamera::setCameraNearClipPlane(float fNearClipPlane)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	this->fNearClipPlane = fNearClipPlane;

	updateProjectionAndClipWindows();
}

void SCamera::setCameraFarClipPlane(float fFarClipPlane)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	this->fFarClipPlane = fFarClipPlane;

	updateProjectionAndClipWindows();
}

void SCamera::setFixedCameraZoom(float fZoom)
{
	if (fZoom > 0.0f)
	{
		std::lock_guard<std::mutex> lock(mtxLocRotView);

		fRadius = fZoom;

		bNeedToUpdateViewMatrix = true;
	}
}

void SCamera::setFixedCameraRotation(float fPhi, float fTheta)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	this->fPhi = fPhi;
	this->fTheta = fTheta;

	bNeedToUpdateViewMatrix = true;
}

void SCamera::setCameraAspectRatio(float fAspectRatio)
{
	std::lock_guard<std::mutex> lock(mtxLocRotView);

	this->fAspectRatio = fAspectRatio;

	updateProjectionAndClipWindows();
}
