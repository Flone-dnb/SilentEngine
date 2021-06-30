// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <string>

// Custom
#include "SilentEngine/Public/SPrimitiveShapeGenerator/SPrimitiveShapeGenerator.h"

//@@Class
/*
The class is used to import mesh data from files with the .obj format.
*/
class SFormatOBJImporter
{
public:

	//@@Function
	/*
	* desc: used to read mesh data from .obj file into the 'pMeshData' pointer.
	* param "pMeshData": a pointer to your SMeshData instance that will be filled.
	* return: false if successful, true otherwise.
	*/
	static bool importMeshDataFromFile(const std::wstring& sPathToFile, SMeshData* pMeshData, bool bFlipUVByY = true);

private:

	SFormatOBJImporter() = default;

	static SVector readValues(const std::string& sLine, size_t iReadStartIndex, size_t iValueCount);
	static void    readVertex(const std::string& sLine, size_t& iReadIndex, int& iVertexIndex, int& iUVIndex, int& iNormalIndex);
	static std::string  readVertexValue(const std::string& sLine, size_t& iReadIndex);
};

