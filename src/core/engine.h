#pragma once

#include "../renderer/renderer.h"
#include "camera.h"
#include "scene.h"
#include "timer.h"
#include <memory>
#include <string>

namespace Sendai
{

struct AppWindow
{
    AppWindow(std::wstring Title, UINT Width, UINT Height) : Title{Title}, Width{Width}, Height{Height}
    {
    }

    std::wstring Title;
    std::wstring Window;
    HINSTANCE hInstance;
    HWND hWnd;
    UINT Width;
    UINT Height;
    FLOAT AspectRatio;
};

struct App
{
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
VOID AfterRun();

namespace Callback
{
VOID DoNothing(App *const Engine);
VOID FileOpen(App *const Engine);
VOID WireframeMode(App *const Engine);
VOID GridMode(App *const Engine);
} // namespace Callback

} // namespace Sendai
