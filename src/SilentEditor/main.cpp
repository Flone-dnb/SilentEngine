// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SilentEditor/EditorApplication/EditorApplication.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	EditorApplication app(hInstance);

	//app.initCompileShadersInRelease();

	app.setWindowTitleText(L"Silent Editor");

	if (app.init())
	{
		return 1;
	}

	app.setShowFrameStatsInWindowTitle(true);
	app.getVideoSettings()->setFPSLimit(120.0f);

	return app.run();
}