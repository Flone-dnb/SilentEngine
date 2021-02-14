// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <string>
#include <vector>
#include <mutex>
#include <functional>

// DirectX
#include <d3d12.h>
#include <wrl.h> // smart pointers

class SMeshDataComputeResource;

struct SComputeShaderResource
{
	Microsoft::WRL::ComPtr<ID3D12Resource> pResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> pUploadResource;

	SMeshDataComputeResource* pMeshComputeResource = nullptr;

	std::string sResourceName;

	unsigned int iShaderRegister;

	unsigned long long iDataSizeInBytes;

	bool bIsUAV;
};

struct SComputeShaderConstant
{
	std::string sConstantName;
	float _32BitConstant;
	unsigned int iShaderRegister;

	UINT iRootParamIndex;
};

//@@Class
/*
The class represents a compute shader.
*/
class SComputeShader
{
public:

	//@@Function
	/*
	* desc: use to query copy of the results of the compute shader for the next frame only.
	* param "sResourceNameToCopyFrom": the name of the resource (added through setAddData()), which will be copied after the compute shader finished work.
	* param "bBlockDraw": determines if we should block the frame drawing process right after we started executing our compute shader (will cause
	fps drop), or pass 'false' if you don't need this data right away (the data may be copied after 2-3 frames).
	* param "callback": function which will be called after the data was copied, the first param is a pointer to the copied data
	(you need to delete[] this data manually), the second param is the size of the copied data in bytes (should be equal to iDataSizeInBytes
	passed to setAddData()).
	An example of std::function pointing to a member
	function of a class - "using namespace std::placeholders; std::function<void(char_ptr, size_t)> f = std::bind(&Foo::foo, this, _1, _2);".
	* return: returns true if the resource with the specified name was not found, or startShaderExecution() was not called.
	If you call stopShaderExecution() after the copyComputeResults() call then even if the shader worked at least 1 time,
	the callback function will not be called.
	*/
	bool copyComputeResults(const std::string& sResourceNameToCopyFrom, bool bBlockDraw, std::function<void(char*, size_t)> callback);
	//@@Function
	/*
	* desc: starts to execute shader on every draw call until stopShaderExecution() is called.
	* param "iThreadGroupCountX": defines the amount of thread groups that will be dispatched by X-axis. Must be at least 1.
	* param "iThreadGroupCountY": defines the amount of thread groups that will be dispatched by Y-axis. Must be at least 1.
	* param "iThreadGroupCountZ": defines the amount of thread groups that will be dispatched by Z-axis. Must be at least 1.
	* return: returns true if the compute shader (hlsl) was not specified (see SComputeShader::compileShader()) or no shader data is added.
	*/
	bool startShaderExecution(unsigned int iThreadGroupCountX, unsigned int iThreadGroupCountY, unsigned int iThreadGroupCountZ);
	//@@Function
	/*
	* desc: the shader will no longer be executed until startShaderExecution() is called.
	*/
	void stopShaderExecution();
	//@@Function
	/*
	* desc: compiles the specified compute shader for later use.
	*/
	void compileShader(const std::wstring& sPathToShaderFile, const std::string& sShaderEntryFunctionName);
	//@@Function
	/*
	* desc: sets if shader should be executed before/after the frame is drawn.
	* remarks: by default executed before draw.
	*/
	void setSettingExecuteShaderBeforeDraw(bool bExecuteShaderBeforeDraw);
	//@@Function
	/*
	* desc: creates a resource for shader use, read-only data is passed as a 'StructuredBuffer' to 't' shader register (in hlsl),
	read-write data is passed as a 'RWStructuredBuffer' to 'u' shader register (in hlsl).
	* param "bReadOnlyData": true for read-only resource.
	* param "sResourceName": unique name of the resource.
	* param "iDataSizeInBytes": the size of buffer we need to allocate. For example, if you need to have 4 float values in the buffer,
	then pass: sizeof(float) x 4, or if you need to allocate space for your struct: sizeof(MyStruct) x N.
	* param "iShaderRegister": shader register (in hlsl), note that read-only and read-write data can use the same register
	as they are bound to the different shader registers (read-only to 't', read-write to 'u').
	* param "pInitData": initial data of the buffer (optional).
	* return: returns true if something went wrong or if the passed resource name is not unique, or if you reached max. amount of added resources
	(we have 64 free slots, each resource takes 2 slots, each 32 bit constant 1 slot).
	* remarks: calling this function after startShaderExecution() call, will always return true. 
	*/
	bool setAddData(bool bReadOnlyData, const std::string& sResourceName, unsigned long long iDataSizeInBytes, unsigned int iShaderRegister, const void* pInitData = nullptr);
	//@@Function
	/*
	* desc: adds a mesh resource for shader use, it will be a 'RWStructuredBuffer' to 'u' shader register (in hlsl).
	* param "pResource": resource to vertex/index buffer of the mesh (use getMeshDataAsComputeResource() in component).
	* param "sResourceName": unique name of the resource.
	* param "iShaderRegister": shader register (in hlsl).
	* param "iOutDataSizeInBytes": the size of the passed resource that you can use,
	for vertex buffer will be 'meshData.getVerticesCount() x sizeof(SVertex)'
	see SPrimitiveShapeGenerator.h to see SVertex struct, for index buffer size will be 'meshData.getIndicesCount() x sizeof(std::uint32_t)' (if
	meshData.hasIndicesMoreThan16Bits() == true) or
	'meshData.getIndicesCount() x sizeof(std::uint16_t)' (if meshData.hasIndicesMoreThan16Bits() == false).
	* return: returns true if something went wrong or if the passed resource name is not unique, or if you reached max. amount of added resources
	(we have 64 free slots, each resource takes 2 slots, each 32 bit constant 1 slot).
	* remarks: if using vertex buffer, don't forget to create SVertex struct in hlsl.
	Calling this function after startShaderExecution() call, will always return true.
	*/
	bool setAddMeshResource(SMeshDataComputeResource* pResource, const std::string& sResourceName, unsigned int iShaderRegister, unsigned long long& iOutDataSizeInBytes);

