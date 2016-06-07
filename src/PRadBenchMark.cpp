#include "PRadBenchMark.h"

PRadBenchMark::PRadBenchMark()
{
}

PRadBenchMark::~PRadBenchMark()
{
}

void PRadBenchMark::Start()
{
    gettimeofday(&timeStart, nullptr);
}

void PRadBenchMark::Stop()
{
    gettimeofday(&timeStop, nullptr);
}

unsigned int PRadBenchMark::GetElapsedTime()
{
    return (unsigned int)((timeStop.tv_sec - timeStart.tv_sec)*1000 + (timeStop.tv_usec - timeStart.tv_usec)/1000);
}
