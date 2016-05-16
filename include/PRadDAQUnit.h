#ifndef PRAD_DAQ_UNIT_H
#define PRAD_DAQ_UNIT_H

#include <string>
#include <unordered_map>
#include "TH1.h"
#include "datastruct.h"

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
    void AddHist(const std::string &name);
    TH1 *GetHist(const std::string &name = "ADC");
    int GetOccupancy() {return occupancy;};
    std::string &GetName() {return channelName;};
    void AssignID(const unsigned short &id) {channelID = id;};
    unsigned short GetID() {return channelID;};
    const double &GetEnergy() {return energy;};
    TH1I *GetADCHist() {return adcHist;};
    TH1I *GetPEDHist() {return pedHist;};
    TH1I *GetLMSHist() {return lmsHist;};
    virtual unsigned short Sparsification(const unsigned short &adcVal);
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
    TH1I *pedHist;
    TH1I *lmsHist;
    std::unordered_map<std::string, TH1*> histograms;
};

#endif
