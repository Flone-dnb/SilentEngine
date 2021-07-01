// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <vector>
#include <mutex>

// Custom
#include "SilentEngine/Private/GUI/SGUIObject/SGUIObject.h"

#if defined(DEBUG) || defined(_DEBUG)
class SGUIImage;
#endif

struct SLayoutChild
{
	SGUIObject* pChild;
	int iRatio;
};

enum class SLayoutType
{
	SLT_HORIZONTAL,
	SLT_VERTICAL,
};

//@@Class
/*
This class represents a layout that can have child GUI object.
Layout positions all childs horizontally or vertically and forces them to maintain the specified proportions relative to each other.
*/
class SGUILayout : public SGUIObject
{
public:

	//@@Function
	/*
	* desc: creates a layout with the specified width and height (in normalized range: [0, 1]).
	* param "sObjectName": name of this object.
	* param "fWidth": width to keep of this layout (in normalized range: [0, 1]).
	* param "fHeight": height to keep of this layout (in normalized range: [0, 1]).
	* param "layoutType": type of this layout.
	* param "bExpandItems": whether to stretch layout items so they will fill the whole layout space or don't stretch them to keep their original size.
	* remarks: if this layout will be used in another layout the width and the height will be ignored.
	*/
	SGUILayout(const std::string& sObjectName, float fWidth, float fHeight, SLayoutType layoutType, bool bExpandItems);
	SGUILayout() = delete;
	SGUILayout(const SGUILayout&) = delete;
	SGUILayout& operator= (const SGUILayout&) = delete;
	virtual ~SGUILayout();

	//@@Function
	/*
	* desc: adds the object as a child to this layout.
	* param "pChildObject": child object to add.
	* param "fRatio": the ratio that this child object will take in the layout. Ignored when 'bExpandItems' is 'false'.
	* remarks: for example, add 2 child objects with ratios 1 and 1, this means that the first one will take 50% of the layout space,
	and the second one also takes 50% of the layout space. Another example, add 2 child objects with ratios 2 and 1, this means that the first
	one will take (2 / (2 + 1)) = 66% of the layout space, and the other one 33% of the layout space.
	*/
	void addChild(SGUIObject* pChildObject, int iRatio);

	//@@Function
	/*
	* desc: use to set the position of the object (in normalized range: [0, 1]) on the screen.
	*/
	virtual void setPosition(const SVector& vPos) override;

	//@@Function
	/*
	* desc: use to set the scaling of the GUI object.
	*/
	virtual void setScale(const SVector& vScale) override;

#if defined(DEBUG) || defined(_DEBUG)
	//@@Function
	/*
	* desc: draws an image filling the whole layout zone to help visualize the layout bounds.
	* return: this function is only available in the debug mode.
	*/
	void setDrawDebugLayoutFillImage(bool bDraw, const SVector& vFillImageColor);
#endif

	//@@Function
	/*
	* desc: removes the object from this layout.
	* return: false if successful, true if the object is not a child of this layout.
	* remarks: the child will be invisible after this call.
	*/
	bool removeChild(SGUIObject* pChildObject);

	//@@Function
	/*
	* desc: removes all childs from this layout.
	* remarks: all childs will be invisible after this call.
	*/
	void removeAllChilds();

	//@@Function
	/*
	* desc: returns the size of the GUI object without scaling.
	*/
	virtual SVector getSizeInPixels() override;

	//@@Function
	/*
	* desc: returnes all childs of this layout.
	*/
	std::vector<SLayoutChild>* getChilds();

protected:

	friend class SGUIObject;
	friend class SApplication;

	virtual void setViewport(D3D12_VIEWPORT viewport) override;
	virtual void onMSAAChange() override;
	virtual bool checkRequiredResourcesBeforeRegister() override;
	// call under mtxChilds
	virtual void recalculateSizeToKeepScaling() override;
	virtual SVector getFullSizeInPixels() override;

	std::vector<SLayoutChild> vChilds;

#if defined(DEBUG) || defined(_DEBUG)
	SGUIImage* pDebugLayoutFillImage = nullptr;
#endif

	SLayoutType layoutType;

	float fWidth;
	float fHeight;

	std::mutex mtxChilds;

	bool bExpandItems;
};