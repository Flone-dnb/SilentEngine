// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "MyContainer.h"

#include "SilentEngine/public/EntityComponentSystem/SMeshComponent/SMeshComponent.h"
#include "SilentEngine/public/EntityComponentSystem/SDirectionalLightComponent/SDirectionalLightComponent.h"
#include "SilentEngine/public/EntityComponentSystem/SPointLightComponent/SPointLightComponent.h"
#include "SilentEngine/public/SApplication/SApplication.h"
#include "SilentEngine/public/SLevel/SLevel.h"
#include "SilentEngine/public/FileImport/SFormatOBJImporter/SFormatOBJImporter.h"

MyContainer::MyContainer(const std::string& sContainerName) : SContainer(sContainerName)
{
	// Root component is container itself.

	// Create floor mesh.
	pFloorMeshComponent = new SMeshComponent("Floor");
	SMeshData meshData;
	if (SFormatOBJImporter::importMeshDataFromFile(L"sample_data/floor.obj", &meshData))
	{
		SError::showErrorMessageBoxAndLog("Failed to import mesh.");
		return;
	}
	pFloorMeshComponent->setMeshData(meshData, true);
	addComponentToContainer(pFloorMeshComponent);
	pFloorMeshComponent->setLocalLocation(SVector(0.0f, 0.0f, -5.0f));

	// Register floor material.
	bool bError = false;
	SMaterial* pFloorMaterial = SApplication::getApp()->registerMaterial("Floor Material", bError);
	if (bError)
	{
		SError::showErrorMessageBoxAndLog("Failed to register floor mesh material.");
		return;
	}
	SMaterialProperties floorMaterialProperties;
	floorMaterialProperties.setDiffuseColor(SVector(0.1f, 0.05f, 0.0f));
	pFloorMaterial->setMaterialProperties(floorMaterialProperties);
	//pFloorMeshComponent->setMeshMaterial(pFloorMaterial);

	// Create suzanne mesh.
	pSuzanneMeshComponent = new SMeshComponent("Suzanne");
	if (SFormatOBJImporter::importMeshDataFromFile(L"sample_data/suzanne.obj", &meshData))
	{
		SError::showErrorMessageBoxAndLog("Failed to import mesh.");
		return;
	}
	pSuzanneMeshComponent->setMeshData(meshData, true);
	addComponentToContainer(pSuzanneMeshComponent);

	// No more meshes (will recalculate level bounds).

	// Create directional light.
	pDirectionalLightComponent = new SDirectionalLightComponent("Directional Light",
		SApplication::getApp()->getCurrentLevel()->getLevelBounds(true));
	addComponentToContainer(pDirectionalLightComponent);
	pDirectionalLightComponent->setLightDirection(SVector(-1.0f, 0.0f, -1.0f));
	pDirectionalLightComponent->setLightColor(SVector(0.8f, 0.8f, 1.0f));

	// Create point light.
	pPointLightComponent = new SPointLightComponent("Point Light");
	addComponentToContainer(pPointLightComponent);
	pPointLightComponent->setLocalLocation(SVector(10.0f, 0.0f, 10.0f));
	pPointLightComponent->setLightColor(SVector(1.0f, 0.5f, 0.0f));

	// Add a skybox.
	//pSkyboxMeshComponent = new SMeshComponent("Skybox");
	//if (SFormatOBJImporter::importMeshDataFromFile(L"sample_data/skyboxMesh.obj", &meshData))
	//{
	//	SError::showErrorMessageBoxAndLog("Failed to import mesh.");
	//	return;
	//}
	//pSkyboxMeshComponent->setMeshData(meshData, true);
	//addComponentToContainer(pSkyboxMeshComponent);

	//// Load skybox texture.
	//auto skyboxTextureHandle = SApplication::getApp()->loadTextureFromDiskToGPU(
	//	"Skybox", L"sample_data/skybox.dds", true, bError);
	//if (bError)
	//{
	//	SError::showErrorMessageBoxAndLog("Failed to import skybox texture.");
	//	return;
	//}

	//// Register skybox shader.
	//SCustomShaderProperties shaderProps;
	//shaderProps.skyboxTexture = skyboxTextureHandle;
	//auto pSkyboxShader = SApplication::getApp()->compileCustomShader(L"shaders/skybox.hlsl", shaderProps);

	//// Assign skybox shader.
	//pSkyboxMeshComponent->setUseCustomShader(pSkyboxShader);
	//pSkyboxMeshComponent->setLocalRotation(SVector(90.0f, 0.0f, 0.0f)); // rotate for skybox to be correct

	vInitialPointLightLocation = pPointLightComponent->getLocalLocation();
}

void MyContainer::onTick(float fDeltaTime)
{
	fTotalTime += fDeltaTime;

	// Rotate point light.
	auto vNewLocation = vInitialPointLightLocation;
	vNewLocation.rotateAroundAxis(SVector(0.0f, 0.0f, 1.0f), 360.0f * sinf(fTotalTime * fPointLightRotationSpeed));
	pPointLightComponent->setLocalLocation(vNewLocation);
}

MyContainer::~MyContainer()
{
	// If the component was added to the container (addComponentToContainer()) then
	// here DO NOT:
	// - delete pComponent;
	// - pComponent->removeFromContainer();
	// the added component will be deleted in the SContainer's (parent) destructor.
}