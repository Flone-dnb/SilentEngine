// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SFormatOBJImporter.h"

// STL
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// Custom
#include "SilentEngine/Private/SError/SError.h"
#include "SilentEngine/Public/SVector/SVector.h"

bool SFormatOBJImporter::importMeshDataFromFile(const std::wstring& sPathToFile, SMeshData* pMeshData)
{
	// See if the file exists.

	std::ifstream objFile(sPathToFile);

	if (objFile.is_open() == false)
	{
		SError::showErrorMessageBox(L"SFormatOBJImporter::importMeshDataFromFile", L"The specified file cannot be opened, does it exist?");
		return true;
	}
	else
	{
		objFile.close();
	}


	// (use ISO C++17 Standard for no errors on MSVC 2019 toolset)
	if (fs::path(sPathToFile).extension().string() != ".obj")
	{
		// Not an .obj file.

		SError::showErrorMessageBox(L"SFormatOBJImporter::importMeshDataFromFile", L"File format is not '.obj'.");
		return true;
	}



	std::vector<SVector> vVertices;
	std::vector<SVector> vUVs;
	std::vector<SVector> vNormals;

	pMeshData->clearVertices();
	pMeshData->clearIndices();


	// Read file.

	objFile.open(sPathToFile);

	std::string sReadLine = "";

	while (std::getline(objFile, sReadLine))
	{
		if (sReadLine[0] == 'v')
		{
			if (sReadLine[1] == ' ')
			{
				vVertices.push_back(readValues(sReadLine, 2, 3));
			}
			else if (sReadLine[1] == 't')
			{
				vUVs.push_back(readValues(sReadLine, 3, 2));
			}
			else if (sReadLine[1] == 'n')
			{
				vNormals.push_back(readValues(sReadLine, 3, 3));
				vNormals.back().normalizeVector();
			}
		}
		else if (sReadLine[0] == 'f')
		{
			size_t iVertexCount = 0;

			for (size_t i = 2; i < sReadLine.size();)
			{
				int iVertexIndex = -1;
				int iUVIndex = -1;
				int iNormalIndex = -1;

				readVertex(sReadLine, i, iVertexIndex, iUVIndex, iNormalIndex);

				SVector vNormal = SVector(0.0f, 0.0f, 0.0f);
				SVector vUV = SVector(0.0f, 0.0f);

				if (iUVIndex != -1)
				{
					vUV = vUVs[iUVIndex];
				}

				if (iNormalIndex != -1)
				{
					vNormal = vNormals[iNormalIndex];
				}

				SMeshVertex vertex(vVertices[iVertexIndex], vNormal, SVector(), vUV);

				pMeshData->addVertex(vertex);
				pMeshData->addIndex(pMeshData->getVerticesCount() - 1);

				iVertexCount++;

				if ((iVertexCount == 3) && (i < sReadLine.size()))
				{
					objFile.close();

					SError::showErrorMessageBox(L"SFormatOBJImporter::importMeshDataFromFile", L"Object faces are not triangulated!");

					return true;
				}
			}
		}
		else
		{
			continue;
		}
	}

	objFile.close();

	return false;
}

SVector SFormatOBJImporter::readValues(const std::string& sLine, size_t iReadStartIndex, size_t iValueCount)
{
	size_t iReadValues = 0;
	SVector vVec;

	std::string sValue = "";

	for (size_t i = iReadStartIndex; i < sLine.size(); i++)
	{
		if (sLine[i] != ' ')
		{
			sValue += sLine[i];
		}
		else
		{
			if (sValue != "")
			{
				if (iReadValues == 0)
				{
					vVec.setX(std::stod(sValue));
				}
				else if (iReadValues == 1)
				{
					vVec.setY(std::stod(sValue));
				}
				else
				{
					vVec.setZ(std::stod(sValue));
				}

				iReadValues++;

				if (iReadValues == iValueCount)
				{
					break;
				}

				sValue = "";
			}
		}
	}

	if (sValue != "")
	{
		if (iReadValues == 0)
		{
			vVec.setX(std::stod(sValue));
		}
		else if (iReadValues == 1)
		{
			vVec.setY(std::stod(sValue));
		}
		else
		{
			vVec.setZ(std::stod(sValue));
		}
	}

	return vVec;
}

void SFormatOBJImporter::readVertex(const std::string& sLine, size_t& iReadIndex, int& iVertexIndex, int& iUVIndex, int& iNormalIndex)
{
	iVertexIndex = std::stoi(readVertexValue(sLine, iReadIndex));
	iVertexIndex--; // start from 0.
	
	if (iReadIndex + 1 < sLine.size())
	{
		if (sLine[iReadIndex] == ' ')
		{
			return;
		}
	}
	else
	{
		return;
	}

	if (sLine[iReadIndex + 1] != '/')
	{
		// Read UV.

		iReadIndex++;

		iUVIndex = std::stoi(readVertexValue(sLine, iReadIndex));
		iUVIndex--; // start from 0.

		iReadIndex++;
	}
	else
	{
		iReadIndex += 2;
	}

	// Read normal.
	
	iNormalIndex = std::stoi(readVertexValue(sLine, iReadIndex));
	iNormalIndex--; // start from 0.

	iReadIndex++;
}

std::string SFormatOBJImporter::readVertexValue(const std::string& sLine, size_t& iReadIndex)
{
	std::string sStringIndex = "";

	for (; iReadIndex < sLine.size(); iReadIndex++)
	{
		if (sLine[iReadIndex] != '/' && sLine[iReadIndex] != ' ')
		{
			sStringIndex += sLine[iReadIndex];
		}
		else
		{
			break;
		}
	}

	return sStringIndex;
}
