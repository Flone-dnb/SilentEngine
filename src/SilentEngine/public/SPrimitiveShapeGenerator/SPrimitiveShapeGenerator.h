// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#pragma once

// DirectX
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>

// STL
#include <vector>

// Custom
#include "SilentEngine/Public/SVector/SVector.h"
#include "SilentEngine/Private/SMaterial/SMaterial.h"

//@@Struct
/*
The struct represents the vertex structure used by the shaders.
*/
struct SVertex
{
	DirectX::XMFLOAT3 vPos; // 3 float values
	DirectX::XMFLOAT3 vNormal; // 3 float values
	DirectX::XMFLOAT2 vUV; // 2 float values
};

//@@Struct
/*
The struct represents mesh vertex.
*/
struct SMeshVertex
{
	//@@Function
	SMeshVertex()
	{
		this->vPosition = { 0.0f, 0.0f, 0.0f };
		this->vNormal = { 0.0f, 0.0f, 0.0f };
		this->vTangent = { 0.0f, 0.0f, 0.0f };
		this->vUV = { 0.0f, 0.0f };
	};
	//@@Function
	/*
	* desc: the constructor which initializes the mesh vertex with
	the given position and zeros all other components of the structure, except for the color, the default color is white.
	*/
	SMeshVertex(const SVector& vPosition)
	{
		this->vPosition = {vPosition.getX(), vPosition.getY(), vPosition.getZ()};
		this->vNormal   = {0.0f, 0.0f, 0.0f};
		this->vTangent  = {0.0f, 0.0f, 0.0f};
		this->vUV       = {0.0f, 0.0f};
	}
	//@@Function
	/*
	* desc: the constructor which initializes the mesh vertex with
	the given parameters.
	*/
	SMeshVertex(
		const SVector& vPosition,
		const SVector& vNormal,
		const SVector& vTangent,
		const SVector& vUV)
	{
		this->vPosition = {vPosition.getX(), vPosition.getY(), vPosition.getZ()};
		this->vNormal   = {vNormal.getX(),   vNormal.getY(),   vNormal.getZ()};
		this->vTangent  = {vTangent.getX(),  vTangent.getY(),  vTangent.getZ()};
		this->vUV       = {vUV.getX(),       vUV.getY()};
	}
	//@@Function
	/*
	* desc: the constructor which initializes the mesh vertex with
	the given parameters.
	*/
	SMeshVertex(
		float fPosX, float fPosY, float fPosZ,
		float fNormalX, float fNormalY, float fNormalZ,
		float fTangentX, float fTangentY, float fTangentZ,
		float fU, float fV)
	{
		this->vPosition = {fPosX, fPosY, fPosZ};
		this->vNormal   = {fNormalX, fNormalY, fNormalZ};
		this->vTangent  = {fTangentX, fTangentY, fTangentZ};
		this->vUV       = {fU, fV};
	}

private:

	friend class SMeshData;
	friend class SPrimitiveShapeGenerator;

	DirectX::XMFLOAT3 vPosition;
	DirectX::XMFLOAT3 vNormal;
	DirectX::XMFLOAT3 vTangent;
	DirectX::XMFLOAT2 vUV;
};


struct SMeshDataComputeResource
{
	class SComponent* pResourceOwner;
	bool bVertexBuffer;
};
//@@Struct
/*
The struct represents the 3D-geometry data.
*/
struct SMeshData
{
	//@@Function
	/*
	* desc: adds a new vertex to the mesh geometry.
	* param "vertex": vertex to add.
	*/
	void addVertex(const SMeshVertex& vertex)
	{
		vVertices.push_back(vertex);
	}

	//@@Function
	/*
	* desc: adds a new index to the mesh geometry.
	* param "iIndex": index to add.
	*/
	void addIndex(std::uint32_t iIndex)
	{
		if (iIndex > UINT16_MAX)
		{
			bHasIndicesMoreThan16Bits = true;
		}

		vIndices32.push_back(iIndex);
	}

	//@@Function
	/*
	* desc: sets the vector of vertices, erasing the old one.
	* param "vVertices": vector to add.
	*/
	void setVertices(std::vector<SMeshVertex>& vVertices)
	{
		this->vVertices = vVertices;
	}

	//@@Function
	/*
	* desc: sets the vector of indices, erasing the old one.
	* param "vIndices32": vector to add.
	*/
	void setIndices(std::vector<std::uint32_t> vIndices32)
	{
		for (size_t i = 0; i < vIndices32.size(); i++)
		{
			if (vIndices32[i] > UINT16_MAX)
			{
				bHasIndicesMoreThan16Bits = true;
				break;
			}
		}

		this->vIndices32 = vIndices32;
	}

	//@@Function
	/*
	* desc: clears (deletes) all the vertices of the mesh data.
	*/
	void clearVertices()
	{
		vVertices.clear();
	}

	//@@Function
	/*
	* desc: clears (deletes) all the indices of the mesh data.
	*/
	void clearIndices()
	{
		bHasIndicesMoreThan16Bits = false;

		vIndices32.clear();
		vIndices16.clear();
	}

	//@@Function
	/*
	* desc: returns the vertices of the mesh data.
	*/
	std::vector<SMeshVertex>& getVertices()
	{
		return vVertices;
	}

	//@@Function
	/*
	* desc: returns the index of the mesh data.
	*/
	std::uint32_t getIndexAt(size_t i)
	{
		return vIndices32[i];
	}

