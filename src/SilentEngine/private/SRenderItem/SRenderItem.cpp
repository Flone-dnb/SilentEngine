// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#include "SRenderItem.h"


D3D12_VERTEX_BUFFER_VIEW SMeshGeometry::getVertexBufferView() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = pVertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = iVertexGraphicsObjectSizeInBytes;
	vbv.SizeInBytes = iVertexBufferSizeInBytes;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW SMeshGeometry::getIndexBufferView() const
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = pIndexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = indexFormat;
	ibv.SizeInBytes = iIndexBufferSizeInBytes;

	return ibv;
}

void SMeshGeometry::freeUploaders()
{
	pVertexBufferUploader = nullptr;
	pIndexBufferUploader = nullptr;
}