	//@@Function
	/*
	* desc: adds a 32 bit constant value for shader use, it will be a 'cbuffer' to 'b' shader register (in hlsl).
	* return: returns true if something went wrong or if the passed constant name is not unique, or if you reached max. amount of added resources
	(we have 64 free slots, each resource takes 2 slots, each 32 bit constant 1 slot).
	* remarks: you can bind multiple constants to the same 'iShaderRegister', they will be copied to one cbuffer
	in the order in which you added them, it's better to add all values in one cbuffer.
	Calling this function after startShaderExecution() call, will always return true.
	*/
	bool setAdd32BitConstant(float _32BitConstant, const std::string& sConstantName, unsigned int iShaderRegister);

	//@@Function
	/*
	* desc: updates added 32 bit constant value to a new value.
	* return: returns true if the constant with this name was not found.
	* remarks: can be called after startShaderExecution(), it's recommended to update constants in onTick() function.
	*/
	bool setUpdate32BitConstant(float _32BitConstant, const std::string& sContantName);

private:

	friend class SApplication;
	friend class SMeshComponent;

	// Only SApplication can create instances of SComputeShader.
	SComputeShader() = delete;
	SComputeShader(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, bool bCompileShaderInReleaseMode,
		std::string sComputeShaderName = "Compute Shader");
	SComputeShader(const SComputeShader&) = delete;
	SComputeShader& operator= (const SComputeShader&) = delete;
	~SComputeShader();


	void finishedCopyingComputeResults(char* pData, size_t iSizeInBytes);
	// called when setMeshData gets called.
	void updateMeshResource(const std::string& sResourceName);


	void createRootSignatureAndPSO();

	
	std::mutex mtxComputeSettings;

	ID3D12Device* pDevice;
	ID3D12GraphicsCommandList* pCommandList;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> pComputeRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pComputePSO;
	Microsoft::WRL::ComPtr<ID3DBlob> pCompiledShader;


	std::vector<SComputeShaderResource*> vShaderResources;
	std::vector<UINT64> vFinishFences;
	std::mutex mtxFencesVector;

	std::function<void(char*, size_t)> callbackWhenResultsCopied;

	std::string sComputeShaderName;
	std::string sResourceNameToCopyFrom;

	std::vector<SComputeShaderConstant> v32BitConstants;
	std::vector<UINT> vUsedRootIndex;

	unsigned int iThreadGroupCountX, iThreadGroupCountY, iThreadGroupCountZ;

	bool bExecuteShaderBeforeDraw;
	bool bExecuteShader;
	bool bCompileShaderInReleaseMode;
	bool bCompiledShader;
	bool bWaitForComputeShaderRightAfterDraw;
	bool bWaitForComputeShaderToFinish;
	bool bCopyingComputeResult;
};