	//@@Function
	/*
	* desc: returns the indices in the format of std::uint16_t. This means that the highest value of the index is 65535.
	Use SMeshData::hasIndicesMoreThan16Bits() to determine if the mesh data has indices with value more than the uint16_t max value,
	and use getIndices32() if it does.
	*/
	std::vector<std::uint16_t>& getIndices16()
	{
		if(vIndices16.empty())
		{
			vIndices16.resize(vIndices32.size());

			for (size_t i = 0; i < vIndices32.size(); i++)
			{
				vIndices16[i] = static_cast<std::uint16_t>(vIndices32[i]);
			}
		}

		return vIndices16;
	}

	//@@Function
	/*
	* desc: returns the indices in the format of std::uint32_t.
	*/
	std::vector<std::uint32_t>& getIndices32()
	{
		return vIndices32;
	}

	//@@Function
	/*
	* desc: returns the number of vertices in the mesh data.
	*/
	size_t getVerticesCount() const
	{
		return vVertices.size();
	}

	//@@Function
	/*
	* desc: returns the number of indices in the mesh data.
	*/
	size_t getIndicesCount() const
	{
		return vIndices32.size();
	}

	//@@Function
	/*
	* desc: returns the vertices of the mesh data in the format used by the shaders.
	*/
	std::vector<SVertex> toShaderVertex() const
	{
		std::vector<SVertex> vShaderVertices;

		for (size_t i = 0; i < vVertices.size(); i++)
		{
			SVertex vertex;
			vertex.vPos = vVertices[i].vPosition;
			vertex.vNormal = vVertices[i].vNormal;
			vertex.vUV = vVertices[i].vUV;

			vShaderVertices.push_back(vertex);
		}

		return vShaderVertices;
	}

	//@@Function
	/*
	* desc: returns true, if the indices values of the mesh data exceed std::uint16_t max value which is 65535.
	*/
	bool hasIndicesMoreThan16Bits() const
	{
		return bHasIndicesMoreThan16Bits;
	}

private:

	//@@Function
	/*
	* desc: sets the material that this mesh will use. Used by mesh components.
	*/
	void setMeshMaterial(SMaterial* pMeshMaterial)
	{
		this->pMeshMaterial = pMeshMaterial;
	}

	//@@Function
	/*
	* desc: returns the material that this mesh is using (if the setMeshMaterial() was called before), otherwise nullptr (default engine material).
	Used by mesh components.
	*/
	SMaterial* getMeshMaterial()
	{
		if (pMeshMaterial)
		{
			return pMeshMaterial;
		}
		else
		{
			return nullptr;
		}
	}


	friend class SApplication;
	friend class SMeshComponent;
	friend class SRuntimeMeshComponent;
	friend class SContainer;
	friend class SComponent;

	// nullptr or registered original material
	SMaterial* pMeshMaterial = nullptr;

	//@@Variable
	/* the vector that contains all vertices of the mesh data. */
	std::vector<SMeshVertex>   vVertices;
	//@@Variable
	/* the vector that contains all indices of the mesh data. */
	std::vector<std::uint32_t> vIndices32;
	//@@Variable
	/* the vector that contains indices of the mesh data. This vector will be filled when the getIndices16() is called. */
	std::vector<std::uint16_t> vIndices16;

	//@@Variable
	/* true if the indices values of the mesh data exceed UINT16_MAX. */
	bool bHasIndicesMoreThan16Bits = false;
};


//@@Class
/*
The class is used to generate a primitive 3D-geometry.
*/
class SPrimitiveShapeGenerator
{
public:

	//@@Function
	/*
	* desc: returns box mesh data.
	*/
	static SMeshData createBox                (float fWidth, float fHeight, float fDepth);
	//@@Function
	/*
	* desc: returns plane mesh data.
	*/
	static SMeshData createPlane             (float fWidth, float fDepth, std::uint32_t iWidthVertexCount, std::uint32_t iDepthVertexCount);
	//@@Function
	/*
	* desc: returns sphere mesh data.
	*/
	static SMeshData createSphere             (float fRadius, std::uint32_t iSliceCount, std::uint32_t iStackCount);
	//@@Function
	/*
	* desc: returns cylinder mesh data.
	*/
	static SMeshData createCylinder           (float fBottomRadius, float fTopRadius, float fHeight, std::uint32_t iSliceCount, std::uint32_t iStackCount);
	//@@Function
	/*
	* desc: returns arrow mesh data.
	*/
	static SMeshData createArrowByPositiveX   (bool bBoxOnTheTip = false);

private:

	//@@Function
	/*
	* desc: subdivides each triangle.
	*/
	static void        subdivide              (SMeshData& meshData);
	//@@Function
	/*
	* desc: returns the mid point between two vertices.
	*/
	static SMeshVertex getMidPoint            (const SMeshVertex& v0, const SMeshVertex& v1);
	//@@Function
	/*
	* desc: creates cylinder top cap.
	*/
	static void        createCylinderTopCap   (float fBottomRadius, float fTopRadius, float fHeight, std::uint32_t iSliceCount, std::uint32_t iStackCount, SMeshData& meshData);
	//@@Function
	/*
	* desc: creates cylinder bottom cap.
	*/
	static void        createCylinderBottomCap(float fBottomRadius, float fTopRadius, float fHeight, std::uint32_t iSliceCount, std::uint32_t iStackCount, SMeshData& meshData);
};

