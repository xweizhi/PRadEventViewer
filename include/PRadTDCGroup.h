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
    void AddTimeMeasure(const unsigned short &count);
    void AddTimeMeasure(const std::vector<unsigned short> &counts);
    void UpdateTimeMeasure(const std::vector<unsigned short> &counts);
    void CleanBuffer();
    void ResetHistograms();
    void ClearTimeMeasure();
    void FillHist(const unsigned short &time);

    int GetID() const {return groupID;};
    std::string GetName() const {return groupName;};
    ChannelAddress GetAddress() const {return address;};
    size_t GetNbOfChs() const {return groupList.size();};
    TH1I *GetHist() const {return tdcHist;};
    const std::vector<PRadDAQUnit *> &GetGroupList() const {return groupList;};
    std::vector<unsigned short> &GetTimeMeasure() {return timeMeasure;};

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
    ChannelAddress address;
    std::vector<PRadDAQUnit *> groupList;
    std::vector<unsigned short> timeMeasure;
    TH1I *tdcHist;
    int groupID;
};

#endif
