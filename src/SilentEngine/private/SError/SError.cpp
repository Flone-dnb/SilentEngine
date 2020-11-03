// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#include "SError.h"

#include <windows.h>
#include <winnt.h>

#include "SilentEngine/Public/SApplication/SApplication.h"

void SError::showErrorMessageBox(HRESULT hresult, std::wstring sPathToFailedFunction)
{
	LPTSTR errorText = NULL;

	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		hresult,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPTSTR)&errorText,
		0,
		NULL);

	if (errorText != NULL)
	{
		std::wstring sErrorString = L"An error occurred at " + sPathToFailedFunction + L". Error description: " + std::wstring(errorText);

		MessageBox(0, sErrorString.c_str(), L"Error", 0);

		LocalFree(errorText);
	}
	else
	{
		std::wstring sErrorString = L"An unknown error occurred at " + sPathToFailedFunction;

		MessageBox(0, sErrorString.c_str(), L"Error", 0);
	}

	if (hresult == 0x887a0005)
	{
		// ?
		SApplication::getApp()->showDeviceRemovedReason();
	}

	HRESULT error = hresult;
	ThrowIfFailed(error);
}

void SError::showErrorMessageBox(std::wstring sPathToFailedFunction, std::wstring sErrorString)
{
	std::wstring sErrorStr = L"An error occured at " + sPathToFailedFunction + L". Error description: " + sErrorString;

	MessageBox(0, sErrorStr.c_str(), L"Error", 0);

	HRESULT error = S_OK;
	throw DXException(error);
}
