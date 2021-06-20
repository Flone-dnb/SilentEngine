// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#define CATCH_CONFIG_RUNNER
#include <iostream>
#include "Catch2/catch.hpp"

int main(int argc, char* argv[]){
//int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
//	_In_ PSTR szCmdLine, _In_ int iCmdShow){

	const int res = Catch::Session().run();
    
#if _WIN32
	std::system("pause");
#endif

	return 0;
}
