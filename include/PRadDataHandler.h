#ifndef PRAD_DATA_HANDLER_H
#define PRAD_DATA_HANDLER_H

#include <unordered_map>
#include <map>
#include <deque>
#include <fstream>
#include "PRadEventStruct.h"
#include "PRadException.h"
#include "ConfigParser.h"
#include <thread>

class PRadEvioParser;
class PRadHyCalCluster;
class PRadDSTParser;
class PRadDAQUnit;
class PRadTDCGroup;
class PRadGEMSystem;
class PRadGEMAPV;
class TH1D;
class TH2I;

// epics channel
struct epics_ch
{
    std::string name;
    uint32_t id;
    float value;

    epics_ch(const std::string &n, const uint32_t &i, const float &v)
    : name(n), id(i), value(v)
    {};
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

    // mode change
    void SetOnlineMode(const bool &mode);

    // add channels
    void AddChannel(PRadDAQUnit *channel);
    void AddTDCGroup(PRadTDCGroup *group);
    void RegisterChannel(PRadDAQUnit *channel);
    void RegisterEPICS(const std::string &name, const uint32_t &id, const float &value);
    void BuildChannelMap();

    // get channels/lists
    PRadGEMSystem *GetSRS() {return gem_srs;};
    PRadDAQUnit *GetChannel(const ChannelAddress &daqInfo);
    PRadDAQUnit *GetChannel(const std::string &name);
    PRadDAQUnit *GetChannel(const unsigned short &id);
    PRadDAQUnit *GetChannelPrimex(const unsigned short &id);
    PRadTDCGroup *GetTDCGroup(const std::string &name);
    PRadTDCGroup *GetTDCGroup(const ChannelAddress &addr);
    const std::unordered_map< std::string, PRadTDCGroup *> &GetTDCGroupSet() {return map_name_tdc;};
    std::vector< PRadDAQUnit* > &GetChannelList() {return channelList;};

    // read config files
    void ReadConfig(const std::string &path);
    template<typename... Args>
    void ExecuteConfigCommand(void (PRadDataHandler::*act)(Args...), Args&&... args);
    void ReadTDCList(const std::string &path);
    void ReadGEMConfiguration(const std::string &path);
    void ReadChannelList(const std::string &path);
    void ReadPedestalFile(const std::string &path);
    void ReadGEMPedestalFile(const std::string &path);
    void ReadCalibrationFile(const std::string &path);
    void ReadEPICSChannels(const std::string &path);

    // file reading and writing
    void ReadFromDST(const std::string &path, const uint32_t &mode = DST_UPDATE_ALL);
    void ReadFromEvio(const std::string &path, const int &evt = -1, const bool &verbose = false);
    void ReadFromSplitEvio(const std::string &path, const int &split = -1, const bool &verbose = true);
    void Decode(const void *buffer);
    void Replay(const std::string &r_path, const int &split = -1, const std::string &w_path = "");
    void WriteToDST(const std::string &path);

    // data handler
    void Clear();
    void SetRunNumber(const int &run) {runInfo.run_number = run;};
    void StartofNewEvent(const unsigned char &tag);
    void EndofThisEvent(const unsigned int &ev);
    void EndProcess(EventData *data);
    void WaitEventProcess();
    void FeedData(JLabTIData &tiData);
    void FeedData(JLabDSCData &dscData);
    void FeedData(ADC1881MData &adcData);
    void FeedData(TDCV767Data &tdcData);
    void FeedData(TDCV1190Data &tdcData);
    void FeedData(GEMRawData &gemData);
    void FeedData(std::vector<GEMZeroSupData> &gemData);
    void FeedTaggerHits(TDCV1190Data &tdcData);
    void FillHistograms(EventData &data);
    void UpdateEPICS(const std::string &name, const float &value);
    void UpdateTrgType(const unsigned char &trg);
    void AccumulateBeamCharge(EventData &event);
    void UpdateLiveTimeScaler(EventData &event);
    void UpdateOnlineInfo(EventData &event);
    void UpdateRunInfo(const RunInfo &ri) {runInfo = ri;};
    void AddHyCalClusterMethod(PRadHyCalCluster *r, const std::string &name, const std::string &c_path);
    void SetHyCalClusterMethod(const std::string &name);
    void ListHyCalClusterMethods();

