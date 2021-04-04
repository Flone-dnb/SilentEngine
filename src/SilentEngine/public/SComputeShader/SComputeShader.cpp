// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SComputeShader.h"

// Custom
#include "SilentEngine/Private/SError/SError.h"
#pragma warning(push, 0) // disable warnings from this header
#include "SilentEngine/Private/d3dx12.h"
#pragma warning(pop)
#include "SilentEngine/Public/SApplication/SApplication.h"
#include "SilentEngine/Private/EntityComponentSystem/SComponent/SComponent.h"
#include "SilentEngine/Public/EntityComponentSystem/SMeshComponent/SMeshComponent.h"
#include "SilentEngine/Public/EntityComponentSystem/SRuntimeMeshComponent/SRuntimeMeshComponent.h"
#include "SilentEngine/Private/SMiscHelpers/SMiscHelpers.h"
#include "SilentEngine/Public/SPrimitiveShapeGenerator/SPrimitiveShapeGenerator.h" // for SMeshDataComputeResource

bool SComputeShader::copyComputeResults(const std::vector<std::string>& vResourceNamesToCopyFrom, bool bBlockDraw, std::function<void(std::vector<char*>, std::vector<size_t>)> callback)
{
	if (bExecuteShader == false)
	{
		return true;
	}


	std::vector<bool> vFound(vResourceNamesToCopyFrom.size());
	for (size_t i = 0; i < vResourceNamesToCopyFrom.size(); i++)
	{
		for (size_t j = 0; j < vShaderResources.size(); j++)
		{
			if (vShaderResources[j]->sResourceName == vResourceNamesToCopyFrom[i])
			{
				vFound[i] = true;

				break;
			}
		}
	}

	bool bFoundAll = true;

	for (size_t i = 0; i < vFound.size(); i++)
	{
		if (vFound[i] == false)
		{
			bFoundAll = false;
			break;
		}
	}

	if (bFoundAll == false)
	{
		SError::showErrorMessageBox(L"SComputeShader::copyComputeResults()", L"Not all resource names were found.");
		return true;
	}


	bWaitForComputeShaderRightAfterDraw = bBlockDraw;
	bWaitForComputeShaderToFinish = true;

	this->vResourceNamesToCopyFrom = vResourceNamesToCopyFrom;

	callbackWhenResultsCopied = callback;

	return false;
}

bool SComputeShader::startShaderExecution(unsigned int iThreadGroupCountX, unsigned int iThreadGroupCountY, unsigned int iThreadGroupCountZ)
{
	if (bCompiledShader == false || vShaderResources.size() == 0)
	{
		return true;
	}

	if (iThreadGroupCountX < 1 || iThreadGroupCountY < 1 || iThreadGroupCountZ < 1)
	{
		SError::showErrorMessageBox(L"SComputeShader::startShaderExecution()", L"iThreadGroupCount must be at least 1.");
		return true;
	}

	this->iThreadGroupCountX = iThreadGroupCountX;
	this->iThreadGroupCountY = iThreadGroupCountY;
	this->iThreadGroupCountZ = iThreadGroupCountZ;

	if (pComputeRootSignature == nullptr)
	{
		// First time started compute shader.
		createRootSignatureAndPSO();
	}

	bExecuteShader = true;

	return false;
}

void SComputeShader::stopShaderExecution()
{
	bExecuteShader = false;

	bWaitForComputeShaderRightAfterDraw = false;
	bWaitForComputeShaderToFinish = false;

	mtxFencesVector.lock();
	vFinishFences.clear();
	mtxFencesVector.unlock();
}

void SComputeShader::compileShader(const std::wstring& sPathToShaderFile, const std::wstring& sShaderEntryFunctionName)
{
	pCompiledShader = SMiscHelpers::compileShader(sPathToShaderFile, nullptr, sShaderEntryFunctionName, SE_CS_SM, bCompileShaderInReleaseMode);

	bCompiledShader = true;
}

void SComputeShader::setSettingExecuteShaderBeforeDraw(bool bExecuteShaderBeforeDraw)
{
	this->bExecuteShaderBeforeDraw = bExecuteShaderBeforeDraw;
}

