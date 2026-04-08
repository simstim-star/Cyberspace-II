#include "../core/pch.h"

#include "../core/engine.h"

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    const INT ReturnCode = Sendai::Run();
    Sendai::AfterRun();
    return ReturnCode;
}