    // show data
    int GetCurrentEventNb();
    void ChooseEvent(const int &idx = -1);
    void ChooseEvent(const EventData &event);
    unsigned int GetEventCount() {return energyData.size();};
    unsigned int GetEPICSEventCount() {return epicsData.size();};
    int GetRunNumber() {return runInfo.run_number;};
    double GetBeamCharge() {return runInfo.beam_charge;};
    double GetLiveTime() {return (1. - runInfo.dead_count/runInfo.ungated_count);};
    TH1D *GetEnergyHist() {return energyHist;};
    TH2I *GetTagEHist() {return TagEHist;};
    TH2I *GetTagTHist() {return TagTHist;};
    EventData &GetEvent(const unsigned int &index);
    EventData &GetLastEvent();
    std::deque<EventData> &GetEventData() {return energyData;};
    EPICSData &GetEPICSEvent(const unsigned int &index);
    std::deque<EPICSData> &GetEPICSData() {return epicsData;};
    RunInfo &GetRunInfo() {return runInfo;};
    OnlineInfo &GetOnlineInfo() {return onlineInfo;};
    double GetEnergy() {return totalE;};
    double GetEnergy(const EventData &event);
    float GetEPICSValue(const std::string &name);
    float GetEPICSValue(const std::string &name, const int &index);
    float GetEPICSValue(const std::string &name, const EventData &event);
    void PrintOutEPICS();
    void PrintOutEPICS(const std::string &name);

    // analysis tools
    void InitializeByData(const std::string &path = "", int run = -1, int ref = 2);
    void ResetChannelHists();
    void SaveHistograms(const std::string &path);
    std::vector<double> FitHistogram(const std::string &channel,
                                     const std::string &hist_name,
                                     const std::string &fit_func,
                                     const double &range_min,
                                     const double &range_max,
                                     const bool &verbose = false) throw(PRadException);
    void FitPedestal();
    void ReadGainFactor(const std::string &path, const int &ref = 2);
    void CorrectGainFactor(const int &ref = 2);
    void RefillEnergyHist();
    int FindEventIndex(const int &event_number);
    void HyCalReconstruct(const int &event_index);
    void HyCalReconstruct(EventData &event);
    HyCalHit *GetHyCalCluster(int &size);
    std::vector<HyCalHit> GetHyCalCluster();


    // other functions
    void GetRunNumberFromFileName(const std::string &name, const size_t &pos = 0, const bool &verbose = true);
    std::vector<epics_ch> GetSortedEPICSList();
    void SaveEPICSChannels(const std::string &path);

private:
    PRadEvioParser *parser;
    PRadDSTParser *dst_parser;
    PRadGEMSystem *gem_srs;
    PRadHyCalCluster *hycal_recon;
    RunInfo runInfo;
    OnlineInfo onlineInfo;
    double totalE;
    bool onlineMode;
    bool replayMode;
    int current_event;
    std::thread end_thread;

    // maps
    std::unordered_map< ChannelAddress, PRadDAQUnit* > map_daq;
    std::unordered_map< std::string, PRadDAQUnit* > map_name;
    std::unordered_map< std::string, PRadTDCGroup* > map_name_tdc;
    std::unordered_map< ChannelAddress, PRadTDCGroup* > map_daq_tdc;
    std::unordered_map< std::string, PRadHyCalCluster *> hycal_recon_map;

    std::vector< PRadDAQUnit* > channelList;
    std::vector< PRadDAQUnit* > freeList; // channels that should be freed by handler
    std::vector< PRadTDCGroup* > tdcList;

    // data related
    std::unordered_map< std::string, uint32_t > epics_map;
    std::vector< float > epics_values;
    std::deque< EventData > energyData;
    std::deque< EPICSData > epicsData;

    EventData *newEvent;
    TH1D *energyHist;
    TH2I *TagEHist;
    TH2I *TagTHist;
};

#endif
