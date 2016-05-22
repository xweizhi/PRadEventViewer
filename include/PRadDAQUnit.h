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
    void UpdateCalibrationFactor(const double &c) {calf = c;};
    void UpdateEnergy(const unsigned short &adcVal);
    void CleanBuffer();
    void AddHist(const std::string &name);
    template<typename... Args>
    void AddHist(const std::string &name, const std::string &type, Args&&... args);
    void MapHist(const std::string &name, PRadTriggerType type);
    TH1 *GetHist(const std::string &name = "ADC");
    TH1 *GetHist(PRadTriggerType type) {return hist[(size_t)type];};
    int GetOccupancy() {return occupancy;};
    std::string &GetName() {return channelName;};
    void AssignID(const unsigned short &id) {channelID = id;};
    unsigned short GetID() {return channelID;};
    const bool &IsHyCalChannel() {return in_hycal;};
    const double &GetCalibrationFactor() {return calf;};
    const double &GetEnergy() {return energy;};
    virtual double Calibration(const unsigned short &adcVal); // will be implemented by the derivative class
    virtual unsigned short Sparsification(const unsigned short &adcVal, const bool &count = true);

protected:
    std::string channelName;
    ChannelAddress address;
    Pedestal pedestal;
    std::string tdcGroup;
    int occupancy;
    unsigned short sparsify;
    unsigned short channelID;
    double calf;
    double energy;
    bool in_hycal;
    TH1 *hist[MAX_Trigger];
    std::unordered_map<std::string, TH1*> histograms;
};

#endif
