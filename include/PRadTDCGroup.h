#ifndef PRAD_TDC_GROUP_H
#define PRAD_TDC_GROUP_H

#include <vector>
#include <string>

class PRadDAQUnit;
class TH1I;

class PRadTDCGroup
{
public:
    PRadTDCGroup(const std::string &n);
    virtual ~PRadTDCGroup();
    void AddChannel(PRadDAQUnit *ch);
    size_t GetNbOfChs() {return groupList.size();};
    const std::vector<PRadDAQUnit *> &GetGroupList() {return groupList;};
    const std::string &GetName() {return groupName;};
    TH1I *GetHist() {return tdcHist;};

private:
    std::string groupName;
    TH1I *tdcHist;
    std::vector<PRadDAQUnit *> groupList;
};

#endif
