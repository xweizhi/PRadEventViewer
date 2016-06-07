#ifndef PRAD_BENCH_MARK_H
#define PRAD_BENCH_MARK_H

#include <chrono>

class PRadBenchMark
{
public:
    PRadBenchMark();
    ~PRadBenchMark();
    void Reset();
    unsigned int GetElapsedTime();

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
};

#endif
