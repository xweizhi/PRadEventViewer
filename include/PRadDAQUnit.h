#ifndef PRAD_DAQ_UNIT
#define PRAD_DAQ_UNIT

#include <string>
#include "datastruct.h"

class TH1I;

class PRadDAQUnit
{
public:
    struct Pedestal
    {
        double mean;
        double sigma;
        Pedestal() {};
        Pedestal(const double &m, const double &s)
        : mean(m), sigma(s) {};
    };

public:
    PRadDAQUnit(const char *name, const ChannelAddress &daqAddr, const int &tdc = 0);
    virtual ~PRadDAQUnit();
    ChannelAddress GetDAQInfo() {return address;};
    Pedestal GetPedestal() {return pedestal;};
    int GetTDCID() {return tdcGroup;};
    void UpdatePedestal(const double &m, const double &s);
    void CleanBuffer();
    unsigned short Sparsification(unsigned short &adcVal);
    int GetOccupancy() {return occupancy;};
    TH1I *adcHist;

protected:
    ChannelAddress address;
    Pedestal pedestal;
    int tdcGroup;
    int occupancy;
    unsigned short sparsify;
};

#endif
