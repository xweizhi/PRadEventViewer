#ifndef PRAD_DATA_HANDLER_H
#define PRAD_DATA_HANDLER_H

#include <unordered_map>
#include <map>
#include <deque>
#include <fstream>
#include "datastruct.h"
#include "PRadEventStruct.h"
#include "PRadException.h"
#include "ConfigParser.h"

#ifdef MULTI_THREAD
#include <thread>
#include <mutex>
#endif

class PRadEvioParser;
class PRadDAQUnit;
class PRadTDCGroup;
class PRadGEMSystem;
class PRadGEMAPV;
class TH1D;
class TH2I;

// helper struct
struct epics_ch
{
    std::string name;
    size_t id;

    epics_ch(const std::string &n, const size_t &i)
    : name(n), id(i)
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
    void BuildChannelMap();

    // get channels/lists
    PRadGEMSystem *GetSRS() {return gem_srs;};
    PRadDAQUnit *GetChannel(const ChannelAddress &daqInfo);
    PRadDAQUnit *GetChannel(const std::string &name);
    PRadDAQUnit *GetChannel(const unsigned short &id);
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

    // dst data file
    void WriteToDST(const std::string &pat, std::ios::openmode mode = std::ios::out | std::ios::binary);
    void WriteToDST(std::ofstream &dst_file, const EventData &data) throw(PRadException);
    void WriteToDST(std::ofstream &dst_file, const EPICSData &data) throw(PRadException);
    void WriteEPICSMapToDST(std::ofstream &dst_file) throw(PRadException);
    void WriteRunInfoToDST(std::ofstream &dst_file) throw(PRadException);
    void WriteHyCalInfoToDST(std::ofstream &dst_file) throw(PRadException);
    void WriteGEMInfoToDST(std::ofstream &dst_file) throw(PRadException);
    void ReadFromDST(const std::string &path, std::ios::openmode mode = std::ios::in | std::ios::binary);
    void ReadFromDST(std::ifstream &dst_file, EventData &data) throw(PRadException);
    void ReadFromDST(std::ifstream &dst_file, EPICSData &data) throw(PRadException);
    void ReadEPICSMapFromDST(std::ifstream &dst_file) throw(PRadException);
    void ReadRunInfoFromDST(std::ifstream &dst_file) throw(PRadException);
    void ReadHyCalInfoFromDST(std::ifstream &dst_file) throw(PRadException);
    void ReadGEMInfoFromDST(std::ifstream &dst_file) throw(PRadException);

    // evio data file
    void ReadFromEvio(const std::string &path, const int &evt = -1, const bool &verbose = false);
    void ReadFromSplitEvio(const std::string &path, const int &split = -1, const bool &verbose = true);
    void Decode(const void *buffer);

    // data handler
    void Clear();
    void SetRunNumber(const int &run) {runInfo.run_number = run;};
    void StartofNewEvent(const unsigned char &tag);
    void EndofThisEvent(const unsigned int &ev = 0);
    void FeedData(JLabTIData &tiData);
    void FeedData(ADC1881MData &adcData);
    void FeedData(TDCV767Data &tdcData);
    void FeedData(TDCV1190Data &tdcData);
    void FeedData(GEMRawData &gemData);
    void FillTaggerHist(TDCV1190Data &tdcData);
    void UpdateEPICS(const std::string &name, const float &value);
    void UpdateTrgType(const unsigned char &trg);
    void UpdateScalarGroup(const unsigned int &size, const unsigned int *gated, const unsigned int *ungated);
    void AccumulateBeamCharge(const double &c);
    void UpdateLiveTimeScaler(const unsigned int &ungated, const unsigned int &gated);
 
    // show data
    int GetCurrentEventNb();
    void ChooseEvent(const int &idx = -1);
    void ChooseEvent(const EventData &event);
    unsigned int GetEventCount() {return energyData.size();};
    unsigned int GetEPICSEventCount() {return epicsData.size();};
    unsigned int GetScalarCount(const unsigned int &group = 0, const bool &gated = false);
    int GetRunNumber() {return runInfo.run_number;};
    double GetBeamCharge() {return runInfo.beam_charge;};
    double GetLiveTime() {return (1. - runInfo.dead_count/runInfo.ungated_count);};
    std::vector<unsigned int> GetScalarsCount(const bool &gated = false);
    TH1D *GetEnergyHist() {return energyHist;};
    TH2I *GetTagEHist() {return TagEHist;};
    TH2I *GetTagTHist() {return TagTHist;};
    EventData &GetEventData(const unsigned int &index);
    EventData &GetLastEvent();
    EPICSData &GetEPICSData(const unsigned int &index);
    double GetEnergy() {return totalE;};
    float GetEPICSValue(const std::string &name);
    float GetEPICSValue(const std::string &name, const int &index);
    void PrintOutEPICS();
    void PrintOutEPICS(const std::string &name);

    // analysis tools
    void InitializeByData(const std::string &path = "", int run = -1, int ref = 2);
    void ResetChannelHists();
    void SaveHistograms(const std::string &path);
    void FitHistogram(const std::string &channel,
                      const std::string &hist_name,
                      const std::string &fit_func,
                      const double &range_min,
                      const double &range_max) throw(PRadException);
    void FitPedestal();
    void ReadGainFactor(const std::string &path, const int &ref = 2);
    void CorrectGainFactor(const int &ref = 2);
    void RefillEnergyHist();
    int FindEventIndex(const int &event_number);

    // other functions
    void GetRunNumberFromFileName(const std::string &name, const size_t &pos = 0, const bool &verbose = true);
    std::vector<epics_ch> GetSortedEPICSList();
    void SaveEPICSChannels(const std::string &path);

private:
    PRadEvioParser *parser;
    PRadGEMSystem *gem_srs;
    RunInfo runInfo;
    double totalE;
    bool onlineMode;
    int current_event;
#ifdef MULTI_THREAD
    std::mutex myLock;
#endif
    std::unordered_map< ChannelAddress, PRadDAQUnit* > map_daq;
    std::unordered_map< std::string, PRadDAQUnit* > map_name;
    std::unordered_map< std::string, PRadTDCGroup* > map_name_tdc;
    std::unordered_map< ChannelAddress, PRadTDCGroup* > map_daq_tdc;

    // to save space, separate epics channels to map (string to vector index) and values (vector)
    std::unordered_map< std::string, size_t > epics_map;
    std::vector< float > epics_values;

    std::vector< PRadDAQUnit* > channelList;
    std::vector< PRadDAQUnit* > freeList; // channels that should be freed by handler
    std::vector< PRadTDCGroup* > tdcList;
    std::vector< ScalarChannel > triggerScalars;
    std::deque< EventData > energyData;
    std::deque< EPICSData > epicsData;
    EventData newEvent;
    TH1D *energyHist;
    TH2I *TagEHist;
    TH2I *TagTHist;
};

#endif
