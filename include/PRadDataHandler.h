#ifndef PRAD_DATA_HANDLER_H
#define PRAD_DATA_HANDLER_H

// Multi threads to deal with several banks in an event simultaneously.
// Depends on the running evironment, you may have greatly improved
// performance to read large evio file.
#define MULTI_THREAD

#ifdef MULTI_THREAD
#include <thread>
#include <mutex>
#endif

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <deque>
#include <vector>
#include "datastruct.h"

using namespace std;

class PRadEvioParser;
class PRadDAQUnit;
class PRadTDCGroup;
class TH1I;
class TH1D;

typedef struct ChannelData
{
    unsigned short channel_id;
    unsigned short value;
    ChannelData() : channel_id(0), value(0) {};
    ChannelData(const unsigned short &i, const unsigned short &v)
    : channel_id(i), value(v) {};       
} TDC_Data, ADC_Data;

struct EventData
{
    unsigned char type;
    unsigned char lms_phase;
    unsigned int time;
    vector< ADC_Data > adc_data;
    vector< TDC_Data > tdc_data;
    EventData() : type(0), lms_phase(0), time(0) {};
    EventData(const PRadTriggerType &t) : type((unsigned char)t), lms_phase(0), time(0) {};
    EventData(const PRadTriggerType &t, vector< ADC_Data > &adc, vector< TDC_Data > &tdc)
    : type((unsigned char)t), lms_phase(0), time(0), adc_data(adc), tdc_data(tdc) {};
    void clear() {type = 0; adc_data.clear(); tdc_data.clear();};
    void update_type(const unsigned char &t) {type = t;};
    void update_time(const unsigned int &t) {time = t;};
    void add_adc(const ADC_Data &a) {adc_data.push_back(a);};
    void add_tdc(const TDC_Data &t) {tdc_data.push_back(t);};
};

struct EPICSValue
{
    int att_event;
    float value;
    EPICSValue() : att_event(0), value(0) {};
    EPICSValue(const int &e, const float &v)
    : att_event(e), value(v) {};
    bool operator < (const int &e) const
    {
        return att_event < e;
    }
};

// a simple hash function for DAQ configuration
namespace std
{
    template <>
    struct hash<ChannelAddress>
    {
        size_t operator()(const ChannelAddress &cfg) const
        {
            // crate id is 1-6, slot is 1-26, channel is 0-63
            // thus they can be filled in a 16 bit word
            // [ 0 0 0 | 0 0 0 0 0 | 0 0 0 0 0 0 0 0 ]
            // [ crate |    slot   |     channel     ]
            // this simple hash ensures no collision at current setup
            // which means fast access
            return (cfg.crate << 13 | cfg.slot << 8 | cfg.channel);
        }
    };
}

typedef unordered_map< ChannelAddress, PRadDAQUnit* >::iterator daq_iter;
typedef unordered_map< string, PRadDAQUnit* >::iterator name_iter;
typedef unordered_map< string, PRadTDCGroup* >::iterator tdc_name_iter;
typedef unordered_map< ChannelAddress, PRadTDCGroup* >::iterator tdc_daq_iter;

class PRadDataHandler
{
public:
    PRadDataHandler();
    virtual ~PRadDataHandler();
    void AddChannel(PRadDAQUnit *channel);
    void AddTDCGroup(PRadTDCGroup *group);
    void RegisterChannel(PRadDAQUnit *channel);
    void Decode(const void *buffer);
    void FeedData(JLabTIData &tiData);
    void FeedData(ADC1881MData &adcData);
    void FeedData(GEMAPVData &gemData);
    void FeedData(TDCV767Data &tdcData);
    void FeedData(TDCV1190Data &tdcData);
    void UpdateEvent(int idx = 0);
    void UpdateEventNb(const unsigned int &n) {eventNb = n;};
    void UpdateTrgType(const unsigned char &trg);
    void UpdateLMSPhase(const unsigned char &ph);
    void UpdateEPICS(const string &name, const float &value);
    float FindEPICSValue(const string &name);
    float FindEPICSValue(const string &name, const int &event);
    void PrintOutEPICS(const int &event = 0);
    unsigned int GetEventCount() {return energyData.size();};
    int GetCurrentEventNb();
    TH1D *GetEnergyHist() {return energyHist;};
    void Clear();
    void EndofThisEvent();
    void OnlineMode() {onlineMode = true;};
    void OfflineMode() {onlineMode = false;};
    void BuildChannelMap();
    void SaveHistograms(const string &path);
    PRadDAQUnit *GetChannel(const ChannelAddress &daqInfo);
    PRadDAQUnit *GetChannel(const string &name);
    PRadDAQUnit *GetChannel(const unsigned short &id);
    PRadTDCGroup *GetTDCGroup(const string &name);
    PRadTDCGroup *GetTDCGroup(const ChannelAddress &addr);
    const unordered_map< string, PRadTDCGroup *> &GetTDCGroupSet() {return map_name_tdc;};
    vector< PRadDAQUnit* > GetChannelList() {return channelList;};

private:
    PRadEvioParser *parser;
    double totalE;
    unsigned int eventNb;
    bool onlineMode;
#ifdef MULTI_THREAD
    mutex myLock;
#endif
    unordered_map< ChannelAddress, PRadDAQUnit* > map_daq;
    unordered_map< string, PRadDAQUnit* > map_name;
    unordered_map< string, PRadTDCGroup* > map_name_tdc;
    unordered_map< ChannelAddress, PRadTDCGroup* > map_daq_tdc;
    map< string, vector<EPICSValue> > epics_channels; // order is important to iterate variables in this case
    vector< PRadDAQUnit* > channelList;
    vector< PRadDAQUnit* > freeList;
    vector< PRadTDCGroup* > tdcList;
    deque< EventData > energyData;
    EventData newEvent, lastEvent;
    TH1D *energyHist;
};

#endif
