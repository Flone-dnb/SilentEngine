// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "Common.h"

TEST_CASE("Initialize and run the engine with default settings.", "[MainTests::initAndRun]") {
	EditorApplication app(GetModuleHandle(NULL));

	std::promise<bool> promiseReady;
	std::promise<bool> promiseFinishRun;
	std::future<bool> futureFinishRun = promiseFinishRun.get_future();
	std::future<bool> futureReady = promiseReady.get_future();

	std::thread t([&app, &promiseReady, &promiseFinishRun](){
		//app.initCompileShadersInRelease();  // uncomment for more fps in debug build
		//app.initDisableD3DDebugLayer(); // not recommended, but uncomment for more fps in debug build
		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_BETWEEN_TESTS_MS));
		if (app.init(L"MainWindow_" + std::to_wstring(time(0))))
		{
			promiseReady.set_value(true);
			promiseFinishRun.set_value(true);
			//REQUIRE(false);
			return;
		}
		app.getVideoSettings()->setFPSLimit(120.0f);
		promiseReady.set_value(false);
		app.run();
		promiseFinishRun.set_value(false);
	});
	t.detach();

	bool bError = futureReady.get();
	if (bError)
	{
		REQUIRE(false); // init failed
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	app.hideWindow(); // hide for testing

	PostMessage(app.getMainWindowHandle(), WM_QUIT, 0, 0);

	futureFinishRun.get();

	REQUIRE(true);
}

TEST_CASE("Register/unregister materials.", "[MainTests::registerUnregisterMaterials]") {
	EditorApplication app(GetModuleHandle(NULL));

	std::promise<bool> promiseReady;
	std::promise<bool> promiseFinishRun;
	std::future<bool> futureFinishRun = promiseFinishRun.get_future();
	std::future<bool> futureReady = promiseReady.get_future();

	std::thread t([&app, &promiseReady, &promiseFinishRun]() {
		//app.initCompileShadersInRelease();  // uncomment for more fps in debug build
		//app.initDisableD3DDebugLayer(); // not recommended, but uncomment for more fps in debug build
		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_BETWEEN_TESTS_MS));
		if (app.init(L"MainWindow_" + std::to_wstring(time(0))))
		{
			promiseReady.set_value(true);
			promiseFinishRun.set_value(true);
			//REQUIRE(false);
			return;
		}
		app.getVideoSettings()->setFPSLimit(120.0f);
		promiseReady.set_value(false);
		app.run();
		promiseFinishRun.set_value(false);
		});
	t.detach();

	bool bError = futureReady.get();
	if (bError)
	{
		REQUIRE(false); // init failed
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	app.hideWindow(); // hide for testing


	// --------------------------------------------------

	size_t iMatCountBefore = app.getRegisteredMaterials()->size(); // should be 1 for engine material

	// Register 2 materials.

	const std::string sMat1 = "test_mat1";
	const std::string sMat2 = "test_mat2";

	bError = false;
	SMaterial* pMat1 = app.registerMaterial(sMat1, bError);
	if (bError)
	{
		REQUIRE(false);
	}

	bError = false;
	SMaterial* pMat2 = app.registerMaterial(sMat2, bError);
	if (bError)
	{
		REQUIRE(false);
	}

	// Check size.

	REQUIRE(app.getRegisteredMaterials()->size() == 2 + iMatCountBefore);

	// Unregister.

	if (app.unregisterMaterial(sMat1))
	{
		REQUIRE(false);
	}

	if (app.unregisterMaterial(sMat2))
	{
		REQUIRE(false);
	}

	// Check size.

	REQUIRE(app.getRegisteredMaterials()->size() == iMatCountBefore);


	// --------------------------------------------------

	PostMessage(app.getMainWindowHandle(), WM_QUIT, 0, 0);

	futureFinishRun.get();

	REQUIRE(true);
}

TEST_CASE("Load/unload textures.", "[MainTests::loadUnloadTextures]") {
	EditorApplication app(GetModuleHandle(NULL));

	std::promise<bool> promiseReady;
	std::promise<bool> promiseFinishRun;
	std::future<bool> futureFinishRun = promiseFinishRun.get_future();
	std::future<bool> futureReady = promiseReady.get_future();

	std::thread t([&app, &promiseReady, &promiseFinishRun]() {
		//app.initCompileShadersInRelease();  // uncomment for more fps in debug build
		//app.initDisableD3DDebugLayer(); // not recommended, but uncomment for more fps in debug build
		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_BETWEEN_TESTS_MS));
		if (app.init(L"MainWindow_" + std::to_wstring(time(0))))
		{
			promiseReady.set_value(true);
			promiseFinishRun.set_value(true);
			//REQUIRE(false);
			return;
		}
		app.getVideoSettings()->setFPSLimit(120.0f);
		promiseReady.set_value(false);
		app.run();
		promiseFinishRun.set_value(false);
		});
	t.detach();

	bool bError = futureReady.get();
	if (bError)
	{
		REQUIRE(false); // init failed
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	app.hideWindow(); // hide for testing


	// --------------------------------------------------

	// Load texture.

	const std::wstring sTex = L"assets/tex.dds";

	bError = false;
	STextureHandle tex = app.loadTextureFromDiskToGPU("tex", sTex, bError);
	if (bError)
	{
		REQUIRE(false);
	}

	// Check size.

	REQUIRE(app.getLoadedTextures().size() == 1);

	// Unload texture.
	if (app.unloadTextureFromGPU(tex))
	{
		REQUIRE(false);
	}

	// --------------------------------------------------

	PostMessage(app.getMainWindowHandle(), WM_QUIT, 0, 0);

	futureFinishRun.get();

	REQUIRE(true);
}

