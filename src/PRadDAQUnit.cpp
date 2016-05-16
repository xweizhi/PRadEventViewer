#include "PRadDAQUnit.h"

PRadDAQUnit::PRadDAQUnit(const std::string &name,
                         const ChannelAddress &daqAddr,
                         const std::string &tdc)
: address(daqAddr), pedestal(Pedestal(0, 0)), tdcGroup(tdc),
  occupancy(0), sparsify(0), channelID(0), energy(0)
{
    std::string hist_name;

    hist_name = name + "_ADC";
    adcHist = new TH1I(hist_name.c_str(), "ADC Value", 8192, 0, 8191);
    histograms["ADC"] = adcHist;

    hist_name = name + "_PED";
    pedHist = new TH1I(hist_name.c_str(), "Channel Pedestal", 8192, 0, 8191);
    histograms["PED"] = pedHist;

    hist_name = name + "_LMS";
    lmsHist = new TH1I(hist_name.c_str(), "LMS Events", 8192, 0, 8191);
    histograms["LMS"] = lmsHist;
    channelName = name;
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
        histograms[n] = new TH1I(hist_name.c_str(), n.c_str(), 8192, 0, 8191);
    }
}

TH1 *PRadDAQUnit::GetHist(const std::string &n)
{
    auto it = histograms.find(n);
    if(it == histograms.end())
        return nullptr;
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

