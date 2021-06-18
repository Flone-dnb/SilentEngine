// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <string>

// DirectX
#include "SilentEngine/Private/d3dx12.h"

// DirectXTK
#include "SimpleMath.h"
#include "SpriteFont.h"

// Custom
#include "SilentEngine/Private/GUI/SGUIObject/SGUIObject.h"

//@@Class
/*
This class represents a simple text that can be displayed on screen.
*/
class SGUISimpleText : public SGUIObject
{
public:
	//@@Function
	SGUISimpleText(const std::string& sObjectName);
	SGUISimpleText() = delete;
	SGUISimpleText(const SGUISimpleText&) = delete;
	SGUISimpleText& operator= (const SGUISimpleText&) = delete;
	virtual ~SGUISimpleText();

	//@@Function
	/*
	* desc: sets up a font for the text.
	* param "sPathToSprintFont": path to the .spritefont file containing the font for the text.
	* return: false if successful, true otherwise.
	* remarks: you can call this function again after the GUI object was registered to set a new font without needing to register again.
	*/
	bool setFont(const std::wstring& sPathToSprintFont);

	//@@Function
	/*
	* desc: sets the text to display.
	*/
	void setText(const std::wstring& sText);

	//@@Function
	/*
	* desc: sets the outline of the text.
	*/
	void setDrawTextOutline(bool bDrawOutline, const SVector& vOutlineColor);

	//@@Function
	/*
	* desc: sets the text shadow rendering.
	*/
	void setDrawTextShadow(bool bDrawTextShadow);

	//@@Function
	/*
	* desc: sets the max. line width for text wrapping (in normalized range: [0.0, 1.0] that represents the screen in pixels).
	* param "fMaxLineWidth": max. width to wrap text, pass 0.0f to disable wrapping.
	* param "bAlignTextAtCenter": horizontal text alignment.
	* remarks: for example, passing the value of 0.5f will wrap the text when the width of a line will reach half of the screen width.
	*/
	void setWordWrapMaxLineWidth(float fMaxLineWidth, bool bAlignTextAtCenter = false);

	//@@Function
	/*
	* desc: returns the text.
	*/
	std::wstring getText() const;

	//@@Function
	/*
	* desc: returns the size of the GUI object without scaling.
	*/
	virtual SVector getSizeInPixels() const;

protected:

	virtual void setViewport(D3D12_VIEWPORT viewport) override;
	virtual void onMSAAChange() override;
	virtual bool checkRequiredResourcesBeforeRegister() override;
	virtual void recalculateSizeToKeepScaling() override;

private:

	friend class SApplication;

	std::wstring wrapText();
	
	void initFontResource();

	std::unique_ptr<DirectX::SpriteBatch> pSpriteBatch = nullptr;
	std::unique_ptr<DirectX::SpriteFont>  pSpriteFont = nullptr;

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;

	DirectX::XMFLOAT4 outlineColor;

	std::wstring sPathToSpriteFont;
	std::wstring sRawText;
	std::wstring sWrappedText;

	float fMaxLineWidth;

	bool bDrawOutline;
	bool bDrawShadow;
	bool bAlignTextAtCenter;
	bool bInitFontCalled;
};