TEST_CASE("Compile custom shader.", "[MainTests::compileCustomShader]") {
	EditorApplication app(GetModuleHandle(NULL));

	std::promise<bool> promiseReady;
	std::promise<bool> promiseFinishRun;
	std::future<bool> futureFinishRun = promiseFinishRun.get_future();
	std::future<bool> futureReady = promiseReady.get_future();

	std::thread t([&app, &promiseReady, &promiseFinishRun]() {
		//app.initCompileShadersInRelease();  // uncomment for more fps in debug build
		//app.initDisableD3DDebugLayer(); // not recommended, but uncomment for more fps in debug build
		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_BETWEEN_TESTS_MS));
		if (app.init(L"MainWindow_" + std::to_wstring(time(0))))
		{
			promiseReady.set_value(true);
			promiseFinishRun.set_value(true);
			//REQUIRE(false);
			return;
		}
		app.getVideoSettings()->setFPSLimit(120.0f);
		promiseReady.set_value(false);
		app.run();
		promiseFinishRun.set_value(false);
		});
	t.detach();

	bool bError = futureReady.get();
	if (bError)
	{
		REQUIRE(false); // init failed
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	app.hideWindow(); // hide for testing


	// --------------------------------------------------

	// Compile.

	SShader* pShader = app.compileCustomShader(L"shaders/basic.hlsl", SCustomShaderProperties());
	if (pShader == nullptr)
	{
		REQUIRE(false);
	}

	// Check size.

	REQUIRE(app.getCompiledCustomShaders()->size() == 1);

	// Unload.
	
	if (app.unloadCompiledShaderFromGPU(pShader))
	{
		REQUIRE(false);
	}

	// --------------------------------------------------

	PostMessage(app.getMainWindowHandle(), WM_QUIT, 0, 0);

	futureFinishRun.get();

	REQUIRE(true);
}

TEST_CASE("Compile custom compute shader.", "[MainTests::compileCustomComputeShader]") {
	EditorApplication app(GetModuleHandle(NULL));

	std::promise<bool> promiseReady;
	std::promise<bool> promiseFinishRun;
	std::future<bool> futureFinishRun = promiseFinishRun.get_future();
	std::future<bool> futureReady = promiseReady.get_future();

	std::thread t([&app, &promiseReady, &promiseFinishRun]() {
		//app.initCompileShadersInRelease();  // uncomment for more fps in debug build
		//app.initDisableD3DDebugLayer(); // not recommended, but uncomment for more fps in debug build
		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_BETWEEN_TESTS_MS));
		if (app.init(L"MainWindow_" + std::to_wstring(time(0))))
		{
			promiseReady.set_value(true);
			promiseFinishRun.set_value(true);
			//REQUIRE(false);
			return;
		}
		app.getVideoSettings()->setFPSLimit(120.0f);
		promiseReady.set_value(false);
		app.run();
		promiseFinishRun.set_value(false);
		});
	t.detach();

	bool bError = futureReady.get();
	if (bError)
	{
		REQUIRE(false); // init failed
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	app.hideWindow(); // hide for testing


	// --------------------------------------------------

	// Register.

	SComputeShader* pShader = app.registerCustomComputeShader("test_Shader");
	if (pShader == nullptr)
	{
		REQUIRE(false);
	}

	size_t iMatrixSize = 3;
	std::vector<float> vMatrix1(iMatrixSize * 3, 1.0f);
	std::vector<float> vMatrix2(iMatrixSize * 3, 1.0f);
	pShader->compileShader(L"assets/test_matrix_compute.hlsl", L"matrixCalc");
	if (pShader->setAdd32BitConstant(iMatrixSize, "fMatrixSize", 0))
	{
		REQUIRE(false);
	}
	if (pShader->setAddData(true, "matrix1", vMatrix1.size() * sizeof(float), 0, vMatrix1.data()))
	{
		REQUIRE(false);
	}
	if (pShader->setAddData(true, "matrix2", vMatrix2.size() * sizeof(float), 1, vMatrix2.data()))
	{
		REQUIRE(false);
	}
	if (pShader->setAddData(false, "outMatrix", vMatrix1.size() * sizeof(float), 0))
	{
		REQUIRE(false);
	}

	// Execute.

	if (pShader->startShaderExecution(8, 8, 8))
	{
		REQUIRE(false);
	}

	// Wait for results.

	std::promise<bool> promiseShaderFinish;
	std::future<bool> futureShaderFinish = promiseShaderFinish.get_future();

	std::function<void(std::vector<char*>, std::vector<size_t>)> lambda = [&promiseShaderFinish](std::vector<char*> vData, std::vector<size_t> vSize){
		std::vector<float> vOutMatrix;

		size_t iCount = vSize[0] / static_cast<float>(sizeof(float));
		for (size_t i = 0; i < iCount; i++)
		{
			float fValue = 0.0f;
			std::memcpy(&fValue, vData[0] + (i * sizeof(float)), sizeof(float));
			vOutMatrix.push_back(fValue);

			if (vOutMatrix.back() > 3.1f || vOutMatrix.back() < 2.9f)
			{
				REQUIRE(false); // the result should be 3
			}
		}
		
		promiseShaderFinish.set_value(false);
	};
	if (pShader->copyComputeResults({ "outMatrix" }, true, lambda))
	{
		REQUIRE(false);
	}

	futureShaderFinish.get();

	// Get size.
	REQUIRE(app.getRegisteredComputeShaders()->size() == 1);

	// Unregister.

	app.unregisterCustomComputeShader(pShader);

	// --------------------------------------------------

	PostMessage(app.getMainWindowHandle(), WM_QUIT, 0, 0);

	futureFinishRun.get();

	REQUIRE(true);
}