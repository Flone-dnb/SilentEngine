// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************


#pragma once

// STL
#include <source_location>
#include <string>

#include <comdef.h> // for _com_error


class DXException
{
public:
	DXException(HRESULT hresult)
	{
		this->hresult       = hresult;
	}

private:

	HRESULT hresult = S_OK;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hresult = (x);                                            \
    if(FAILED(hresult)) { throw DXException(hresult); }               \
}
#endif


class SError
{
public:

	static void showErrorMessageBoxAndLog(HRESULT hresult, const std::source_location& location = std::source_location::current());
	static void showErrorMessageBoxAndLog(std::string sErrorString, const std::source_location& location = std::source_location::current());
};