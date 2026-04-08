/*
 * Based on
 * https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12MeshShaders/src/DynamicLOD/StepTimer.h
 */

#include "pch.h"

#include "timer.h"

using Sendai::StepTimer;

StepTimer::StepTimer()
{
    ElapsedTicks = 0;
    TotalTicks = 0;
    LeftoverTicks = 0;
    FrameCount = 0;
    FramesPerSecond = 0;
    FramesThisSecond = 0;
    QPCSecondCounter = 0;
    bFixedTimeStep = FALSE;
    TargetElapsedTicks = TICKS_PER_SECOND / 60;

    QueryPerformanceFrequency(&QPCFrequency);
    QueryPerformanceCounter(&QPCLastTime);

    // Initialize max delta to 1/10 of a second.
    QPCMaxDelta = QPCFrequency.QuadPart / 10;
}

// After an intentional timing discontinuity (for instance a blocking IO operation)
// call this to aVOID having the fixed timestep logic attempt a set of catch-up
// Update calls.
VOID StepTimer::ResetElapsedTime()
{
    QueryPerformanceCounter(&QPCLastTime);

    LeftoverTicks = 0;
    FramesPerSecond = 0;
    FramesThisSecond = 0;
    QPCSecondCounter = 0;
}

// Update timer state, calling the specified Update function the appropriate number of times.
VOID StepTimer::TickWithUpdateFn(const std::function<VOID()> &Update)
{
    // Query the current time.
    LARGE_INTEGER CurrentTime;
    QueryPerformanceCounter(&CurrentTime);
    // Update delta since last call
    UINT64 Delta = CurrentTime.QuadPart - QPCLastTime.QuadPart;
    QPCLastTime = CurrentTime;
    QPCSecondCounter += Delta;
    // Clamp excessively large time deltas (e.g. after paused in the debugger).
    if (Delta > QPCMaxDelta)
    {
        Delta = QPCMaxDelta;
    }
    // Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
    Delta *= TICKS_PER_SECOND;
    Delta /= QPCFrequency.QuadPart;

    UINT32 LastFrameCount = FrameCount;

    if (bFixedTimeStep)
    {
        // Fixed timestep update logic

        // If the app is running very close to the target elapsed time (within 1/4 of a millisecond) just clamp
        // the clock to exactly match the target value. This prevents tiny and irrelevant errors
        // from accumulating over time. Without this clamping, a game that requested a 60 fps
        // fixed update, running with vsync enabled on a 59.94 NTSC display, would eventually
        // accumulate enough tiny errors that it would drop a frame. It is better to just round
        // small deviations down to zero to leave things running smoothly.
        if (abs((int)(Delta - TargetElapsedTicks)) < TICKS_PER_SECOND / 4000)
        {
            Delta = TargetElapsedTicks;
        }
        // Add to the leftover. If the accumulated leftover passes a certain target, we will need to take some actions
        // to ensure that the time step is still fixed.
        LeftoverTicks += Delta;

        // This will be done every time the leftover reaches the target. In case it has reached the target exaclty, it
        // will do what is expected: just step with the target. If it surpases, we will still step with target, but we
        // will also register a leftover that will propagate to the next tick (this leftover will be the difference
        // timeDelta - target)
        while (LeftoverTicks >= TargetElapsedTicks)
        {
            ElapsedTicks = TargetElapsedTicks;
            TotalTicks += TargetElapsedTicks;
            LeftoverTicks -= TargetElapsedTicks; // will be zero if st->leftOverTicks == st->targetElapsedTicks
            FrameCount++;

            if (Update)
            {
                Update();
            }
        }
    }
    else
    {
        // Variable timestep update logic.
        ElapsedTicks = Delta;
        TotalTicks += Delta;
        LeftoverTicks = 0;
        FrameCount++;

        if (Update)
        {
            Update();
        }
    }

    // Track the current framerate.
    if (FrameCount != LastFrameCount)
    {
        FramesThisSecond++;
    }

    if (QPCSecondCounter >= (UINT64)(QPCFrequency.QuadPart))
    {
        FramesPerSecond = FramesThisSecond;
        FramesThisSecond = 0;
        QPCSecondCounter %= QPCFrequency.QuadPart;
    }
}