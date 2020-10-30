// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#include "SPrimitiveShapeGenerator.h"

#include <algorithm>


SMeshData SPrimitiveShapeGenerator::createBox(float fWidth, float fHeight, float fDepth)
{
	SMeshData meshData;

	std::vector<SMeshVertex> vVertices(24);

	float fHalfWidth  = 0.5f * fWidth;
	float fHalfHeight = 0.5f * fHeight;
	float fHalfDepth  = 0.5f * fDepth;

	vVertices[0]  = SMeshVertex(-fHalfWidth, -fHalfDepth, -fHalfHeight, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	vVertices[1]  = SMeshVertex(-fHalfWidth, fHalfDepth,  -fHalfHeight, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	vVertices[2]  = SMeshVertex(fHalfWidth,  fHalfDepth,  -fHalfHeight, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	vVertices[3]  = SMeshVertex(fHalfWidth,  -fHalfDepth, -fHalfHeight, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	vVertices[4]  = SMeshVertex(-fHalfWidth, -fHalfDepth, fHalfHeight, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	vVertices[5]  = SMeshVertex(fHalfWidth,  -fHalfDepth, fHalfHeight, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	vVertices[6]  = SMeshVertex(fHalfWidth,  fHalfDepth,  fHalfHeight, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	vVertices[7]  = SMeshVertex(-fHalfWidth, fHalfDepth,  fHalfHeight, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	vVertices[8]  = SMeshVertex(-fHalfWidth, fHalfDepth, -fHalfHeight, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	vVertices[9]  = SMeshVertex(-fHalfWidth, fHalfDepth, fHalfHeight,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	vVertices[10] = SMeshVertex(fHalfWidth,  fHalfDepth, fHalfHeight,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	vVertices[11] = SMeshVertex(fHalfWidth,  fHalfDepth, -fHalfHeight, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	vVertices[12] = SMeshVertex(-fHalfWidth, -fHalfDepth, -fHalfHeight, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	vVertices[13] = SMeshVertex(fHalfWidth,  -fHalfDepth, -fHalfHeight, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	vVertices[14] = SMeshVertex(fHalfWidth,  -fHalfDepth, fHalfHeight,  0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	vVertices[15] = SMeshVertex(-fHalfWidth, -fHalfDepth, fHalfHeight,  0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	vVertices[16] = SMeshVertex(-fHalfWidth, -fHalfDepth, fHalfHeight,  -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
	vVertices[17] = SMeshVertex(-fHalfWidth, fHalfDepth,  fHalfHeight,  -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
	vVertices[18] = SMeshVertex(-fHalfWidth, fHalfDepth,  -fHalfHeight, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);
	vVertices[19] = SMeshVertex(-fHalfWidth, -fHalfDepth, -fHalfHeight, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);

	vVertices[20] = SMeshVertex(fHalfWidth, -fHalfDepth, -fHalfHeight, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	vVertices[21] = SMeshVertex(fHalfWidth, fHalfDepth,  -fHalfHeight, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	vVertices[22] = SMeshVertex(fHalfWidth, fHalfDepth,  fHalfHeight,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
	vVertices[23] = SMeshVertex(fHalfWidth, -fHalfDepth, fHalfHeight,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);


	meshData.setVertices(vVertices);


	std::vector<std::uint32_t> vIndices(36);

	vIndices[0]  = 0; vIndices[1]   = 1; vIndices[2]    = 2;
	vIndices[3]  = 0; vIndices[4]   = 2; vIndices[5]    = 3;

	vIndices[6]  = 4; vIndices[7]   = 5; vIndices[8]    = 6;
	vIndices[9]  = 4; vIndices[10]  = 6; vIndices[11]   = 7;

	vIndices[12] = 8; vIndices[13]  =  9; vIndices[14]  = 10;
	vIndices[15] = 8; vIndices[16]  = 10; vIndices[17]  = 11;

	vIndices[18] = 12; vIndices[19] = 13; vIndices[20] = 14;
	vIndices[21] = 12; vIndices[22] = 14; vIndices[23] = 15;

	vIndices[24] = 16; vIndices[25] = 17; vIndices[26] = 18;
	vIndices[27] = 16; vIndices[28] = 18; vIndices[29] = 19;

	vIndices[30] = 20; vIndices[31] = 21; vIndices[32] = 22;
	vIndices[33] = 20; vIndices[34] = 22; vIndices[35] = 23;


	meshData.setIndices(vIndices);

	return meshData;
}

SMeshData SPrimitiveShapeGenerator::createPlane(float fWidth, float fDepth, std::uint32_t iWidthVertexCount, std::uint32_t iDepthVertexCount)
{
	SMeshData meshData;

	std::uint32_t iVertexCount = iWidthVertexCount * iDepthVertexCount;
	
	// Create the vertices.

	float fHalfWidth = 0.5f * fWidth;
	float fHalfDepth = 0.5f * fDepth;

	float fXPolygonStep = fWidth / (iWidthVertexCount - 1);
	float fYPolygonStep = fDepth / (iDepthVertexCount - 1);

	float fDeltaUStep = 1.0f / (iWidthVertexCount - 1);
	float fDeltaVStep = 1.0f / (iDepthVertexCount - 1);

	for (std::uint32_t i = 0; i < iDepthVertexCount; i++)
	{
		float fY = fHalfDepth - i * fYPolygonStep;

		for (std::uint32_t j = 0; j < iWidthVertexCount; j++)
		{
			float fX = -fHalfWidth + j * fXPolygonStep;

			SMeshVertex vertex;
			vertex.vPosition = DirectX::XMFLOAT3(fX, fY, 0.0f);
			vertex.vNormal   = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
			vertex.vTangent  = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
			vertex.vUV       = DirectX::XMFLOAT2(j * fDeltaUStep, i * fDeltaVStep);

			meshData.addVertex(vertex);
		}
	}


	// Create the indices.

	for (std::uint32_t i = 0; i < iDepthVertexCount - 1; i++)
	{
		for (std::uint32_t j = 0; j < iWidthVertexCount - 1; j++)
		{
			meshData.addIndex(i * iWidthVertexCount + j);
			meshData.addIndex((i + 1) * iWidthVertexCount + j);
			meshData.addIndex(i * iWidthVertexCount + j + 1);

			meshData.addIndex((i + 1) * iWidthVertexCount + j);
			meshData.addIndex((i + 1) * iWidthVertexCount + j + 1);
			meshData.addIndex(i * iWidthVertexCount + j + 1);
		}
	}

	return meshData;
}

SMeshData SPrimitiveShapeGenerator::createSphere(float fRadius, std::uint32_t iSliceCount, std::uint32_t iStackCount)
{
	SMeshData meshData;

	SMeshVertex topVertex(0.0f, 0.0f, fRadius, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	SMeshVertex bottomVertex(0.0f, 0.0f, -fRadius, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	meshData.addVertex(topVertex);

	float fPhiStep   = DirectX::XM_PI / iStackCount;
	float fThetaStep = 2.0f * DirectX::XM_PI / iSliceCount;

	// Compute the vertices stating at the top pole and moving down the stacks.

	for (std::uint32_t i = 1; i <= iStackCount - 1; i++)
	{
		float fPhi = i * fPhiStep;

		// Vertices of ring.
		for (std::uint32_t j = 0; j <= iSliceCount; j++)
		{
			float fTheta = j * fThetaStep;

			SMeshVertex v;

			// Convert spherical coordinates to cartesian.
			v.vPosition.x = fRadius * sinf(fPhi) * cosf(fTheta);
			v.vPosition.y = fRadius * sinf(fPhi) * sinf(fTheta);
			v.vPosition.z = fRadius * cosf(fPhi);

			// Partial derivative of P with respect to theta.
			v.vTangent.x = -fRadius * sinf(fPhi) * sinf(fTheta);
			v.vTangent.y = fRadius * sinf(fPhi) * cosf(fTheta);
			v.vTangent.z = 0.0f;

			// Normalize tangent.
			DirectX::XMVECTOR tangent = DirectX::XMLoadFloat3(&v.vTangent);
			DirectX::XMStoreFloat3(&v.vTangent, DirectX::XMVector3Normalize(tangent));

			DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&v.vPosition);
			DirectX::XMStoreFloat3(&v.vNormal, DirectX::XMVector3Normalize(pos));

			v.vUV.x = fTheta / DirectX::XM_2PI;
			v.vUV.y = fPhi   / DirectX::XM_PI;

			meshData.addVertex(v);
		}
	}

	meshData.addVertex(bottomVertex);

	// Compute indices for top stack. The top stack was written first to the vertex buffer
	// and connects the top pole to the first ring.

	for(std::uint32_t i = 1; i <= iSliceCount; i++)
	{
		meshData.addIndex(0);
		meshData.addIndex(i);
		meshData.addIndex(i + 1);
	}

	// Compute indices for inner stacks (not connected to poles).

	std::uint32_t iStartIndex = 1;
	std::uint32_t iRingVertexCount = iSliceCount + 1;

	for (std::uint32_t i = 0; i < iStackCount - 2; i++)
	{
		for (std::uint32_t j = 0; j < iSliceCount; j++)
		{
			meshData.addIndex(iStartIndex + i * iRingVertexCount + j);
			meshData.addIndex(iStartIndex + (i + 1) * iRingVertexCount + j);
			meshData.addIndex(iStartIndex + i * iRingVertexCount + j + 1);

			meshData.addIndex(iStartIndex + (i + 1) * iRingVertexCount + j);
			meshData.addIndex(iStartIndex + (i + 1) * iRingVertexCount + j + 1);
			meshData.addIndex(iStartIndex + i * iRingVertexCount + j + 1);
		}
	}

	// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
	// and connects the bottom pole to the bottom ring.

	std::uint32_t iSouthPoleIndex = static_cast<std::uint32_t>(meshData.getVerticesCount() - 1);

	iStartIndex = iSouthPoleIndex - iRingVertexCount;

	for (std::uint32_t i = 0; i < iSliceCount; i++)
	{
		meshData.addIndex(iSouthPoleIndex);
		meshData.addIndex(iStartIndex + i + 1);
		meshData.addIndex(iStartIndex + i);
	}

	return meshData;
}

SMeshData SPrimitiveShapeGenerator::createCylinder(float fBottomRadius, float fTopRadius, float fHeight, std::uint32_t iSliceCount, std::uint32_t iStackCount)
{
	SMeshData meshData;


	// Build stacks.

	float fStackHeight = fHeight / iStackCount;

	float fRadiusStep = (fTopRadius - fBottomRadius) / iStackCount;

	std::uint32_t iRingCount = iStackCount + 1;

	for (std::uint32_t i = 0; i < iRingCount; i++)
	{
		float fZ = -0.5f * fHeight + i * fStackHeight;
		float fR = fBottomRadius + i * fRadiusStep;

		// Vertices of ring.
		float fThetaStep = 2.0f * DirectX::XM_PI / iSliceCount;
		for (std::uint32_t j = 0; j <= iSliceCount; j++)
		{
			SMeshVertex vertex;

			float fCos = cosf(j * fThetaStep);
			float fSin = sinf(j * fThetaStep);

			vertex.vPosition = DirectX::XMFLOAT3(fR * fCos, fR * fSin, fZ);

			vertex.vUV.x     = static_cast<float>(j) / iSliceCount;
			vertex.vUV.y     = 1.0f - static_cast<float>(i) / iStackCount;

			// Cylinder can be parameterized as follows, where we introduce v
			// parameter that goes in the same direction as the v tex-coord
			// so that the bitangent goes in the same direction as the v tex-coord.
			//   Let r0 be the bottom radius and let r1 be the top radius.
			//   y(v) = h - hv for v in [0,1].
			//   r(v) = r1 + (r0-r1)v
			//
			//   x(t, v) = r(v)*cos(t)
			//   y(t, v) = h - hv
			//   z(t, v) = r(v)*sin(t)
			// 
			//  dx/dt = -r(v)*sin(t)
			//  dy/dt = 0
			//  dz/dt = +r(v)*cos(t)
			//
			//  dx/dv = (r0-r1)*cos(t)
			//  dy/dv = -h
			//  dz/dv = (r0-r1)*sin(t)

			vertex.vTangent  = DirectX::XMFLOAT3(-fSin, fCos, 0.0f);

			float fRadiusDelta = fBottomRadius - fTopRadius;
			DirectX::XMFLOAT3 bitangent(fRadiusDelta * fCos, fRadiusDelta * fSin, -fHeight);

			DirectX::XMVECTOR T = DirectX::XMLoadFloat3(&vertex.vTangent);
			DirectX::XMVECTOR B = DirectX::XMLoadFloat3(&bitangent);
			DirectX::XMVECTOR N = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(T, B));
			DirectX::XMStoreFloat3(&vertex.vNormal, N);

			meshData.addVertex(vertex);
		}
	}

	// Add one because we duplicate the first and last vertex per ring
	// since the texture coordinates are different.
	std::uint32_t iRingVertexCount = iSliceCount + 1;

	// Compute indices for each stack.
	for (std::uint32_t i = 0; i < iStackCount; i++)
	{
		for (std::uint32_t j = 0; j < iSliceCount; j++)
		{
			meshData.addIndex(i * iRingVertexCount + j);
			meshData.addIndex((i + 1) * iRingVertexCount + j + 1);
			meshData.addIndex((i + 1) * iRingVertexCount + j);

			meshData.addIndex(i * iRingVertexCount + j);
			meshData.addIndex(i * iRingVertexCount + j + 1);
			meshData.addIndex((i + 1) * iRingVertexCount + j + 1);
		}
	}

	createCylinderTopCap(fBottomRadius, fTopRadius, fHeight, iSliceCount, iStackCount, meshData);
	createCylinderBottomCap(fBottomRadius, fTopRadius, fHeight, iSliceCount, iStackCount, meshData);

	return meshData;
}

SMeshData SPrimitiveShapeGenerator::createArrowByPositiveX(bool bBoxOnTheTip)
{
	SMeshData meshData;

	std::vector<SMeshVertex> vVertices;

	float fArrowWidth  = 0.25f;
	float fArrowLength = 4.0f;

	vVertices.push_back(SMeshVertex(SVector(0.0f, -fArrowWidth, -fArrowWidth)));
	vVertices.push_back(SMeshVertex(SVector(0.0f, fArrowWidth,  -fArrowWidth)));
	vVertices.push_back(SMeshVertex(SVector(0.0f, -fArrowWidth, fArrowWidth)));
	vVertices.push_back(SMeshVertex(SVector(0.0f, fArrowWidth,  fArrowWidth)));

	vVertices.push_back(SMeshVertex(SVector(fArrowLength, -fArrowWidth, -fArrowWidth)));
	vVertices.push_back(SMeshVertex(SVector(fArrowLength, fArrowWidth,  -fArrowWidth)));
	vVertices.push_back(SMeshVertex(SVector(fArrowLength, -fArrowWidth, fArrowWidth)));
	vVertices.push_back(SMeshVertex(SVector(fArrowLength, fArrowWidth,  fArrowWidth)));

	vVertices.push_back(SMeshVertex(SVector(fArrowLength,  -fArrowWidth - fArrowWidth, -fArrowWidth  - fArrowWidth)));
	vVertices.push_back(SMeshVertex(SVector(fArrowLength,  fArrowWidth  + fArrowWidth,  -fArrowWidth - fArrowWidth)));
	vVertices.push_back(SMeshVertex(SVector(fArrowLength, -fArrowWidth - fArrowWidth,  fArrowWidth  + fArrowWidth)));
	vVertices.push_back(SMeshVertex(SVector(fArrowLength, fArrowWidth  + fArrowWidth,  fArrowWidth  + fArrowWidth)));

	if (bBoxOnTheTip)
	{
		vVertices.push_back(SMeshVertex(SVector(fArrowLength + fArrowLength / 4,  -fArrowWidth - fArrowWidth, -fArrowWidth  - fArrowWidth)));
		vVertices.push_back(SMeshVertex(SVector(fArrowLength + fArrowLength / 4,  fArrowWidth  + fArrowWidth,  -fArrowWidth - fArrowWidth)));
		vVertices.push_back(SMeshVertex(SVector(fArrowLength + fArrowLength / 4, -fArrowWidth - fArrowWidth,  fArrowWidth  + fArrowWidth)));
		vVertices.push_back(SMeshVertex(SVector(fArrowLength + fArrowLength / 4, fArrowWidth  + fArrowWidth,  fArrowWidth  + fArrowWidth)));
	}
	else
	{
		vVertices.push_back(SMeshVertex(SVector(fArrowLength + fArrowLength / 2, 0.0f, 0.0f)));
	}

	meshData.setVertices(vVertices);


	std::vector<std::uint32_t> vIndices(54);

	vIndices[0]  = 0; vIndices[1]   = 2; vIndices[2]    = 1;
	vIndices[3]  = 2; vIndices[4]   = 3; vIndices[5]    = 1;

	vIndices[6]  = 4; vIndices[7]   = 6; vIndices[8]    = 0;
	vIndices[9]  = 6; vIndices[10]  = 2; vIndices[11]   = 0;

	vIndices[12] = 1; vIndices[13]  = 3; vIndices[14]   = 5;
	vIndices[15] = 3; vIndices[16]  = 7; vIndices[17]   = 5;

	vIndices[18]  = 2; vIndices[19]  = 6; vIndices[20]  = 7;
	vIndices[21]  = 2; vIndices[22]  = 7; vIndices[23]  = 3;

	vIndices[24] = 5; vIndices[25]  = 4; vIndices[26]   = 0;
	vIndices[27] = 1; vIndices[28]  = 5; vIndices[29]   = 0;

	vIndices[30] = 5; vIndices[31]  = 7; vIndices[32]   = 4;
	vIndices[33] = 7; vIndices[34]  = 6; vIndices[35]   = 4;

	vIndices[36] = 8; vIndices[37]  = 10; vIndices[38]  = 9;
	vIndices[39] = 10; vIndices[40] = 11; vIndices[41]  = 9;

	if (bBoxOnTheTip)
	{
		vIndices[42] = 8;   vIndices[43] = 12; vIndices[44] = 14;
		vIndices[45] = 8;   vIndices[46] = 14; vIndices[47] = 10;

		vIndices[48] = 13;  vIndices[49] = 9;  vIndices[50] = 11;
		vIndices[51] = 13;  vIndices[52] = 11; vIndices[53] = 15;

		vIndices.resize(vIndices.size() + 18);

		vIndices[54] = 8;  vIndices[55] = 9;   vIndices[56] = 13;
		vIndices[57] = 8;  vIndices[58] = 13;  vIndices[59] = 12;

		vIndices[60] = 10; vIndices[61] = 14;  vIndices[62] = 15;
		vIndices[63] = 10; vIndices[64] = 15;  vIndices[65] = 11;

		vIndices[66] = 12; vIndices[67] = 13;  vIndices[68] = 15;
		vIndices[69] = 12; vIndices[70] = 15;  vIndices[71] = 14;
	}
	else
	{
		vIndices[42] = 12;  vIndices[43] = 10; vIndices[44] = 8;
		vIndices[45] = 12;  vIndices[46] = 8;  vIndices[47] = 9;
		vIndices[48] = 9;   vIndices[49] = 11; vIndices[50] = 12;
		vIndices[51] = 11;  vIndices[52] = 10; vIndices[53] = 12;
	}


	meshData.setIndices(vIndices);

	return meshData;
}

void SPrimitiveShapeGenerator::subdivide(SMeshData& meshData)
{
	SMeshData inputCopy = meshData;

	meshData.clearVertices();
	meshData.clearIndices();

	//       v1
	//       *
	//      / \
    //     /   \
	// m0 *-----* m1
    //   / \   / \
	//  /   \ /   \
	// *-----*-----*
    // v0    m2      v2

	std::uint32_t iTrisCount = static_cast<std::uint32_t>(inputCopy.getIndicesCount() / 3);

	for (std::uint32_t i = 0; i < iTrisCount; i++)
	{
		SMeshVertex v0 = inputCopy.getVertices()[ inputCopy.getIndices32()[i * 3 + 0] ];
		SMeshVertex v1 = inputCopy.getVertices()[ inputCopy.getIndices32()[i * 3 + 1] ];
		SMeshVertex v2 = inputCopy.getVertices()[ inputCopy.getIndices32()[i * 3 + 2] ];


		// Generate the midpoints.

		SMeshVertex m0 = getMidPoint(v0, v1);
		SMeshVertex m1 = getMidPoint(v1, v2);
		SMeshVertex m2 = getMidPoint(v0, v2);


		// Add new geometry.

		meshData.addVertex(v0); // 0
		meshData.addVertex(v1); // 1
		meshData.addVertex(v2); // 2
		meshData.addVertex(m0); // 3
		meshData.addVertex(m1); // 4
		meshData.addVertex(m2); // 5

		meshData.addIndex(i * 6 + 0);
		meshData.addIndex(i * 6 + 3);
		meshData.addIndex(i * 6 + 5);

		meshData.addIndex(i * 6 + 3);
		meshData.addIndex(i * 6 + 4);
		meshData.addIndex(i * 6 + 5);

		meshData.addIndex(i * 6 + 5);
		meshData.addIndex(i * 6 + 4);
		meshData.addIndex(i * 6 + 2);

		meshData.addIndex(i * 6 + 3);
		meshData.addIndex(i * 6 + 1);
		meshData.addIndex(i * 6 + 4);
	}
}

SMeshVertex SPrimitiveShapeGenerator::getMidPoint(const SMeshVertex& v0, const SMeshVertex& v1)
{
	DirectX::XMVECTOR vPos0  = DirectX::XMLoadFloat3(&v0.vPosition);
	DirectX::XMVECTOR vPos1  = DirectX::XMLoadFloat3(&v1.vPosition);

	DirectX::XMVECTOR vNorm0 = DirectX::XMLoadFloat3(&v0.vNormal);
	DirectX::XMVECTOR vNorm1 = DirectX::XMLoadFloat3(&v1.vNormal);

	DirectX::XMVECTOR vTan0  = DirectX::XMLoadFloat3(&v0.vTangent);
	DirectX::XMVECTOR vTan1  = DirectX::XMLoadFloat3(&v1.vTangent);

	DirectX::XMVECTOR vUV0   = DirectX::XMLoadFloat2(&v0.vUV);
	DirectX::XMVECTOR vUV1   = DirectX::XMLoadFloat2(&v1.vUV);


	// Below we are using the "*" and "+" operators.
	using namespace DirectX;
	// Microsoft provides the operator overloads in DirectXMathVector.inl header,
	// which is included at the end of DirectXMath.h. However, to be able to use it, you must have "using namespace DirectX"
	// in the scope where you're trying to use the operator.

	// Compute the midpoints of all the attributes.
	// Vectors need to be normalized since linear interpolating can make them not unit length.

	DirectX::XMVECTOR vPos     = 0.5f * (vPos0 + vPos1);
	DirectX::XMVECTOR vNormal  = DirectX::XMVector3Normalize(0.5f * (vNorm0 + vNorm1));
	DirectX::XMVECTOR vTangent = DirectX::XMVector3Normalize(0.5f * (vTan0 + vTan1));
	DirectX::XMVECTOR vUV      = 0.5f * (vUV0 + vUV1);

	SMeshVertex vOutVertex;
	DirectX::XMStoreFloat3(&vOutVertex.vPosition, vPos);
	DirectX::XMStoreFloat3(&vOutVertex.vNormal,   vNormal);
	DirectX::XMStoreFloat3(&vOutVertex.vTangent,  vTangent);
	DirectX::XMStoreFloat2(&vOutVertex.vUV,       vUV);

	return vOutVertex;
}

void SPrimitiveShapeGenerator::createCylinderTopCap(float fBottomRadius, float fTopRadius, float fHeight, std::uint32_t iSliceCount, std::uint32_t iStackCount, SMeshData& meshData)
{
	std::uint32_t iStartIndex = static_cast<uint32_t>(meshData.getVerticesCount());

	float fZ = 0.5f * fHeight;
	float fThetaStep = 2.0f * DirectX::XM_PI / iSliceCount;

	// Duplicate cap ring vertices because the texture coordinates and normals differ.
	for (std::uint32_t i = 0; i <= iSliceCount; i++)
	{
		float fX = fTopRadius * cosf(i * fThetaStep);
		float fY = fTopRadius * sinf(i * fThetaStep);

		// Scale down by the height to try and make top cap texture coord area proportional to base.
		float fU = fX / fHeight + 0.5f;
		float fV = fY / fHeight + 0.5f;

		meshData.addVertex( SMeshVertex(fX, fY, fZ, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, fU, fV) );
	}

	// Cap center vertex.
	meshData.addVertex( SMeshVertex(0.0f, 0.0f, fZ, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f) );

	// Index of center vertex.
	std::uint32_t iCenterIndex = static_cast<std::uint32_t>(meshData.getVerticesCount()) - 1;

	for (std::uint32_t i = 0; i < iSliceCount; i++)
	{
		meshData.addIndex(iCenterIndex);
		meshData.addIndex(iStartIndex + i);
		meshData.addIndex(iStartIndex + i + 1);
	}
}

void SPrimitiveShapeGenerator::createCylinderBottomCap(float fBottomRadius, float fTopRadius, float fHeight, std::uint32_t iSliceCount, std::uint32_t iStackCount, SMeshData& meshData)
{
	std::uint32_t iStartIndex = static_cast<uint32_t>(meshData.getVerticesCount());

	float fZ = -0.5f * fHeight;
	float fThetaStep = 2.0f * DirectX::XM_PI / iSliceCount;

	// Duplicate cap ring vertices because the texture coordinates and normals differ.
	for (std::uint32_t i = 0; i <= iSliceCount; i++)
	{
		float fX = fBottomRadius * cosf(i * fThetaStep);
		float fY = fBottomRadius * sinf(i * fThetaStep);

		// Scale down by the height to try and make top cap texture coord area proportional to base.
		float fU = fX / fHeight + 0.5f;
		float fV = fY / fHeight + 0.5f;

		meshData.addVertex( SMeshVertex(fX, fY, fZ, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, fU, fV) );
	}

	// Cap center vertex.
	meshData.addVertex( SMeshVertex(0.0f, 0.0f, fZ, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f) );

	// Index of center vertex.
	std::uint32_t iCenterIndex = static_cast<std::uint32_t>(meshData.getVerticesCount()) - 1;

	for (std::uint32_t i = 0; i < iSliceCount; i++)
	{
		meshData.addIndex(iCenterIndex);
		meshData.addIndex(iStartIndex + i + 1);
		meshData.addIndex(iStartIndex + i);
	}
}
