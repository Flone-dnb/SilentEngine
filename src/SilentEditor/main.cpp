// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SilentEditor/EditorApplication/EditorApplication.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ PSTR szCmdLine, _In_ int iCmdShow)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	EditorApplication app(hInstance);

	//app.initCompileShadersInRelease();  // uncomment for more fps in debug build
	//app.initDisableD3DDebugLayer(); // not recommended, but uncomment for more fps in debug build

	app.setWindowTitleText(L"Silent Editor");

	app.getVideoSettings()->setInitFullscreen(false); // only for windowed apps, don't use this in your games

	if (app.init())
	{
		return 1;
	}

	app.setShowFrameStatsInWindowTitle(true); // only for windowed apps, don't use this in your games
	app.getVideoSettings()->setFPSLimit(120.0f);

	return app.run();
}