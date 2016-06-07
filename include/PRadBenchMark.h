#ifndef PRAD_BENCH_MARK_H
#define PRAD_BENCH_MARK_H

#include "sys/time.h"

class PRadBenchMark
{
public:
    PRadBenchMark();
    ~PRadBenchMark();
    void Start();
    void Stop();
    unsigned int GetElapsedTime();

private:
    struct timeval timeStart, timeStop;
};

#endif
