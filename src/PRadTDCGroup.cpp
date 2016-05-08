#include "PRadTDCGroup.h"
#include "PRadDAQUnit.h"
#include "TH1I.h"

PRadTDCGroup::PRadTDCGroup(const std::string &name)
: groupName(name)
{
    tdcHist = new TH1I(name.c_str(), "TDC Value", 8192, 0, 8191);
}

PRadTDCGroup::~PRadTDCGroup()
{
    delete tdcHist;
}

void PRadTDCGroup::AddChannel(PRadDAQUnit *ch)
{
    groupList.push_back(ch);
}
