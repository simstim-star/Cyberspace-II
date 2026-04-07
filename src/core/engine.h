#pragma once

#include "../renderer/renderer.h"
#include "camera.h"
#include "scene.h"
#include "timer.h"
#include <string>
#include <memory>

namespace Sendai {

struct AppWindow {
	AppWindow(std::wstring Title, UINT Width, UINT Height) : Title{Title}, Width{Width}, Height{Height} {}

	std::wstring Title;
	std::wstring Window;
	HINSTANCE hInstance;
	HWND hWnd;
	UINT Width;
	UINT Height;
	FLOAT AspectRatio;
};

struct App {
	App(std::wstring Title);
	VOID Update();

	AppWindow Window;
	BOOL bRunning;

	std::unique_ptr<Renderer> RendererCore;

	// S_UI UI;
	Camera Camera;
	StepTimer Timer;
	Sendai::Scene Scene;
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
