#pragma once

#include "../renderer/renderer.h"
#include "camera.h"
#include "scene.h"
#include "timer.h"
#include <string>

namespace Sendai {

struct App {
	std::wstring Title;
	std::wstring Window;
	HINSTANCE hInstance;
	HWND hWnd;
	BOOL bRunning;

	R_Core RendererCore;

	// S_UI UI;
	Camera Camera;
	StepTimer Timer;
	S_Scene Scene;
	UINT FrameCounter;
};

INT Run();
void AfterRun();

namespace Callback {
void DoNothing(App *const Engine);
void FileOpen(App *const Engine);
void WireframeMode(App *const Engine);
void GridMode(App *const Engine);
} // namespace Callback

} // namespace Sendai
