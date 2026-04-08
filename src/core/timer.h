/*
 * Based on
 * https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12MeshShaders/src/DynamicLOD/StepTimer.h
 */

#pragma once
#include <functional>

namespace Sendai
{

// Integer format represents time using 10,000,000 ticks per second.
#define TICKS_PER_SECOND 10000000

// Helper class for animation and simulation timing.
class StepTimer
{
  public:
    StepTimer();

    // After an intentional timing discontinuity (for instance a blocking IO operation)
    // call this to aVOID having the fixed timestep logic attempt a set of catch-up
    // Update calls.
    VOID ResetElapsedTime();

    // Update timer state, calling the specified Update function the appropriate number of times.
    VOID TickWithUpdateFn(const std::function<VOID()> &UpdateFunc);

    // Only updates timer state, but doesn't do anything else
    inline VOID Tick()
    {
        TickWithUpdateFn(nullptr);
    }

    inline FLOAT ElapsedSeconds() const
    {
        return static_cast<FLOAT>(ElapsedTicks) / TICKS_PER_SECOND;
    }

  private:
    // Source timing data uses QueryPerformanceCounter units.
    LARGE_INTEGER QPCFrequency;
    LARGE_INTEGER QPCLastTime;
    UINT64 QPCMaxDelta;
    // Derived timing data uses a canonical tick format.
    UINT64 ElapsedTicks;
    UINT64 TotalTicks;
    UINT64 LeftoverTicks;
    // Members for tracking the framerate.
    UINT32 FrameCount;
    UINT32 FramesPerSecond;
    UINT32 FramesThisSecond;
    UINT64 QPCSecondCounter;
    // Members for configuring fixed timestep mode.
    BOOL bFixedTimeStep;
    UINT64 TargetElapsedTicks;
};

} // namespace Sendai
