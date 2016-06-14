#ifndef PRAD_DAQ_UNIT_H
#define PRAD_DAQ_UNIT_H

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include "TH1.h"
#include "datastruct.h"

class PRadDAQUnit
{
public:
    enum ChannelType
    {
        LeadGlass,
        LeadTungstate,
        Scintillator,
        LightMonitor,
        Undefined,
    };

    struct Geometry
    {
        ChannelType type;
        double size_x;
        double size_y;
        double x;
        double y;

        Geometry()
        : type(Undefined), size_x(0), size_y(0), x(0), y(0)
        {};
        Geometry(const ChannelType &t,
                 const double &sx,
                 const double &sy,
                 const double &pos_x,
                 const double &pos_y)
        : type(t), size_x(sx), size_y(sy), x(pos_x), y(pos_y)
        {};
    };

    struct Pedestal
    {
        double mean;
        double sigma;
        Pedestal() {};
        Pedestal(const double &m, const double &s)
        : mean(m), sigma(s) {};
    };

    struct CalibrationConstant
    {
        double factor;
        double base_factor;
        std::vector<double> base_gain;

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
    PRadDAQUnit(const std::string &name, const ChannelAddress &daqAddr, const std::string &tdc = "", const Geometry &geo = Geometry());
    virtual ~PRadDAQUnit();
    ChannelAddress GetDAQInfo() {return address;};
    Pedestal GetPedestal() {return pedestal;};
    std::string GetTDCName() {return tdcGroup;};
    void UpdatePedestal(const double &m, const double &s);
    void UpdateCalibrationConstant(const CalibrationConstant &c) {cal_const = c;};
    void GainCorrection(const double &g, const int &ref) {cal_const.GainCorrection(g, ref);};
    void UpdateADC(const unsigned short &adcVal) {adc_value = adcVal;};
    void UpdateGeometry(const Geometry &geo) {geometry = geo;};
    void CleanBuffer();
    void ResetHistograms();
    void AddHist(const std::string &name);
    void MapHist(const std::string &name, PRadTriggerType type);
    TH1 *GetHist(const std::string &name = "PHYS");
    TH1 *GetHist(PRadTriggerType type) {return hist[(size_t)type];};
    std::vector<TH1*> GetHistList();
    int GetOccupancy() {return occupancy;};
    const std::string &GetName() {return channelName;};
    
    void AssignID(const unsigned short &id) {channelID = id;};
    unsigned short GetID() {return channelID;};
    const double &GetCalibrationFactor() {return cal_const.factor;};
    const unsigned short &GetADC() {return adc_value;};
    double GetEnergy();
    double GetReferenceGain(const size_t &ref) {return cal_const.GetReferenceGain(ref);};
    Geometry GetGeometry() {return geometry;};
    double GetX() {return geometry.x;};
    double GetY() {return geometry.y;};
    const ChannelType &GetType() {return geometry.type;};
    bool IsHyCalModule() {return (geometry.type == LeadGlass) || (geometry.type == LeadTungstate);};
    virtual double Calibration(const unsigned short &adcVal); // will be implemented by the derivative class
    virtual bool Sparsification(const unsigned short &adcVal);

    template<typename T>
    void FillHist(const T& t, const size_t &pos)
    {
        if(hist[pos]) {
            hist[pos]->Fill(t);
        }
    };

    template<typename... Args>
    void AddHist(const std::string &n, const std::string &type, const std::string &title, Args&&... args)
    {
        if(GetHist(n) == nullptr) {
            std::string hist_name = channelName + "_" + n;
            switch(type.at(0))
            {
            case 'I':
            case 'i':
                histograms[n] = new TH1I(hist_name.c_str(), title.c_str(), std::forward<Args>(args)...);
                break;
            case 'F':
            case 'f':
                histograms[n] = new TH1F(hist_name.c_str(), title.c_str(), std::forward<Args>(args)...);
                break;
            case 'D':
            case 'd':
                histograms[n] = new TH1D(hist_name.c_str(), title.c_str(), std::forward<Args>(args)...);
                break;
            default:
                std::cout << "WARNING: " << channelName << ", does not support histogram type " << type << std::endl;
                break;
            }
        } else {
            std::cout << "WARNING: "<< channelName << ", hist " << n << " exists, skip adding it." << std::endl;
        }
    }


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
    Geometry geometry;
    ChannelAddress address;
    Pedestal pedestal;
    std::string tdcGroup;
    int occupancy;
    unsigned short sparsify;
    unsigned short channelID;
    unsigned short adc_value;
    CalibrationConstant cal_const;
    TH1 *hist[MAX_Trigger];
    std::unordered_map<std::string, TH1*> histograms;
};

#endif
