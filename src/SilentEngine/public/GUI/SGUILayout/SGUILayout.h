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
	* remarks: if this layout will be used in another layout the width and the height will be ignored.
	*/
	SGUILayout(const std::string& sObjectName, float fWidth, float fHeight, SLayoutType layoutType);
	SGUILayout() = delete;
	SGUILayout(const SGUILayout&) = delete;
	SGUILayout& operator= (const SGUILayout&) = delete;
	virtual ~SGUILayout();

	//@@Function
	/*
	* desc: adds the object as a child to this layout.
	* param "pChildObject": child object to add.
	* param "fRatio": the ratio that this child object will take in the layout.
	* remarks: for example, add 2 child objects with ratios 1 and 1, this means that the first one will take 50% of the layout space,
	and the second one also takes 50% of the layout space. Another example, add 2 child objects with ratios 2 and 1, this means that the first
	one will take (2 / (2 + 1)) = 66% of the layout space, and the other one 33% of the layout space.
	*/
	void addChild(SGUIObject* pChildObject, int iRatio);
	//@@Function
	/*
	* desc: removes the object from this layout.
	* return: false if successful, true if the object is not a child of this layout.
	*/
	bool removeChild(SGUIObject* pChildObject);

	//@@Function
	/*
	* desc: removes all childs from this layout.
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

	virtual void setViewport(D3D12_VIEWPORT viewport) override;
	virtual void onMSAAChange() override;
	virtual bool checkRequiredResourcesBeforeRegister() override;
	// call under mtxChilds
	virtual void recalculateSizeToKeepScaling() override;
	virtual SVector getFullSizeInPixels() override;

	std::vector<SLayoutChild> vChilds;

	SLayoutType layoutType;

	float fWidth;
	float fHeight;

	std::mutex mtxChilds;
};