// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "MyContainer.h"

#include "SilentEngine/Public/EntityComponentSystem/SMeshComponent/SMeshComponent.h"
#include "SilentEditor/EditorApplication/EditorApplication.h"

MyContainer::MyContainer(const std::string& sContainerName) : SContainer(sContainerName)
{
	// Root component is container itself.

	SMeshComponent* pSMeshComponent = new SMeshComponent("SMeshComponent");
	addComponentToContainer(pSMeshComponent);

	pSMeshComponent->setMeshData(SPrimitiveShapeGenerator::createBox(1.0f, 1.0f, 1.0f), true);



	bool bError;
	SMaterial* pMat = EditorApplication::getApp()->registerMaterial("my mat", bError);

	SMaterialProperties props;
	props.setDiffuseColor(SVector(0.0f, 1.0f, 0.0f));

	pMat->setMaterialProperties(props);



	SMeshComponent* pSMeshComponentGreen = new SMeshComponent("pSMeshComponentGreen");
	addComponentToContainer(pSMeshComponentGreen);

	pSMeshComponentGreen->setMeshData(SPrimitiveShapeGenerator::createBox(1.0f, 1.0f, 1.0f), true);
	pSMeshComponentGreen->setLocalLocation(SVector(-3.0f, 0.0f, 0.0f));


	pSMeshComponentGreen->setMeshMaterial(pMat);
}

MyContainer::~MyContainer()
{
	// If the component was added to the container (addComponentToContainer()) then
	// here DO NOT:
	// - delete pComponent;
	// - pComponent->removeFromContainer();
	// the added component will be deleted in the SContainer's (parent) destructor.
}