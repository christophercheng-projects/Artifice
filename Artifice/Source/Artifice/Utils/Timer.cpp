#include "Timer.h"


Timer::Timer()
{
    Reset();
}

void Timer::Reset()
{
    m_Start = std::chrono::high_resolution_clock::now();
}

double Timer::Elapsed()
{
    return std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - m_Start).count();
}

float Timer::ElapsedF()
{
    return std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(std::chrono::high_resolution_clock::now() - m_Start).count() / 1000.0f;
}

float Timer::ElapsedMillis()
{
    return std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(std::chrono::high_resolution_clock::now() - m_Start).count();
}

uint64 Timer::ElapsedMicros()
{
    return (uint64)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_Start).count();
}

uint64 Timer::ElapsedNanos()
{
    return (uint64)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count();
}

uint64 Timer::GetCurrentMicros()
{
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}
