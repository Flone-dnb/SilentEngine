// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <string>
#include <vector>
#include <string_view>
#include <mutex>

// Custom
#include "SilentEngine/Private/GUI/SGUIObject/SGUIObject.h"

//@@Class
/*
This class represents an image that can be used in GUI.
*/
class SGUIImage : public SGUIObject
{
public:

	//@@Function
	SGUIImage(const std::string& sObjectName);
	SGUIImage() = delete;
	SGUIImage(const SGUIImage&) = delete;
	SGUIImage& operator= (const SGUIImage&) = delete;
	virtual ~SGUIImage();

	//@@Function
	/*
	* desc: loads an image to be used in GUI (png, jpeg, tiff, dds).
	* return: false if successful, true otherwise.
	* remarks: you can call this function again after the GUI object was registered to set a new image without needing to register again.
	*/
	bool loadImage(std::wstring_view sPathToImage);

	//@@Function
	/*
	* desc: use to specify the rectangle (in order: left, top, right, bottom corner) for drawing just part of a GUI object (in normalized range: [0, 1]).
	* remarks: for example, passing the (x: 0.0f, y: 0.0f, z: 0.5f, w: 0.5f) will cut the GUI object to render only left-top corner of the object
	(all relative to top-left corner, not the origin).
	*/
	void setCut(const SVector& vSourceRect);

	//@@Function
	/*
	* desc: use to set the rotation in degrees.
	* remarks: it's better to use the rotation only for a square images. Note that 'size to keep' ignores the rotation.
	*/
	virtual void setRotation(float fRotationInDeg);

	//@@Function
	/*
	* desc: returns the rotation in degrees.
	*/
	float getRotation() const;

	//@@Function
	/*
	* desc: returns the size of the GUI object without scaling.
	*/
	virtual SVector getSizeInPixels();

protected:

	virtual void setViewport(D3D12_VIEWPORT viewport) override;
	virtual void onMSAAChange() override;
	virtual bool checkRequiredResourcesBeforeRegister() override;
	// call under mtxSprite
	virtual void recalculateSizeToKeepScaling() override;
	virtual SVector getFullSizeInPixels() override;

private:

	friend class SApplication;

	std::unique_ptr<DirectX::SpriteBatch>  pSpriteBatch = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pTexture;

	SVector sourceRect;

	std::mutex   mtxSprite;

	std::wstring sPathToTexture;

	int          iIndexInHeap;
};

