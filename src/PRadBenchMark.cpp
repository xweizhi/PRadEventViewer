#include "PRadBenchMark.h"

PRadBenchMark::PRadBenchMark()
{
    Reset();
}

PRadBenchMark::~PRadBenchMark()
{
}

void PRadBenchMark::Reset()
{
    time_point = std::chrono::high_resolution_clock::now();
}

unsigned int PRadBenchMark::GetElapsedTime()
{
    auto time_end = std::chrono::high_resolution_clock::now();
    auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_point);
    return int_ms.count();
}
