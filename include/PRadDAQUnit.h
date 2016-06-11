#ifndef PRAD_DAQ_UNIT_H
#define PRAD_DAQ_UNIT_H

#include <string>
#include <vector>
#include <unordered_map>
#include "TH1.h"
#include "datastruct.h"

class PRadDAQUnit
{
public:
    enum ChannelType
    {
        Undefined,
        HyCalModule,
        Scintillator,
        LMS_PMT,
    };

    struct Pedestal
    {
        double mean;
        double sigma;
        Pedestal() {};
        Pedestal(const double &m, const double &s)
        : mean(m), sigma(s) {};
    };

    class CalibrationConstant
    {
    public:
        double factor;
        double base_factor;
        std::vector<double> base_gain;

    public:
        CalibrationConstant()
        : factor(0), base_factor(0)
        {};
        CalibrationConstant(const double &calf, const std::vector<double> &gain)
        : factor(calf), base_factor(calf), base_gain(gain)
        {};

        void AddReferenceGain(const double &gain)
        {
            base_gain.push_back(gain);
        };

        size_t GetReferenceNumber()
        {
            return base_gain.size();
        };

        double GetReferenceGain(const size_t &ref)
        {
            if((ref == 0) || (ref > base_gain.size()))
                return 0.;
            return base_gain.at(ref-1);
        };

        void GainCorrection(const double &gain, const size_t &ref)
        {
            double base = GetReferenceGain(ref);

            if((gain > 0.) && (base > 0.)) {
                factor = base_factor * base/gain;
            }
        };
    };


public:
    PRadDAQUnit(const std::string &name, const ChannelAddress &daqAddr, const std::string &tdc = "");
    virtual ~PRadDAQUnit();
    ChannelAddress GetDAQInfo() {return address;};
    Pedestal GetPedestal() {return pedestal;};
    std::string GetTDCName() {return tdcGroup;};
    void UpdatePedestal(const double &m, const double &s);
    void UpdateCalibrationConstant(const CalibrationConstant &c) {cal_const = c;};
    void GainCorrection(const double &g, const int &ref) {cal_const.GainCorrection(g, ref);};
    void UpdateEnergy(const unsigned short &adcVal);
    void UpdateType(const ChannelType &t) {type = t;};
    void CleanBuffer();
    void AddHist(const std::string &name);
    template<typename... Args>
    void AddHist(const std::string &name, const std::string &type, Args&&... args);
    void MapHist(const std::string &name, PRadTriggerType type);
    template<typename T>
    void FillHist(const T& t, const PRadTriggerType &type);
    TH1 *GetHist(const std::string &name = "PHYS");
    TH1 *GetHist(PRadTriggerType type) {return hist[(size_t)type];};
    std::vector<TH1*> GetHistList();
    int GetOccupancy() {return occupancy;};
    const std::string &GetName() {return channelName;};
    const ChannelType &GetType() {return type;};
    
    void AssignID(const unsigned short &id) {channelID = id;};
    unsigned short GetID() {return channelID;};
    const double &GetCalibrationFactor() {return cal_const.factor;};
    const double &GetEnergy() {return energy;};
    double GetReferenceGain(const size_t &ref) {return cal_const.GetReferenceGain(ref);};
    virtual double Calibration(const unsigned short &adcVal); // will be implemented by the derivative class
    virtual unsigned short Sparsification(const unsigned short &adcVal, const bool &count = true);

    bool operator < (const PRadDAQUnit &rhs) const
    {
        auto name_to_val = [](std::string name)
                           {
                               if(name.at(0) == 'W') return 1000;
                               if(name.at(0) == 'G') return 0;
                               else return (int)name.at(0)*10000;
                           };

        int lhs_val = name_to_val(channelName);
        int rhs_val = name_to_val(rhs.channelName);

        size_t idx = channelName.find_first_of("1234567890");
        if(idx != std::string::npos)
            lhs_val += std::stoi(channelName.substr(idx, -1));

        idx = rhs.channelName.find_first_of("1234567890");
        if(idx != std::string::npos)
            rhs_val += std::stoi(rhs.channelName.substr(idx, -1));

        return lhs_val < rhs_val;
    }

protected:
    std::string channelName;
    ChannelType type;
    ChannelAddress address;
    Pedestal pedestal;
    std::string tdcGroup;
    int occupancy;
    unsigned short sparsify;
    unsigned short channelID;
    CalibrationConstant cal_const;
    double energy;
    TH1 *hist[MAX_Trigger];
    std::unordered_map<std::string, TH1*> histograms;
};

#endif
