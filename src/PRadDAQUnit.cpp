#include "PRadDAQUnit.h"
#include "TH1I.h"

PRadDAQUnit::PRadDAQUnit(const char *name,
                         const ChannelAddress &daqAddr,
                         const std::string &tdc)
: address(daqAddr), pedestal(Pedestal(0, 0)), tdcGroup(tdc),
  occupancy(0), sparsify(0), channelID(0)
{
    adcHist = new TH1I(name, "ADC Value", 8192, 0, 8191);
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

