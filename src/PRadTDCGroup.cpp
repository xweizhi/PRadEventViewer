#include "PRadTDCGroup.h"
#include "PRadDAQUnit.h"
#include "TH1I.h"

PRadTDCGroup::PRadTDCGroup(const std::string &name, const ChannelAddress &addr)
: groupName(name), address(addr)
{
    std::string tdc_name = "TDC_" + name;
    tdcHist = new TH1I(tdc_name.c_str(), "TDC Value", 8192, 0, 8191);
}

PRadTDCGroup::~PRadTDCGroup()
{
    delete tdcHist;
}

void PRadTDCGroup::AddChannel(PRadDAQUnit *ch)
{
    groupList.push_back(ch);
}
