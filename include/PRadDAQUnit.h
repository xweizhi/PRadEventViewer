#ifndef PRAD_DAQ_UNIT_H
#define PRAD_DAQ_UNIT_H

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include "TH1.h"
#include "datastruct.h"

class PRadTDCGroup;

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
        double p0;
        double p1;

        CalibrationConstant()
        : factor(0), base_factor(0), p0(1.), p1(0.)
        {};
        CalibrationConstant(const double &calf,
                            const std::vector<double> &gain,
                            const double &pp0 = 0.,
                            const double &pp1 = 0.)
        : factor(calf), base_factor(calf), base_gain(gain), p0(pp0), p1(pp1)
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
    PRadDAQUnit(const std::string &name, const ChannelAddress &daqAddr, const std::string &tdc_name = "", const Geometry &geo = Geometry());
    virtual ~PRadDAQUnit();
    void AssignID(const unsigned short &id) {channelID = id;};
    void SetTDCGroup(PRadTDCGroup *t) {tdcGroup = t;};
    void UpdatePedestal(const Pedestal &ped);
    void UpdatePedestal(const double &m, const double &s);
    void UpdateCalibrationConstant(const CalibrationConstant &c) {cal_const = c;};
    void GainCorrection(const double &g, const int &ref) {cal_const.GainCorrection(g, ref);};
    void UpdateADC(const unsigned short &adcVal) {adc_value = adcVal;};
    void UpdateGeometry(const Geometry &geo) {geometry = geo;};
    void CleanBuffer();
    void ResetHistograms();
    void AddHist(const std::string &name);
    void MapHist(const std::string &name, PRadTriggerType type);

    bool IsHyCalModule() const {return (geometry.type == LeadGlass) || (geometry.type == LeadTungstate);};
    virtual double Calibration(const unsigned short &adcVal) const; // will be implemented by the derivative class
    virtual unsigned short Sparsification(const unsigned short &adcVal);
    static std::string NameFromPrimExID(int pid);

    unsigned short GetID() const {return channelID;};
    int GetPrimexID() const { return primexID; }
    int GetOccupancy() const {return occupancy;};
    double GetEnergy() const ;
    double GetEnergy(const unsigned short &adcVal) const;
    double GetX() const {return geometry.x;};
    double GetY() const {return geometry.y;};
    double GetCalibrationFactor() const {return cal_const.factor;};
    double GetReferenceGain(const size_t &ref) {return cal_const.GetReferenceGain(ref);};
    unsigned short GetADC() const {return adc_value;};
    ChannelAddress GetDAQInfo() const {return address;};
    Pedestal GetPedestal() const {return pedestal;};
    CalibrationConstant GetCalibrationConstant() const {return cal_const;};
    ChannelType GetType() const {return geometry.type;};
    Geometry GetGeometry() const {return geometry;};
    TH1 *GetHist(const std::string &name = "PHYS") const;
    TH1 *GetHist(PRadTriggerType type) const {return hist[(size_t)type];};
    std::vector<TH1*> GetHistList() const;
    std::string GetName() const {return channelName;};
    std::string GetTDCName() const {return tdcName;};
    PRadTDCGroup *GetTDCGroup() const {return tdcGroup;};

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
    std::string tdcName;
    PRadTDCGroup *tdcGroup;
    int occupancy;
    unsigned short sparsify;
    unsigned short channelID;
    unsigned short adc_value;
    int primexID;
    CalibrationConstant cal_const;
    TH1 *hist[MAX_Trigger];
    std::unordered_map<std::string, TH1*> histograms;
};

#endif
