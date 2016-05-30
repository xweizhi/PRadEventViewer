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
    PRadTDCGroup(const std::string &n, const ChannelAddress &addr, const int &id = -1);
    virtual ~PRadTDCGroup();
    void AddChannel(PRadDAQUnit *ch);
    void AssignID(const int &id) {groupID = id;};
    void CleanBuffer();
    size_t GetNbOfChs() {return groupList.size();};
    const std::vector<PRadDAQUnit *> &GetGroupList() {return groupList;};
    const std::string &GetName() {return groupName;};
    const ChannelAddress &GetAddress() {return address;};
    const int &GetID() {return groupID;};
    TH1I *GetHist() {return tdcHist;};

    bool operator < (const PRadTDCGroup &rhs) const
    {
        auto name_to_val = [](std::string name)
                           {
                               if(name.at(0) == 'W') return 1000;
                               if(name.at(0) == 'G') return 0;
                               else return (int)name.at(0)*10000;
                           };

        int lhs_val = name_to_val(groupName);
        int rhs_val = name_to_val(rhs.groupName);

        size_t idx = groupName.find_first_of("1234567890");
        if(idx != std::string::npos)
            lhs_val += std::stoi(groupName.substr(idx, -1));

        idx = rhs.groupName.find_first_of("1234567890");
        if(idx != std::string::npos)
            rhs_val += std::stoi(rhs.groupName.substr(idx, -1));

        return lhs_val < rhs_val;
    }

private:
    std::string groupName;
    TH1I *tdcHist;
    std::vector<PRadDAQUnit *> groupList;
    ChannelAddress address;
    int groupID;
};

#endif