bool SComputeShader::setAddData(bool bReadOnlyData, const std::string& sResourceName, unsigned long long iDataSizeInBytes, unsigned int iShaderRegister, const void* pInitData)
{
	if (bExecuteShader)
	{
		return true;
	}

	if ((vShaderResources.size() * 2) + v32BitConstants.size() + 2 > 64)
	{
		// We're using root descriptors, each descriptor takes 2 DWORDs, each constant 1 DWORD out of 64 DWORD in root signature.
		return true;
	}

	for (size_t i = 0; i < vShaderResources.size(); i++)
	{
		if (vShaderResources[i]->sResourceName == sResourceName)
		{
			return true;
		}
	}

	SComputeShaderResource* pNewResource = new SComputeShaderResource();
	pNewResource->bIsUAV = !bReadOnlyData;
	pNewResource->iShaderRegister = iShaderRegister;
	pNewResource->sResourceName = sResourceName;
	pNewResource->iDataSizeInBytes = iDataSizeInBytes;


	HRESULT hresult;

	if (pInitData)
	{
		SApplication* pApp = SApplication::getApp();

		pApp->mtxDraw.lock();
		pApp->flushCommandQueue();
		pApp->resetCommandList();

		pNewResource->pResource =
			SMiscHelpers::createBufferWithData(pDevice, pCommandList, pInitData, iDataSizeInBytes, pNewResource->pUploadResource, !bReadOnlyData);

		pApp->executeCommandList();
		pApp->flushCommandQueue();
		pApp->mtxDraw.unlock();
	}
	else
	{
		if (bReadOnlyData == false)
		{
			CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
			CD3DX12_RESOURCE_DESC buf = CD3DX12_RESOURCE_DESC::Buffer(iDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			hresult = pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&buf,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				IID_PPV_ARGS(pNewResource->pResource.GetAddressOf()));
			if (FAILED(hresult))
			{
				SError::showErrorMessageBox(hresult, L"SComputeShader::setAddData::ID3D12Device::CreateCommittedResource() (read-write without init data)");
				delete pNewResource;
				return true;
			}
		}
		else
		{
			SError::showErrorMessageBox(L"SComputeShader::setAddData()", L"cannot create read only resource without init data.");
			delete pNewResource;
			return true;
		}
	}

	vShaderResources.push_back(pNewResource);

	return false;
}

bool SComputeShader::setAddMeshResource(SMeshDataComputeResource* pResource, const std::string& sResourceName, unsigned int iShaderRegister, unsigned long long& iOutDataSizeInBytes)
{
	if (bExecuteShader)
	{
		return true;
	}

	if ((vShaderResources.size() * 2) + v32BitConstants.size() + 2 > 64)
	{
		// We're using root descriptors, each descriptor takes 2 DWORDs, each constant 1 DWORD out of 64 DWORD in root signature.
		return true;
	}

	for (size_t i = 0; i < vShaderResources.size(); i++)
	{
		if (vShaderResources[i]->sResourceName == sResourceName)
		{
			return true;
		}
	}

	SComputeShaderResource* pNewResource = new SComputeShaderResource();
	pNewResource->bIsUAV = true;
	pNewResource->iShaderRegister = iShaderRegister;
	pNewResource->sResourceName = sResourceName;
	if (pResource->pResourceOwner->componentType == SComponentType::SCT_MESH)
	{
		pNewResource->pResource = dynamic_cast<SMeshComponent*>(pResource->pResourceOwner)->getResource(pResource->bVertexBuffer);
	}
	else
	{
		SError::showErrorMessageBox(L"SComputeShader::setAddMeshResource()", L"SComponent type is not SCT_MESH!");
	}
	pNewResource->pMeshComputeResource = pResource;

	pResource->pResourceOwner->bindResourceUpdates(this, sResourceName);

	vShaderResources.push_back(pNewResource);

	return false;
}

