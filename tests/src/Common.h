// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

#define SLEEP_BETWEEN_TESTS_MS 200

#include "Catch2/catch.hpp"

#include <thread>
#include <iostream>
#include <chrono>
#include <future>

#include "SilentEditor/EditorApplication/EditorApplication.h"
#pragma comment(lib, "SilentEditor.lib")
#pragma comment(lib, "DirectXTK12.lib")