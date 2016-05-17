#ifndef PRAD_DATA_HANDLER_H
#define PRAD_DATA_HANDLER_H

// Multi threads to deal with several banks in an event simultaneously.
// Depends on the running evironment, you may have greatly improved
// performance to read large evio file.
//#define MULTI_THREAD

#ifdef MULTI_THREAD
#include <thread>
#include <mutex>
#endif

#include <unordered_map>
#include <deque>
#include <vector>
#include "datastruct.h"

using namespace std;

class PRadEvioParser;
class PRadDAQUnit;
class PRadTDCGroup;
class TH1I;
class TH1D;

// a simple hash function for DAQ configuration
namespace std {
    template <>
    struct hash<ChannelAddress>
    {
        size_t operator()(const ChannelAddress& cfg) const
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
    typedef struct ChannelData {
        unsigned short channel_id;
        unsigned short value;
        ChannelData() : channel_id(0), value(0) {};
        ChannelData(const unsigned short &i, const unsigned short &v)
        : channel_id(i), value(v) {};       
    } TDC_Data, ADC_Data;

    struct EventData {
        unsigned char type;
        unsigned char lms_phase;
        unsigned int time;
        vector< ADC_Data > adc_data;
        vector< TDC_Data > tdc_data;
        EventData() : type(0), lms_phase(0), time(0) {};
        EventData(const PRadTriggerType &t) : type((unsigned char)t), lms_phase(0), time(0) {};
        EventData(const PRadTriggerType &t, vector< ADC_Data > &adc, vector< TDC_Data > &tdc)
        : type((unsigned char)t), lms_phase(0), time(0), adc_data(adc), tdc_data(tdc){};
        void clear() {type = 0; adc_data.clear(); tdc_data.clear();};
        void update_type(const unsigned char &t) {type = t;};
        void update_time(const unsigned int &t) {time = t;};
        void add_adc(const ADC_Data &a) {adc_data.push_back(a);};
        void add_tdc(const TDC_Data &t) {tdc_data.push_back(t);};
    };

    PRadDataHandler();
    virtual ~PRadDataHandler();
    void AddChannel(PRadDAQUnit *channel);
    void AddTDCGroup(PRadTDCGroup *group);
    void RegisterChannel(PRadDAQUnit *channel);
    void FeedData(JLabTIData &tiData);
    void FeedData(ADC1881MData &adcData);
    void FeedData(GEMAPVData &gemData);
    void FeedData(TDCV767Data &tdcData);
    void FeedData(TDCV1190Data &tdcData);
    void FeedData(CAENHVData &hvData);
    void UpdateEvent(int idx = 0);
    void UpdateTrgType(const unsigned char &trg);
    void UpdateLMSPhase(const unsigned char &ph);
    int GetEventCount() {return energyData.size();};
    TH1D *GetEnergyHist() {return energyHist;};
    void Clear();
    void EndofThisEvent();
    void OnlineMode() {onlineMode = true;};
    void OfflineMode() {onlineMode = false;};
    void BuildChannelMap();
    PRadDAQUnit *FindChannel(const ChannelAddress &daqInfo);
    PRadDAQUnit *FindChannel(const string &name);
    PRadDAQUnit *FindChannel(const unsigned short &id);
    PRadTDCGroup *GetTDCGroup(const string &name);
    PRadTDCGroup *GetTDCGroup(const ChannelAddress &addr);
    const unordered_map< string, PRadTDCGroup *> &GetTDCGroupSet() {return map_name_tdc;};
    const vector< PRadDAQUnit* > &GetChannelList() {return channelList;};

private:
    double totalE;
    bool onlineMode;
#ifdef MULTI_THREAD
    mutex myLock;
#endif
    unordered_map< ChannelAddress, PRadDAQUnit* > map_daq;
    unordered_map< string, PRadDAQUnit* > map_name;
    unordered_map< string, PRadTDCGroup* > map_name_tdc;
    unordered_map< ChannelAddress, PRadTDCGroup* > map_daq_tdc;
    vector< PRadDAQUnit* > channelList;
    vector< PRadDAQUnit* > freeList;
    vector< PRadTDCGroup* > tdcList;
    deque< EventData > energyData;
    EventData newEvent, lastEvent;
    TH1D *energyHist;
};

#endif
