#ifndef PRAD_TDC_GROUP_H
#define PRAD_TDC_GROUP_H

#include <vector>
#include <string>
#include "datastruct.h"

class PRadDAQUnit;
class TH1I;

class PRadTDCGroup
{
public:
    PRadTDCGroup(const std::string &n, const ChannelAddress &addr);
    virtual ~PRadTDCGroup();
    void AddChannel(PRadDAQUnit *ch);
    size_t GetNbOfChs() {return groupList.size();};
    const std::vector<PRadDAQUnit *> &GetGroupList() {return groupList;};
    const std::string &GetName() {return groupName;};
    const ChannelAddress &GetAddress() {return address;};
    TH1I *GetHist() {return tdcHist;};

private:
    std::string groupName;
    TH1I *tdcHist;
    std::vector<PRadDAQUnit *> groupList;
    ChannelAddress address;
};

#endif
