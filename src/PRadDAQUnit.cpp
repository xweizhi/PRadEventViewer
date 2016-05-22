#include "PRadDAQUnit.h"
#include <iostream>
#include <utility>

PRadDAQUnit::PRadDAQUnit(const std::string &name,
                         const ChannelAddress &daqAddr,
                         const std::string &tdc)
: channelName(name), address(daqAddr), pedestal(Pedestal(0, 0)),
  tdcGroup(tdc), occupancy(0), sparsify(0), channelID(0), energy(0)
{
    std::string hist_name;

    // some default histograms
    AddHist("PHYS", "Integer", (channelName + " Physics Events").c_str(), 2048, 0, 8191);
    AddHist("PED", "Integer", (channelName + " Pedestal Events").c_str(), 2048, 0, 2047);
    AddHist("LMS", "Integer", (channelName + " LED Source").c_str(), 2048, 0, 8191);

    // default hist-trigger mapping
    MapHist("PED", TI_Internal);
    MapHist("PED", PULS_Pedestal);
    MapHist("PED", PULS_Clock);
    MapHist("PHYS", PHYS_TotalSum);
    MapHist("PHYS", PHYS_TaggerE);
    MapHist("LMS", LMS_Led);
    MapHist("PED", LMS_Alpha);

    // special case
    if(name.find("LMS") != std::string::npos) {
        GetHist("PHYS")->SetTitle((channelName + " Alpha Source").c_str());
        MapHist("PHYS", LMS_Alpha);
    }
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

template<typename... Args>
void PRadDAQUnit::AddHist(const std::string &n, const std::string &type, Args&&... args)
{
    if(GetHist(n) == nullptr) {
        std::string hist_name = channelName + "_" + n;
        switch(type.at(0))
        {
        case 'I':
            histograms[n] = new TH1I(hist_name.c_str(), std::forward<Args>(args)...);
            break;
        case 'F':
            histograms[n] = new TH1F(hist_name.c_str(), std::forward<Args>(args)...);
            break;
        case 'D':
            histograms[n] = new TH1D(hist_name.c_str(), std::forward<Args>(args)...);
            break;
        default:
            std::cout << "WARNING: " << channelName << ", does not support histogram type " << type << std::endl;
            break;
        }
    } else {
        std::cout << "WARNING: "<< channelName << ", hist " << n << " exists, skip adding it." << std::endl;
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

void PRadDAQUnit::UpdatePedestal(const double &m, const double &s)
{
    pedestal = Pedestal(m, s);
    sparsify = (unsigned short)(pedestal.mean + 5*pedestal.sigma + 0.5); // round
}

void PRadDAQUnit::UpdateEnergy(const unsigned short &adcVal)
{
    energy = Calibration(adcVal);
}

// universe calibration code, can be implemented by derivative class
double PRadDAQUnit::Calibration(const unsigned short & /*adcVal*/)
{
    return 0;
}

// erase current data
void PRadDAQUnit::CleanBuffer()
{
    occupancy = 0;
    for(auto &ele : histograms)
    {
        if(ele.second)
            ele.second->Reset();
    }
}

// zero suppression, triggered when adc value is statistically
// above pedestal (5 sigma)
unsigned short PRadDAQUnit::Sparsification(const unsigned short &adcVal, const bool &count)
{
    if(adcVal < sparsify)
        return 0;
    else {
        if(count)
            ++occupancy;
        return adcVal - sparsify;
    }
}