bool SComputeShader::setAdd32BitConstant(float _32BitConstant, const std::string& sConstantName, unsigned int iShaderRegister)
{
	if (bExecuteShader)
	{
		return true;
	}

	if ((vShaderResources.size() * 2) + v32BitConstants.size() + 1 > 64)
	{
		// We're using root descriptors, each descriptor takes 2 DWORDs, each constant 1 DWORD out of 64 DWORD in root signature.
		return true;
	}

	for (size_t i = 0; i < v32BitConstants.size(); i++)
	{
		if (v32BitConstants[i].sConstantName == sConstantName)
		{
			return true;
		}
	}

	SComputeShaderConstant constant;
	constant.iShaderRegister = iShaderRegister;
	constant.sConstantName = sConstantName;
	constant._32BitConstant = _32BitConstant;

	v32BitConstants.push_back(constant);

	return false;
}

bool SComputeShader::setUpdate32BitConstant(float _32BitConstant, const std::string& sContantName)
{
	bool bFound = false;

	for (size_t i = 0; i < v32BitConstants.size(); i++)
	{
		if (v32BitConstants[i].sConstantName == sContantName)
		{
			v32BitConstants[i]._32BitConstant = _32BitConstant;

			bFound = true;

			break;
		}
	}

	return !bFound;
}

SComputeShader::SComputeShader(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool bCompileShaderInReleaseMode, std::string sComputeShaderName)
{
	this->pDevice = pDevice;
	this->pCommandList = pCommandList;
	this->bCompileShaderInReleaseMode = bCompileShaderInReleaseMode;

	pComputeRootSignature = nullptr;

	this->sComputeShaderName = sComputeShaderName;

	bExecuteShaderBeforeDraw = true;
	bWaitForComputeShaderToFinish = false;
	bCopyingComputeResult = false;

	iThreadGroupCountX = 1;
	iThreadGroupCountY = 1;
	iThreadGroupCountZ = 1;

	bExecuteShader = false;
}

SComputeShader::~SComputeShader()
{
	for (size_t i = 0; i < vShaderResources.size(); i++)
	{
		ULONG iLeftRef = vShaderResources[i]->pResource.Reset();

		if ((iLeftRef != 0) && (vShaderResources[i]->pMeshComputeResource == nullptr))
		{
			// This resource is added using setAddData().
			SError::showErrorMessageBox(L"SComputeShader::~SComputeShader()", L"shader resource left ref. count is not 0.");
		}

		iLeftRef = vShaderResources[i]->pUploadResource.Reset();

		if (iLeftRef != 0)
		{
			SError::showErrorMessageBox(L"SComputeShader::~SComputeShader()", L"shader upload resource left ref. count is not 0.");
		}

		if (vShaderResources[i]->pMeshComputeResource)
		{
			if (SApplication::getApp()->doesComponentExists(vShaderResources[i]->pMeshComputeResource->pResourceOwner))
			{
				vShaderResources[i]->pMeshComputeResource->pResourceOwner->unbindResourceUpdates(this);
				// vertex buffer was modified
				//dynamic_cast<SMeshComponent*>(vShaderResources[i]->pMeshComputeResource->pResourceOwner)->bVertexBufferUsedInComputeShader = false;
			}
			
			delete vShaderResources[i]->pMeshComputeResource;
		}

		delete vShaderResources[i];
	}

	ULONG iLeftRef = pComputeRootSignature.Reset();
	if (iLeftRef != 0)
	{
		SError::showErrorMessageBox(L"SComputeShader::~SComputeShader()", L"compute root signature left ref. count is not 0.");
	}

	iLeftRef = pComputePSO.Reset();
	if (iLeftRef != 0)
	{
		SError::showErrorMessageBox(L"SComputeShader::~SComputeShader()", L"compute pso left ref. count is not 0.");
	}

	pCompiledShader.Release();
}

void SComputeShader::finishedCopyingComputeResults(const std::vector<char*>& vData, const std::vector<size_t>& vDataSizes)
{
	bWaitForComputeShaderRightAfterDraw = false;
	bWaitForComputeShaderToFinish = false;

	bCopyingComputeResult = true;
	callbackWhenResultsCopied(vData, vDataSizes);
	bCopyingComputeResult = false;
}

