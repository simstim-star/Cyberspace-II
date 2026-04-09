#pragma once

#include "camera.h"
#include "scene.h"
#include "timer.h"

namespace Sendai
{
class UI;
struct Renderer;

struct AppWindow
{
    AppWindow(std::wstring Title, UINT Width, UINT Height) : Title{Title}, Width{Width}, Height{Height}
    {
    }

    std::wstring Title;
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
    std::unique_ptr<UI> UI;

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
