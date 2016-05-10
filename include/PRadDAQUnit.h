#ifndef PRAD_DAQ_UNIT_H
#define PRAD_DAQ_UNIT_H

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
    PRadDAQUnit(const std::string &name, const ChannelAddress &daqAddr, const std::string &tdc = "");
    virtual ~PRadDAQUnit();
    ChannelAddress GetDAQInfo() {return address;};
    Pedestal GetPedestal() {return pedestal;};
    std::string GetTDCName() {return tdcGroup;};
    void UpdatePedestal(const double &m, const double &s);
    void UpdateEnergy(const unsigned short &adcVal);
    void CleanBuffer();
    unsigned short Sparsification(unsigned short &adcVal);
    int GetOccupancy() {return occupancy;};
    std::string &GetName() {return channelName;};
    void AssignID(const unsigned short &id) {channelID = id;};
    unsigned short GetID() {return channelID;};
    const double &GetEnergy() {return energy;};
    TH1I *GetHist() {return adcHist;};
    virtual double Calibration(const unsigned short &adcVal); // will be implemented by the derivative class

protected:
    ChannelAddress address;
    Pedestal pedestal;
    std::string tdcGroup;
    std::string channelName;
    int occupancy;
    unsigned short sparsify;
    unsigned short channelID;
    double energy;
    TH1I *adcHist;
};

#endif
