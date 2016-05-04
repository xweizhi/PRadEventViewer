#ifndef PRAD_DATA_HANDLER_H
#define PRAD_DATA_HANDLER_H

#include <map>
#include <deque>
#include <vector>
#include "datastruct.h"

using namespace std;

class PRadEvioParser;
class HyCalModule;
class TH1D;
class TH1I;

typedef map< CrateConfig, HyCalModule* >::iterator daq_iter;
typedef map< string, HyCalModule* >::iterator name_iter;
typedef map< int, vector< HyCalModule* > >::iterator tdc_iter;

class PRadDataHandler
{
public:
    typedef struct {
        unsigned short id;
        unsigned short adcValue;
    } ModuleEnergyData;

    PRadDataHandler();
    virtual ~PRadDataHandler();
    void RegisterModule(HyCalModule *hyCalModule);
    void FeedData(ADC1881MData &adcData);
    void FeedData(GEMAPVData &gemData);
    void FeedData(CAENHVData &hvData);
    void ShowEvent(int idx = 0);
    int GetEventCount() {return energyData.size();};
    TH1D *GetEnergyHist() {return energyHist;};
    void Clear();
    void EndofThisEvent();
    void OnlineMode() {onlineMode = true;};
    void OfflineMode() {onlineMode = false;};
    void BuildModuleMap();
    HyCalModule *FindModule(CrateConfig &daqInfo);
    HyCalModule *FindModule(const unsigned short &id);
    vector< HyCalModule* > GetTDCGroup(int &id);
    vector< int > GetTDCGroupIDList();
    const vector< HyCalModule* > &GetModuleList() {return moduleList;};
    TH1I *GetLMSHist(int i = 0) {if(i < 0 || i >= 3) return NULL; else return lmsHist[i];};
    TH1I *GetDynodeSum() {return dynodeSum;};
    TH1I *GetAnodeSum() {return anodeSum;};

private:
    double totalE;
    int totalAnode;
    bool onlineMode;
    map< CrateConfig, HyCalModule* > map_daq;
    map< string, HyCalModule* > map_name;
    map< int, vector< HyCalModule* > > map_tdc;
    vector< HyCalModule* > moduleList;
    deque< vector< ModuleEnergyData > > energyData;
    vector < ModuleEnergyData > newEvent, lastEvent;
    TH1D *energyHist;
    TH1I *lmsHist[3];
    TH1I *dynodeSum;
    TH1I *anodeSum;
};

#endif
