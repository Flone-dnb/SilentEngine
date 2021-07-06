// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SLevel.h"

// Custom
#include "SilentEngine/Public/SApplication/SApplication.h"
#include "SilentEngine/Public/EntityComponentSystem/SContainer/SContainer.h"
#include "SilentEngine/Private/EntityComponentSystem/SComponent/SComponent.h"
#include "SilentEngine/Public/EntityComponentSystem/SMeshComponent/SMeshComponent.h"
#include "SilentEngine/Public/EntityComponentSystem/SRuntimeMeshComponent/SRuntimeMeshComponent.h"

SLevel::SLevel(SApplication* pApp)
{
	this->pApp = pApp;
}

SLevel::~SLevel()
{
	std::vector<SContainer*> vAllContainers = vRenderableContainers;
	vAllContainers.insert(vAllContainers.end(), vNotRenderableContainers.begin(), vNotRenderableContainers.end());

	for (size_t i = 0; i < vAllContainers.size(); i++)
	{
		pApp->despawnContainerFromLevel(vAllContainers[i]);
	}
}

void SLevel::rayCast(SVector& vRayStartPos, SVector& vRayStopPos, std::vector<SRayCastHit>& vHitResult, const std::vector<SComponent*>& vIgnoreList)
{
	std::lock_guard<std::mutex> guard(pApp->mtxDraw);

	std::vector<SComponent*> vRenderableAllComponents = pApp->vAllRenderableSpawnedOpaqueComponents;
	vRenderableAllComponents.insert(vRenderableAllComponents.end(), pApp->vAllRenderableSpawnedTransparentComponents.begin(), pApp->vAllRenderableSpawnedTransparentComponents.end());

	for (size_t i = 0; i < vRenderableAllComponents.size(); i++)
	{
		std::mutex* pMeshPropsMtx = nullptr;
		SMeshData* pMeshData = nullptr;

		if (vRenderableAllComponents[i]->getComponentType() == SComponentType::SCT_MESH)
		{
			SMeshComponent* pMesh = dynamic_cast<SMeshComponent*>(vRenderableAllComponents[i]);
			bool bVisible = pMesh->isVisible() && pMesh->getContainer()->isVisible();

			if (bVisible == false || (pMesh->pParentComponent && pMesh->pParentComponent->bVisible == false))
			{
				continue;
			}

			if (pMesh->renderData.primitiveTopologyType == D3D_PRIMITIVE_TOPOLOGY_LINELIST)
			{
				continue; // lines have no collision
			}

			if (pMesh->getCollisionPreset() == SCollisionPreset::SCP_NO_COLLISION || pMesh->bUseInstancing)
			{
				continue;
			}

			pMeshPropsMtx = &pMesh->mtxComponentProps;
			pMeshPropsMtx->lock();
			pMeshData = pMesh->getMeshData();
		}
		else if (vRenderableAllComponents[i]->getComponentType() == SComponentType::SCT_RUNTIME_MESH)
		{
			SRuntimeMeshComponent* pMesh = dynamic_cast<SRuntimeMeshComponent*>(vRenderableAllComponents[i]);
			bool bVisible = pMesh->isVisible() && pMesh->getContainer()->isVisible();

			if (bVisible == false || (pMesh->pParentComponent && pMesh->pParentComponent->bVisible == false))
			{
				continue;
			}

			if (pMesh->getCollisionPreset() == SCollisionPreset::SCP_NO_COLLISION)
			{
				continue;
			}

			pMeshPropsMtx = &pMesh->mtxComponentProps;
			pMeshPropsMtx->lock();
			pMeshData = pMesh->getMeshData();
		}

		// Check ingore list.
		for (size_t j = 0; j < vIgnoreList.size(); j++)
		{
			if (vRenderableAllComponents[i] == vIgnoreList[j])
			{
				pMeshPropsMtx->unlock();
				continue;
			}
		}

		SVector vRayDirection = vRayStopPos - vRayStartPos;
		float fRayLength = vRayDirection.length();
		vRayDirection.normalizeVector();

		DirectX::XMMATRIX mMeshWorld = DirectX::XMLoadFloat4x4(&vRenderableAllComponents[i]->renderData.vWorld);
		auto det = XMMatrixDeterminant(mMeshWorld);
		DirectX::XMMATRIX mInvMeshWorld = DirectX::XMMatrixInverse(&det, mMeshWorld);

		DirectX::XMVECTOR vRayOriginLocal
			= DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(vRayStartPos.getX(), vRayStartPos.getY(), vRayStartPos.getZ(), 1.0f), mInvMeshWorld);
		DirectX::XMVECTOR vRayDirectionLocal
			= DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(vRayDirection.getX(), vRayDirection.getY(), vRayDirection.getZ(), 0.0f), mInvMeshWorld);

		vRayDirectionLocal = DirectX::XMVector3Normalize(vRayDirectionLocal);

		float fHitDistance = 0.0f;
		bool bHit = false;
		if (vRenderableAllComponents[i]->collisionPreset == SCollisionPreset::SCP_SPHERE)
		{
			if (vRenderableAllComponents[i]->sphereCollision.Intersects(vRayOriginLocal, vRayDirectionLocal, fHitDistance))
			{
				bHit = true;
			}
		}
		else
		{
			if (vRenderableAllComponents[i]->boxCollision.Intersects(vRayOriginLocal, vRayDirectionLocal, fHitDistance))
			{
				bHit = true;
			}
		}

		if (bHit == false)
		{
			pMeshPropsMtx->unlock();
			continue;
		}

		if (fHitDistance > fRayLength)
		{
			pMeshPropsMtx->unlock();
			continue;
		}

		bool bHitTriangle = false;
		fHitDistance = FLT_MAX;
		SVector vHitNormal;

		// Iterate over all trianges.
		std::vector<uint32_t>* vIndices = pMeshData->getIndices32();
		size_t iTrisCount = vIndices->size() / 3;
		size_t vHitTriangle[3] = { 0 };
		for (size_t i = 0; i < iTrisCount; i++)
		{
			// Indices for this triangle.
			UINT i0 = vIndices->operator[](i * 3 + 0);
			UINT i1 = vIndices->operator[](i * 3 + 1);
			UINT i2 = vIndices->operator[](i * 3 + 2);

			// Vertices for this triangle.
			DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&pMeshData->getVertices()->operator[](i0).vPosition);
			DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&pMeshData->getVertices()->operator[](i1).vPosition);
			DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&pMeshData->getVertices()->operator[](i2).vPosition);

			float fTriangleHitDistance = FLT_MAX;
			if (DirectX::TriangleTests::Intersects(vRayOriginLocal, vRayDirectionLocal, v0, v1, v2, fTriangleHitDistance))
			{
				bHitTriangle = true;

				if (fTriangleHitDistance < fHitDistance)
				{
					fHitDistance = fTriangleHitDistance;

					vHitTriangle[0] = i0;
					vHitTriangle[1] = i1;
					vHitTriangle[2] = i2;

					vHitNormal.setX(
						(pMeshData->getVertices()->operator[](i0).getNormal().getX()
							+ pMeshData->getVertices()->operator[](i1).getNormal().getX()
							+ pMeshData->getVertices()->operator[](i2).getNormal().getX()) / 3.0f);
					vHitNormal.setY(
						(pMeshData->getVertices()->operator[](i0).getNormal().getY()
							+ pMeshData->getVertices()->operator[](i1).getNormal().getY()
							+ pMeshData->getVertices()->operator[](i2).getNormal().getY()) / 3.0f);
					vHitNormal.setZ(
						(pMeshData->getVertices()->operator[](i0).getNormal().getZ()
							+ pMeshData->getVertices()->operator[](i1).getNormal().getZ()
							+ pMeshData->getVertices()->operator[](i2).getNormal().getZ()) / 3.0f);
				}
			}
		}

		if (bHitTriangle)
		{
			SRayCastHit hitResult;
			hitResult.pHitComponent = vRenderableAllComponents[i];
			hitResult.fHitDistanceFromRayOrigin = fHitDistance;
			hitResult.vHitNormal = vHitNormal;

			hitResult.vHitTriangleIndices[0] = vHitTriangle[0];
			hitResult.vHitTriangleIndices[1] = vHitTriangle[1];
			hitResult.vHitTriangleIndices[2] = vHitTriangle[2];

			vHitResult.push_back(hitResult);
		}

		pMeshPropsMtx->unlock();
	}

	std::sort(vHitResult.begin(), vHitResult.end(), [](SRayCastHit a, SRayCastHit b) {return a.fHitDistanceFromRayOrigin < b.fHitDistanceFromRayOrigin; });
}

bool SLevel::spawnContainerInLevel(SContainer* pContainer)
{
	return pApp->spawnContainerInLevel(pContainer);
}

void SLevel::despawnContainerFromLevel(SContainer* pContainer)
{
	pApp->despawnContainerFromLevel(pContainer);
}

std::vector<SContainer*>* SLevel::getRenderableContainers()
{
	return &vRenderableContainers;
}

std::vector<SContainer*>* SLevel::getNotRenderableContainers()
{
	return &vNotRenderableContainers;
}