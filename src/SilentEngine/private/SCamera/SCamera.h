// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <mutex>

// DirectX
#include <DirectXMath.h>

// Custom
#include "SilentEngine/Public/SVector/SVector.h"
#include "SilentEngine/Private/SMath/SMath.h"

enum SCAMERA_MODE
{
	CM_FREE = 0,
	CM_FIXED = 1
};

struct STextureBlur
{
	// Default: false - no blur.
	// (used to blur the screen)
	bool bEnableScreenBlur = false;

	// Default: 4.
	// (use in range [1, ...] to control the overall blurriness.
	size_t iBlurStrength = 4;
};

struct SCameraEffects
{
	// Default: 1.0f, 1.0f, 1.0f.
	// (multiplies the color of any pixel fragment)
	SVector vCameraMultiplyColor = SVector(1.0f, 1.0f, 1.0f);

	// Default: 1.0f.
	// (use to control the gamma)
	float fGamma = 1.0f;

	// Default: 1.0f - no saturation.
	// (use in range [-1.0f, ...] to control the saturation of the image)
	float fSaturation = 1.0f;

	STextureBlur screenBlurEffect;
};

class SCamera
{
public:

	SCamera();
	~SCamera();

	//@@Function
	/*
	* desc: determines the mode of the camera: free - default, fixed - the camera only moves in spherical coordinates around a target point.
	Allows you to use 'setFixedCamera...' functions if the mode == CM_FIXED.
	*/
	void setCameraMode               (SCAMERA_MODE mode);

	void moveCameraForward           (float fValue);
	void moveCameraRight             (float fValue);
	void moveCameraUp                (float fValue);
	//@@Function
	/*
	* desc: if using roll, use setDontFlipCamera() to false.
	*/
	void rotateCamera                (float fPitch, float fYaw, float fRoll);
	void rotateCameraPitch           (float fAngleInDeg);
	void rotateCameraYaw             (float fAngleInDeg);
	//@@Function
	/*
	* desc: use setDontFlipCamera() to false.
	*/
	void rotateCameraRoll            (float fAngleInDeg);
	void makeCameraLookAt            (const SVector& vTargetLocation);

	//@@Function
	/*
	* desc: use to set different camera effects like gamma shift, saturation, blur and etc.
	*/
	void setCameraEffects            (SCameraEffects cameraEffects);

	//@@Function
	/*
	* desc: when roll is not used, prevents the camera from flipping (when camera is flipped it changes the x input sign).
	If roll is used set this to false.
	* remarks: true by default.
	*/
	void setDontFlipCamera           (bool bDontFlipCamera);

	void setCameraLocationInWorld    (const SVector& vLocation);
	//@@Function
	/*
	* desc: sets the FOV of the camera, valid range is [60, 120].
	* remarks: default is 90.
	*/
	void setCameraVerticalFOV        (float fFOV);
	void setCameraNearClipPlane      (float fNearClipPlane);
	void setCameraFarClipPlane       (float fFarClipPlane);

	//@@Function
	/*
	* desc: sets the fixed camera's (if setCameraMode() is SCAMERA_MODE::CM_FIXED) zoom (radius in a spherical coordinate system).
	* param "fZoom": radius value for a spherical coordinate system, should be higher than 0.
	*/
	void    setFixedCameraZoom(float fZoom);
	//@@Function
	/*
	* desc: sets the fixed camera's (if setCameraMode() is SCAMERA_MODE::CM_FIXED) rotation (phi and theta in a spherical coordinate system).
	* param "fPhi": vertical rotation.
	* param "fTheta": horizontal rotation.
	*/
	void    setFixedCameraRotation(float fPhi, float fTheta);
	//@@Function
	/*
	* desc: sets the fixed camera's (if setCameraMode() is SCAMERA_MODE::CM_FIXED) rotation shift.
	* param "fHorizontalShift": horizontal rotation shift.
	* param "fVerticalShift": vertical rotation shift.
	*/
	void    setFixedCameraRotationShift(float fHorizontalShift, float fVerticalShift);

	void    getFixedCameraRotation(float* fPhi, float* fTheta) const;
	float   getFixedCameraZoom() const;
	void    getFixedCameraLocalAxisVector(SVector* pvXAxis, SVector* pvYAxis, SVector* pvZAxis) const;

	SVector getCameraLocationInWorld ();
	float   getCameraVerticalFOV     () const;
	float   getCameraHorizontalFOV   () const;
	float   getCameraNearClipPlane   () const;
	float   getCameraFarClipPlane    () const;
	float   getCameraAspectRatio     () const;
	float   getCameraNearClipWindowWidth () const;
	float   getCameraNearClipWindowHeight() const;
	float   getCameraFarClipWindowWidth  () const;
	float   getCameraFarClipWindowHeight () const;
	void    getCameraBasicVectors        (SVector* pvForward, SVector* pvLeft, SVector* pvZAxis);

	SCameraEffects getCameraEffects();

private:

	friend class SApplication;
	friend class SCameraComponent;

	void setCameraAspectRatio(float fAspectRatio);
	void updateViewMatrix();
	void resetCameraLocationSettings();
	void updateProjectionAndClipWindows();

	DirectX::XMMATRIX getViewMatrix();
	DirectX::XMMATRIX getProjMatrix();


	DirectX::XMFLOAT3 vLocation = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 vUpVector = { 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 vRightVector = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 vForwardVector = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 vCameraTargetPos = { 0.0f, 0.0f, 0.0f }; // used only when cameraMode == CM_FIXED

	DirectX::XMFLOAT4X4 mView = SMath::getIdentityMatrix4x4();
	DirectX::XMFLOAT4X4 mProj = SMath::getIdentityMatrix4x4();

	SCAMERA_MODE cameraMode = CM_FREE;

	SCameraEffects cameraEffects;

	std::mutex mtxLocRotView;
	std::mutex mtxCameraEffects;

	float fTheta = 1.5f * DirectX::XM_PI;
	float fPhi = DirectX::XM_PIDIV4;
	float fRadius = 5.0f;

	float fNearClipPlane = 0.3f;
	float fFarClipPlane = 1000.0f;
	float fAspectRatio = 800.0f / 600;
	float fVerticalFOV = 90.0f;
	float fNearClipWindowHeight = 0.0f;
	float fFarClipWindowHeight = 0.0f;

	bool bNeedToUpdateViewMatrix = true;
	bool bDontFlipCamera = true;
};

