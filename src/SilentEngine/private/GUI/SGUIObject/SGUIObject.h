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
	* desc: use to set the size (in normalized range: [0, 1]) that this GUI object should keep relative to the screen resolution.
	* remarks: note that 'size to keep' ignores rotation! Setting the size to (0.5f, 1.0f) will tell the object to keep its
	size as half of the screen, so on 1920x1080 resolution
	the size of this object will be 960x1080, and on the 800x600 resolution, its size will be 400x600. The size is measured from the top-left corner
	of the object. To keep the specified size, the object will use an additional scaling (on top of the scale passed in setScale()) to maintain the
	specified size.
	*/
	void setSizeToKeep(const SVector& vSizeToKeep);
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
	* remarks: note that 'size to keep' ignores rotation!
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
	* desc: returns the Z-layer value of this object.
	*/
	int     getZLayer () const;
	//@@Function
	/*
	* desc: returns the size of the GUI object without scaling.
	*/
	virtual SVector getSizeInPixels() const = 0;

protected:

	friend class SApplication;

	//@@Function
	SGUIObject(const std::string& sObjectName);

	virtual void setViewport(D3D12_VIEWPORT viewport) = 0;
	virtual void onMSAAChange() = 0;
	virtual bool checkRequiredResourcesBeforeRegister() = 0;
	virtual void recalculateSizeToKeepScaling() = 0;

	SGUIType objectType;
	bool bIsRegistered;

	DirectX::SimpleMath::Vector2 origin;
	DirectX::SimpleMath::Vector2 pos;
	DirectX::XMFLOAT2 scale;
	DirectX::XMFLOAT2 screenScale; // used for 'size to keep'
	DirectX::XMFLOAT4 color;
	SVector vSizeToKeep;

	std::string  sObjectName;

	int          iZLayer;

	float        fRotationInRad;

	bool         bIsVisible;
};

