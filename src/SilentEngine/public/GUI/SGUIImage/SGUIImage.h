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

protected:

	virtual void setViewport(D3D12_VIEWPORT viewport) override;
	virtual void onMSAAChange() override;
	virtual bool checkRequiredResourcesBeforeRegister() override;

private:

	friend class SApplication;

	std::unique_ptr<DirectX::SpriteBatch>  pSpriteBatch = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pTexture;

	std::mutex   mtxSprite;

	std::wstring sPathToTexture;

	int          iIndexInHeap;
};

