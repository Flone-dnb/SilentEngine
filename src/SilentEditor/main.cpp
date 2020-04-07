// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "SilentEngine/public/SApplication/SApplication.h"


class GameApplication : public SApplication
{
public:
	GameApplication(HINSTANCE hInstance) : SApplication(hInstance)
	{
		lastMousePosX = 0;
		lastMousePosY = 0;
	}

	void onTick() override
	{
		// Called every time before frame is drawn.
	}

	virtual void onMouseMove(SMouseKey mouseKey, int iMouseXPos, int iMouseYPos) override
	{
		if (mouseKey.getButton() == SMouseButton::SMB_LEFT)
		{
			// Make each pixel correspond to a quarter of a degree.
			float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(iMouseXPos - lastMousePosX));
			float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(iMouseYPos - lastMousePosY));

			// Update angles based on input to orbit camera around box.
			fTheta += dx;
			fPhi += dy;

			// Restrict the angle mPhi.
			fPhi = SMath::clamp(fPhi, 0.1f, SMath::fPi - 0.1f);
		}
		else if (mouseKey.getButton() == SMouseButton::SMB_RIGHT)
		{
			// Make each pixel correspond to 0.005 unit in the scene.
			float dx = 0.005f * static_cast<float>(iMouseXPos - lastMousePosX);
			float dy = 0.005f * static_cast<float>(iMouseYPos - lastMousePosY);

			// Update the camera radius based on input.
			fRadius += dx - dy;

			// Restrict the radius.
			fRadius = SMath::clamp(fRadius, 3.0f, 15.0f);
		}

		lastMousePosX = iMouseXPos;
		lastMousePosY = iMouseYPos;
	}

	virtual void onMouseDown(SMouseKey mouseKey, int iMouseXPos, int iMouseYPos) override
	{
		lastMousePosX = iMouseXPos;
		lastMousePosY = iMouseYPos;

		SetCapture(getMainWindowHandle());
	}

	virtual void onMouseUp(SMouseKey mouseKey, int iMouseXPos, int iMouseYPos) override
	{
		ReleaseCapture();
	}

	~GameApplication() override
	{

	}

private:

	long lastMousePosX;
	long lastMousePosY;
};




int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	GameApplication app(hInstance);

	app.setInitMainWindowTitle(L"Silent Editor");
	app.setCallTick(true);
	app.setInitEnableVSync(true);

	if (app.init())
	{
		return 1;
	}

	return app.run();
}