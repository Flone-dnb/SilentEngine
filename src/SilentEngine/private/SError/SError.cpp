// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#include "SError.h"

// STL
#include <fstream>
#include <filesystem>
#include <string>
#include <ranges>

namespace fs = std::filesystem;

// Custom
#include "SilentEngine/Public/SApplication/SApplication.h"

// Other
#include <windows.h>
#include <winnt.h>

constexpr auto ERROR_FILE_NAME = "last_error.log";

void SError::showErrorMessageBoxAndLog(HRESULT hresult, const std::source_location& location)
{
	LPSTR errorText = NULL;

	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		hresult,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPSTR)&errorText,
		0,
		NULL);

	if (errorText != NULL)
	{
		std::string sErrorString = "An error occurred at file ";
		
		// Get last '\\' char location.
		std::string_view file_name(location.file_name());
		size_t iLastSlashPos = 0;
		for (size_t i = 0; i < file_name.size(); i++)
		{
			if (file_name[i] == '\\')
			{
				iLastSlashPos = i;
			}
		}

		sErrorString += file_name.substr(iLastSlashPos + 1);
		sErrorString += ", function: ";
		sErrorString += location.function_name();
		sErrorString += ", [";
		sErrorString += std::to_string(location.line());
		sErrorString += ",";
		sErrorString += std::to_string(location.column());
		sErrorString += "]. Error description: ";
		sErrorString += errorText;

		std::ofstream errorFile(ERROR_FILE_NAME);
		if (errorFile.is_open())
		{
			errorFile << sErrorString;

			errorFile.close();
		}

		sErrorString += "\n\n";
		sErrorString += "This error message was saved at ";
		sErrorString += fs::current_path().string() + "\\" + ERROR_FILE_NAME;

		MessageBoxA(0, sErrorString.c_str(), "Error", 0);

		LocalFree(errorText);
	}
	else
	{
		std::string sErrorString = "An unknown error occurred at file ";

		// Get last '\\' char location.
		std::string_view file_name(location.file_name());
		size_t iLastSlashPos = 0;
		for (size_t i = 0; i < file_name.size(); i++)
		{
			if (file_name[i] == '\\')
			{
				iLastSlashPos = i;
			}
		}

		sErrorString += file_name.substr(iLastSlashPos + 1);
		sErrorString += ", function: ";
		sErrorString += location.function_name();
		sErrorString += ", [";
		sErrorString += std::to_string(location.line());
		sErrorString += ",";
		sErrorString += std::to_string(location.column());
		sErrorString += "].";

		std::ofstream errorFile(ERROR_FILE_NAME);
		if (errorFile.is_open())
		{
			errorFile << sErrorString;

			errorFile.close();
		}

		sErrorString += "\n\n";
		sErrorString += "This error message was saved at ";
		sErrorString += fs::current_path().string() + "\\" + ERROR_FILE_NAME;

		MessageBoxA(0, sErrorString.c_str(), "Error", 0);
	}

	if (hresult == 0x887a0005)
	{
		// ?
		SApplication::getApp()->showDeviceRemovedReason();
	}

	HRESULT error = hresult;
	ThrowIfFailed(error);
}

void SError::showErrorMessageBoxAndLog(std::string sErrorString, const std::source_location& location)
{
	std::string sErrorStr = "An error occured at file ";

	// Get last '\\' char location.
	std::string_view file_name(location.file_name());
	size_t iLastSlashPos = 0;
	for (size_t i = 0; i < file_name.size(); i++)
	{
		if (file_name[i] == '\\')
		{
			iLastSlashPos = i;
		}
	}

	sErrorStr += file_name.substr(iLastSlashPos + 1);
	sErrorStr += ", function: ";
	sErrorStr += location.function_name();
	sErrorStr += ", [";
	sErrorStr += std::to_string(location.line());
	sErrorStr += ",";
	sErrorStr += std::to_string(location.column());
	sErrorStr += "]. Error description: ";
	sErrorStr += sErrorString;

	std::ofstream errorFile(ERROR_FILE_NAME);
	if (errorFile.is_open())
	{
		errorFile << sErrorStr;

		errorFile.close();
	}

	sErrorStr += "\n\n";
	sErrorStr += "This error message was saved at ";
	sErrorStr += fs::current_path().string() + "\\" + ERROR_FILE_NAME;

	MessageBoxA(0, sErrorStr.c_str(), "Error", 0);

	HRESULT error = S_OK;
	throw DXException(error);
}