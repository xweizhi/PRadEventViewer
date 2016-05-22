#include "PRadTDCGroup.h"
#include "PRadDAQUnit.h"
#include "TH1I.h"

PRadTDCGroup::PRadTDCGroup(const std::string &name, const ChannelAddress &addr, const int &id)
: groupName(name), address(addr), groupID(id)
{
    std::string tdc_name = "TDC_" + name;
    tdcHist = new TH1I(tdc_name.c_str(), (name+" Time Measure").c_str(), 20000, 0, 19999);
}

PRadTDCGroup::~PRadTDCGroup()
{
    delete tdcHist;
}

void PRadTDCGroup::AddChannel(PRadDAQUnit *ch)
{
    groupList.push_back(ch);
}

void PRadTDCGroup::CleanBuffer()
{
    tdcHist->Reset();
}
