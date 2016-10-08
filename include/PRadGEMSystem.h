#ifndef PRAD_GEM_SYSTEM_H
#define PRAD_GEM_SYSTEM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include "PRadEventStruct.h"
#include "PRadException.h"
#include "PRadGEMDetector.h"
#include "PRadGEMFEC.h"


#ifdef MULTI_THREAD
#include <thread>
#include <mutex>
#endif

class PRadGEMDetector;
class TH1I;

// a simple hash function for GEM DAQ configuration
namespace std
{
    template <>
    struct hash<GEMChannelAddress>
    {
        size_t operator()(const GEMChannelAddress &cfg) const
        {
            return (cfg.fec_id << 8 | cfg.adc_ch);
        }
    };
}

class PRadGEMSystem
{
public:
    PRadGEMSystem();
    virtual ~PRadGEMSystem();

    void Clear();
    void SortFECList();
    void LoadConfiguration(const std::string &path) throw(PRadException);
    void LoadPedestal(const std::string &path);
    void RegisterDET(PRadGEMDetector *det);
    void RegisterFEC(PRadGEMFEC *fec);
    void RegisterAPV(PRadGEMAPV *apv);
    void BuildAPVMap();
    void FillRawData(GEMRawData &raw, std::vector<GEM_Data> &container, const bool &fill_hist = false);

    void SetUnivCommonModeThresLevel(const float &thres);
    void SetUnivZeroSupThresLevel(const float &thres);
    void SetUnivTimeSample(const size_t &thres);
    void SetPedestalMode(const bool &m);
    void FitPedestal();
    void SavePedestal(const std::string &path);
    void SaveHistograms(const std::string &path);
    void ClearAPVData();

    PRadGEMDetector *GetDetector(const int &id);
    PRadGEMDetector *GetDetector(const std::string &name);
    PRadGEMDetector *GetDetectorByPlaneX(const std::string &plane_x);
    PRadGEMDetector *GetDetectorByPlaneY(const std::string &plane_y);
    PRadGEMFEC *GetFEC(const int &id);
    PRadGEMAPV *GetAPV(const GEMChannelAddress &addr);
    PRadGEMAPV *GetAPV(const int &fec, const int &adc);

    std::vector<GEM_Data> GetZeroSupData();
    std::vector<PRadGEMDetector*> &GetDETList() {return det_list;};
    std::vector<PRadGEMFEC*> &GetFECList() {return fec_list;};
    std::vector<PRadGEMAPV*> GetAPVList();

private:
    std::vector<PRadGEMDetector*> det_list;
    std::unordered_map<std::string, PRadGEMDetector*> det_map_name;
    std::unordered_map<std::string, PRadGEMDetector*> det_map_plane_x;
    std::unordered_map<std::string, PRadGEMDetector*> det_map_plane_y;
    std::vector<PRadGEMFEC*> fec_list;
    std::unordered_map<int, PRadGEMFEC*> fec_map;
    std::unordered_map<GEMChannelAddress, PRadGEMAPV*> apv_map;
    bool PedestalMode;

#ifdef MULTI_THREAD
    std::mutex locker;
#endif
};

#endif