void SComputeShader::updateMeshResource(const std::string& sResourceName)
{
	for (size_t i = 0; i < vShaderResources.size(); i++)
	{
		if (vShaderResources[i]->sResourceName == sResourceName)
		{
			if (vShaderResources[i]->pMeshComputeResource->pResourceOwner->componentType == SComponentType::SCT_MESH)
			{
				vShaderResources[i]->pResource = dynamic_cast<SMeshComponent*>(vShaderResources[i]->pMeshComputeResource->pResourceOwner)
					->getResource(vShaderResources[i]->pMeshComputeResource->bVertexBuffer);
			}
			else
			{
				SError::showErrorMessageBox(L"SComputeShader::updateMeshResource()", L"SComponent type is not SCT_MESH!");
			}

			break;
		}
	}
}

void SComputeShader::createRootSignatureAndPSO()
{
	std::lock_guard<std::mutex> lock(mtxComputeSettings);


	std::vector<UINT> vUsedCBRegisters;
	std::vector<UINT> vConstantsByRegisters;

	for (size_t i = 0; i < v32BitConstants.size(); i++)
	{
		bool bFound = false;

		for (size_t j = 0; j < vUsedCBRegisters.size(); j++)
		{
			if (vUsedCBRegisters[j] == v32BitConstants[i].iShaderRegister)
			{
				bFound = true;

				vConstantsByRegisters[j] += 1;

				break;
			}
		}

		if (bFound == false)
		{
			vUsedCBRegisters.push_back(v32BitConstants[i].iShaderRegister);
			vConstantsByRegisters.push_back(1);
		}
	}


	// Create root signature.

	CD3DX12_ROOT_PARAMETER* pSlotRootParameter = new CD3DX12_ROOT_PARAMETER[vShaderResources.size() + vUsedCBRegisters.size()];

	for (size_t i = 0; i < vShaderResources.size(); i++)
	{
		if (vShaderResources[i]->bIsUAV)
		{
			pSlotRootParameter[i].InitAsUnorderedAccessView(vShaderResources[i]->iShaderRegister);
		}
		else
		{
			pSlotRootParameter[i].InitAsShaderResourceView(vShaderResources[i]->iShaderRegister);
		}
	}

	UINT iStartIndex = static_cast<UINT>(vShaderResources.size());

	for (size_t i = 0; i < vUsedCBRegisters.size(); i++)
	{
		pSlotRootParameter[iStartIndex].InitAsConstants(vConstantsByRegisters[i], vUsedCBRegisters[i]);

		for (size_t k = 0; k < v32BitConstants.size(); k++)
		{
			if (v32BitConstants[k].iShaderRegister == vUsedCBRegisters[i])
			{
				v32BitConstants[k].iRootParamIndex = iStartIndex;
			}
		}

		vUsedRootIndex.push_back(iStartIndex);

		iStartIndex++;
	}


	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(static_cast<UINT>(vShaderResources.size() + vUsedCBRegisters.size()), pSlotRootParameter,
		0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hresult = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SComputeShader::createRootSignatureAndPSO::ID3D12Device::D3D12SerializeRootSignature()");
		delete[] pSlotRootParameter;
		return;
	}

	hresult = pDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&pComputeRootSignature)
	);
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SComputeShader::createRootSignatureAndPSO::ID3D12Device::CreateRootSignature()");
		delete[] pSlotRootParameter;
		return;
	}

	delete[] pSlotRootParameter;


	// Create PSO.

	D3D12_COMPUTE_PIPELINE_STATE_DESC horzBlurPSO = {};
	horzBlurPSO.pRootSignature = pComputeRootSignature.Get();
	horzBlurPSO.CS =
	{
		reinterpret_cast<BYTE*>(pCompiledShader->GetBufferPointer()),
		pCompiledShader->GetBufferSize()
	};
	horzBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	hresult = pDevice->CreateComputePipelineState(&horzBlurPSO, IID_PPV_ARGS(&pComputePSO));
	if (FAILED(hresult))
	{
		SError::showErrorMessageBox(hresult, L"SComputeShader::createRootSignatureAndPSO::ID3D12Device::CreateGraphicsPipelineState()");
		return;
	}
}
