//============================================================================//
// Basic DAQ channel unit                                                     //
// It could be HyCal Module, LMS PMT or Scintillator                          //
// Chao Peng                                                                  //
// 02/17/2016                                                                 //
//============================================================================//

#include "PRadDAQUnit.h"
#include <utility>

PRadDAQUnit::PRadDAQUnit(const std::string &name,
                         const ChannelAddress &daqAddr,
                         const std::string &tdc,
                         const Geometry &geo)
: channelName(name), geometry(geo), address(daqAddr), pedestal(Pedestal(0, 0)),
  tdcGroup(tdc), occupancy(0), sparsify(0), channelID(0), adc_value(0)
{
    std::string hist_name;

    for(auto &member: hist)
    {
        member = nullptr;
    }

    // some default histograms
    AddHist("PHYS", "Integer", (channelName + " Physics Events"), 2048, 0, 8191);
    AddHist("PED", "Integer", (channelName + " Pedestal Events"), 1024, 0, 1023);
    AddHist("LMS", "Integer", (channelName + " LED Source"), 2048, 0, 8191);

    // default hist-trigger mapping
//    MapHist("PED", TI_Error);
    if(channelName.find("LMS") != std::string::npos) {
        MapHist("PHYS", LMS_Alpha);
        MapHist("PED", PHYS_LeadGlassSum);
        MapHist("PED", PHYS_TotalSum);
        MapHist("PED", PHYS_TaggerE);
        MapHist("PED", PHYS_Scintillator);
    } else {
        MapHist("PED", LMS_Alpha);
        MapHist("PHYS", PHYS_LeadGlassSum);
        MapHist("PHYS", PHYS_TotalSum);
        MapHist("PHYS", PHYS_TaggerE);
        MapHist("PHYS", PHYS_Scintillator);
    }
    MapHist("LMS", LMS_Led);
}

PRadDAQUnit::~PRadDAQUnit()
{
    for(auto &ele : histograms)
    {
        delete ele.second, ele.second = nullptr;
    }
}

void PRadDAQUnit::AddHist(const std::string &n)
{
    if(GetHist(n) == nullptr) {
        std::string hist_name = channelName + "_" + n;
        histograms[n] = new TH1I(hist_name.c_str(), n.c_str(), 2048, 0, 8191);
    } else {
        std::cout << "WARNING: " << channelName << ", hist " << n << " exists, skip adding it." << std::endl;
    }
}

void PRadDAQUnit::MapHist(const std::string &name, PRadTriggerType type)
{
    TH1 *hist_trg = GetHist(name);
    if(hist_trg == nullptr) {
        std::cout << "WARNING: Terminated mapping trigger type " << type
                  << " to histogram " << name << ", whichthat does not exist"
                  << std::endl;
        return;
    }

    size_t index = (size_t) type;
    hist[index] = hist_trg;
}

TH1 *PRadDAQUnit::GetHist(const std::string &n)
{
    auto it = histograms.find(n);
    if(it == histograms.end()) {
        return nullptr;
    }
    return it->second;
}

std::vector<TH1*> PRadDAQUnit::GetHistList()
{
    std::vector<TH1*> hlist;

    for(auto &ele : histograms)
    {
        hlist.push_back(ele.second);
    }

    return hlist;
}

void PRadDAQUnit::UpdatePedestal(const double &m, const double &s)
{
    pedestal = Pedestal(m, s);

    sparsify = (unsigned short)(pedestal.mean + 5*pedestal.sigma + 0.5); // round
}

// universe calibration code, can be implemented by derivative class
double PRadDAQUnit::Calibration(const unsigned short &adcVal)
{
    return (double) adcVal*cal_const.factor;
}

// erase current data
void PRadDAQUnit::CleanBuffer()
{
    occupancy = 0;
    ResetHistograms();
}

void PRadDAQUnit::ResetHistograms()
{
    for(auto &ele : histograms)
    {
        if(ele.second)
            ele.second->Reset();
    }
}

// zero suppression, triggered when adc value is statistically
// above pedestal (5 sigma)
unsigned short PRadDAQUnit::Sparsification(const unsigned short &adcVal)
{
    if(adcVal < sparsify)
        return 0;

    ++occupancy;
    return adcVal - sparsify;
}

double PRadDAQUnit::GetEnergy()
{
    return Calibration(Sparsification(adc_value));
}


