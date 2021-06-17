// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <string_view>

// DirectX
#include <wrl.h> // smart pointers
#include <d3d12.h>

// DirectXTK
#include "SimpleMath.h"
#include "SpriteBatch.h"

// Custom
#include "SilentEngine/Public/SVector/SVector.h"

class SGUIObject;

class SGUILayer
{
public:
	int iLayer = 0;
	std::vector<SGUIObject*> vGUIObjects;
};

enum class SGUIType
{
	SGT_NONE,
	SGT_IMAGE,
	SGT_SIMPLE_TEXT,
};

//@@Class
/*
This class is a base class for all GUI objects.
*/
class SGUIObject
{
public:
	SGUIObject() = delete;
	SGUIObject(const SGUIObject&) = delete;
	SGUIObject& operator= (const SGUIObject&) = delete;

	virtual ~SGUIObject();


	//@@Function
	/*
	* desc: use to show or hide the GUI object from the screen.
	*/
	void setVisible(bool bIsVisible);
	//@@Function
	/*
	* desc: use to set the position of the object (in normalized range: [0, 1]) on the screen.
	*/
	void setPosition(const SVector& vPos);
	//@@Function
	/*
	* desc: use to set the rotation in degrees.
	*/
	void setRotation(float fRotationInDeg);
	//@@Function
	/*
	* desc: use to set the scaling of the GUI object.
	*/
	void setScale(const SVector& vScale);
	//@@Function
	/*
	* desc: use to set the tinting in RGBA format, white (1, 1, 1, 1) for no tinting.
	*/
	void setTint (const SVector& vColor);
	//@@Function
	/*
	* desc: use to flip the GUI object.
	*/
	void setFlip(bool bFlipHorizontally, bool bFlipVertically);
	//@@Function
	/*
	* desc: use to specify the rectangle (in order: left, top, right, bottom corner) for drawing just part of a GUI object (in normalized range: [0, 1]).
	* remarks: does not have any effect on SGUISimpleText. Example, passing the (x: 0.0f, y: 0.0f, z: 0.5f, w: 0.5f) will cut the GUI object to render only left-top corner of the object
	(all relative to top-left corner, not the origin).
	*/
	void setCut  (const SVector& vSourceRect);
	//@@Function
	/*
	* desc: use to set the custom origin of the GUI object (in normalized range: [0, 1]),
	by default the origin point of every GUI object is in the center.
	* remarks: the origin point is used in such operations as translation, rotation and scaling, just like with usual object in 3D.
	*/
	void setCustomOrigin(const SVector& vOrigin);
	//@@Function
	/*
	* desc: use to set the Z-layer, GUI objects with bigger layer value will be rendered on top of the objects with lower layer value.
	* param "iZLayer": positive layer value.
	* remarks: by default all GUI objects have Z-layer index equal to 0 (lowest layer).
	*/
	void setZLayer      (int iZLayer);

	//@@Function
	/*
	* desc: returns true if the GUI object is visible, false if hidden.
	*/
	bool isVisible      () const;
	//@@Function
	/*
	* desc: returns the position in normalized range [0, 1].
	*/
	SVector getPosition () const;
	//@@Function
	/*
	* desc: returns the rotation in degrees.
	*/
	float getRotation () const;
	//@@Function
	/*
	* desc: returns the scaling in degrees.
	*/
	SVector getScaling() const;
	//@@Function
	/*
	* desc: returns the tinting in RGBA format.
	*/
	SVector getTint   () const;
	//@@Function
	/*
	* desc: returns the origin point of the GUI object in normalized range [0, 1].
	*/
	SVector getOrigin () const;
	//@@Function
	/*
	* desc: returns the flip state of the object.
	*/
	void    getFlip   (bool& bFlippedHorizontally, bool& bFlippedVertically) const;
	//@@Function
	/*
	* desc: returns the Z-layer value of this object.
	*/
	int     getZLayer () const;

protected:

	friend class SApplication;

	//@@Function
	SGUIObject(const std::string& sObjectName);

	virtual void setViewport(D3D12_VIEWPORT viewport) = 0;
	virtual void onMSAAChange() = 0;
	virtual bool checkRequiredResourcesBeforeRegister() = 0;

	SGUIType objectType;
	bool bIsRegistered;

	DirectX::SimpleMath::Vector2 origin;
	DirectX::SimpleMath::Vector2 pos;
	SVector sourceRect;
	DirectX::XMFLOAT2 scale;
	DirectX::XMFLOAT4 color;
	DirectX::SpriteEffects effects;

	std::string  sObjectName;

	int          iZLayer;

	float        fRotationInRad;

	bool         bIsVisible;
};

