#pragma once

#include <chrono>

#include "Artifice/Core/Core.h"


class Timer
{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
public:
    Timer();
    void Reset();
    double Elapsed();
    float ElapsedF();
    float ElapsedMillis();
    uint64 ElapsedMicros();
    uint64 ElapsedNanos();


    static uint64 GetCurrentMicros();
};