#include "PRadDAQUnit.h"
#include "TH1I.h"

PRadDAQUnit::PRadDAQUnit(const std::string &name,
                         const ChannelAddress &daqAddr,
                         const std::string &tdc)
: address(daqAddr), pedestal(Pedestal(0, 0)), tdcGroup(tdc),
  occupancy(0), sparsify(0), channelID(0), energy(0)
{
    adcHist = new TH1I(name.c_str(), "ADC Value", 8192, 0, 8191);
    channelName = name;
}

PRadDAQUnit::~PRadDAQUnit()
{
    delete adcHist;
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
    adcHist->Reset();
}

// zero suppression, triggered when adc value is statistically
// above pedestal (5 sigma)
unsigned short PRadDAQUnit::Sparsification(unsigned short &adcVal)
{
    if(adcVal < sparsify)
        return 0;
    else {
        ++occupancy;
        return adcVal - sparsify;
    }
}

