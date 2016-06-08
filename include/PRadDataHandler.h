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
#include <map>
#include <deque>
#include <vector>
#include "datastruct.h"
#include "PRadException.h"

using namespace std;

class PRadEvioParser;
class PRadDAQUnit;
class PRadTDCGroup;
class TH1D;
class TH2I;

typedef struct ChannelData
{
    unsigned short channel_id;
    unsigned short value;

    ChannelData()
    : channel_id(0), value(0)
    {};
    ChannelData(const unsigned short &i, const unsigned short &v)
    : channel_id(i), value(v)
    {};

} TDC_Data, ADC_Data;

struct EventData
{
    int event_number;
    unsigned char type;
    unsigned char latch_word;
    unsigned char lms_phase;
    uint64_t timestamp;
    vector< ADC_Data > adc_data;
    vector< TDC_Data > tdc_data;

    EventData()
    : type(0), lms_phase(0), timestamp(0)
    {};
    EventData(const PRadTriggerType &t)
    : type((unsigned char)t), lms_phase(0), timestamp(0)
    {};
    EventData(const PRadTriggerType &t, vector< ADC_Data > &adc, vector< TDC_Data > &tdc)
    : type((unsigned char)t), lms_phase(0), timestamp(0), adc_data(adc), tdc_data(tdc)
    {};

    void clear() {type = 0; adc_data.clear(); tdc_data.clear();};
    void update_type(const unsigned char &t) {type = t;};
    void update_time(const uint64_t &t) {timestamp = t;};
    void add_adc(const ADC_Data &a) {adc_data.push_back(a);};
    void add_tdc(const TDC_Data &t) {tdc_data.push_back(t);};
};

struct EPICSValue
{
    int att_event;
    float value;

    EPICSValue()
    : att_event(0), value(0)
    {};
    EPICSValue(const int &e, const float &v)
    : att_event(e), value(v)
    {};

    bool operator < (const int &e) const
    {
        return att_event < e;
    }

};

struct ScalarChannel
{
    string name;
    unsigned int count;
    unsigned int gated_count;

    ScalarChannel() : name("undefined"), count(0), gated_count(0) {};
    ScalarChannel(const string &n) : name(n), count(0), gated_count(0) {};
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

class PRadDataHandler
{
public:
    PRadDataHandler();
    virtual ~PRadDataHandler();
    void AddChannel(PRadDAQUnit *channel);
    void AddTDCGroup(PRadTDCGroup *group);
    void RegisterChannel(PRadDAQUnit *channel);
    void ReadTDCList(const string &path);
    void ReadChannelList(const string &path);
    void ReadPedestalFile(const string &path);
    void ReadCalibrationFile(const string &path);
    void ReadGainFactor(const string &path);
    void Decode(const void *buffer);
    void FeedData(JLabTIData &tiData);
    void FeedData(ADC1881MData &adcData);
    void FeedData(GEMAPVData &gemData);
    void FeedData(TDCV767Data &tdcData);
    void FeedData(TDCV1190Data &tdcData);
    void FillTaggerHist(TDCV1190Data &tdcData);
    void ChooseEvent(int idx = 0);
    void UpdateTrgType(const unsigned char &trg);
    void UpdateScalarGroup(const unsigned int &size, const unsigned int *gated, const unsigned int *ungated);
    void UpdateEPICS(const string &name, const float &value);
    float FindEPICSValue(const string &name);
    float FindEPICSValue(const string &name, const int &event);
    void PrintOutEPICS();
    void PrintOutEPICS(const string &name);
    unsigned int GetEventCount() {return energyData.size();};
    unsigned int GetScalarCount(const unsigned int &group = 0, const bool &gated = false);
    vector<unsigned int> GetScalarsCount(const bool &gated = false);
    int GetCurrentEventNb();
    TH1D *GetEnergyHist() {return energyHist;};
    TH2I *GetTagEHist() {return TagEHist;};
    TH2I *GetTagTHist() {return TagTHist;};
    EventData &GetEventData(const unsigned int &index);
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
    void FitHistogram(const string &channel,
                      const string &hist_name,
                      const string &fit_func,
                      const double &range_min,
                      const double &range_max) throw(PRadException);
    void FitPedestal();
    void CorrectGainFactor(const string &reference = "LMS2");

private:
    PRadEvioParser *parser;
    double totalE;
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
    vector< ScalarChannel > triggerScalars;
    deque< EventData > energyData;
    EventData newEvent, lastEvent;
    TH1D *energyHist;
    TH2I *TagEHist;
    TH2I *TagTHist;
};

#endif
