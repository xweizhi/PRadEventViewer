//============================================================================//
// TDC Group, it actually is also a channel as PRad DAQ Unit                  //
// TODO, merge this class to PRad DAQ Unit                                    //
// , or develop a base class for these two                                    //
//                                                                            //
// Chao Peng                                                                  //
// 05/17/2016                                                                 //
//============================================================================//

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
    ClearTimeMeasure();
}

void PRadTDCGroup::ClearTimeMeasure()
{
    timeMeasure.clear();
}

void PRadTDCGroup::AddTimeMeasure(const unsigned short &count)
{
    timeMeasure.push_back(count);
}

void PRadTDCGroup::AddTimeMeasure(const std::vector<unsigned short> &counts)
{
    timeMeasure.insert(timeMeasure.end(), counts.begin(), counts.end());
}

void PRadTDCGroup::UpdateTimeMeasure(const std::vector<unsigned short> &counts)
{
    timeMeasure = counts;
}
