#ifndef PRAD_GEM_SYSTEM_H
#define PRAD_GEM_SYSTEM_H

#include <string>
#include <vector>
#include <unordered_map>
#include "datastruct.h"
#include "PRadEventStruct.h"
#include "PRadException.h"


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

class PRadGEMDET;
class PRadGEMFEC;
class PRadGEMAPV;

class PRadGEMSystem
{
public:
    PRadGEMSystem();
    virtual ~PRadGEMSystem();

    void Clear();
    void LoadConfiguration(const std::string &path) throw(PRadException);
    void LoadPedestal(const std::string &path);
    void RegisterDET(PRadGEMDET *det);
    void RegisterFEC(PRadGEMFEC *fec);
    void RegisterAPV(PRadGEMAPV *apv);
    void BuildAPVMap();

    void SetUnivCommonModeThresLevel(const float &thres);
    void SetUnivZeroSupThresLevel(const float &thres);
    void SetUnivTimeSample(const size_t &thres);
    void ClearAPVData();

    PRadGEMDET *GetDetector(const int &id);
    PRadGEMDET *GetDetector(const std::string &name);
    PRadGEMDET *GetDetectorByPlaneX(const std::string &plane_x);
    PRadGEMDET *GetDetectorByPlaneY(const std::string &plane_y);
    PRadGEMFEC *GetFEC(const int &id);
    PRadGEMAPV *GetAPV(const int &fec, const int &adc);

    std::vector<GEM_Data> GetZeroSupData();

private:
    std::vector<PRadGEMDET*> det_list;
    std::unordered_map<std::string, PRadGEMDET*> det_map_name;
    std::unordered_map<std::string, PRadGEMDET*> det_map_plane_x;
    std::unordered_map<std::string, PRadGEMDET*> det_map_plane_y;
    std::unordered_map<int, PRadGEMFEC*> fec_map;
    std::unordered_map<GEMChannelAddress, PRadGEMAPV*> apv_map;
};

class PRadGEMDET
{
public:
    enum PlaneType
    {
        Plane_X,
        Plane_Y,
        MaxType,
    };

    struct Plane
    {
        std::string name;
        float size;
        int connector;
        int orientation;

        Plane()
        : name("Undefined"), size(0.), connector(-1), orientation(0)
        {};
        Plane(const std::string &n, const float &s, const int &c, const int &o)
        : name(n), size(s), connector(c), orientation(o)
        {};
    };

public:
    PRadGEMDET(const std::string &readoutBoard, const std::string &detectorType, const std::string &detector)
    : name(detector), type(detectorType), readout_board(readoutBoard)
    {};

    void AddPlane(const PlaneType &type, const Plane &plane) {planes[(size_t)type] = plane;};
    void AssignID(const int &i) {id = i;};
    Plane &GetPlaneX() {return planes[(size_t)Plane_X];};
    Plane &GetPlaneY() {return planes[(size_t)Plane_Y];};

    int id;
    std::string name;
    std::string type;
    std::string readout_board;
    Plane planes[MaxType];
};

class PRadGEMFEC
{
public:
    PRadGEMFEC(const int &i, const std::string &p)
    : id(i), ip(p)
    {};
    virtual ~PRadGEMFEC();

    void AddAPV(PRadGEMAPV *apv);
    void RemoveAPV(const int &id);
    void SortAPVList();
    PRadGEMAPV *GetAPV(const int &id);
    std::vector<PRadGEMAPV *> &GetAPVList() {return adc_list;};
    void Clear();
    void ClearAPVData();

    int id;
    std::string ip;
    std::unordered_map<int, PRadGEMAPV*> adc_map;
    std::vector<PRadGEMAPV *> adc_list;
};

class PRadGEMAPV
{
#define TIME_SAMPLE_SIZE 128
public:
    struct Pedestal
    {
        float offset;
        float noise;

        Pedestal() : offset(0), noise(0) {};
        Pedestal(const float &o, const float &n)
        : offset(o), noise(n)
        {};
    };

public:
    PRadGEMAPV(const std::string &plane,
               const int &fec_id,
               const int &adc_ch,
               const int &orientation,
               const int &index,
               const int &header_level,
               const std::string &status);
    virtual ~PRadGEMAPV();

    void ClearData();
    void ClearPedestal();
    void SetTimeSample(const size_t &t);
    void SetCommonModeThresLevel(const float &t) {common_thres = t;};
    void SetZeroSupThresLevel(const float &t) {zerosup_thres = t;};
    void FeedData(const uint32_t *buf, const size_t &size);
    void SplitData(const uint32_t &buf, int &word1, int &word2);
    void UpdatePedestalArray(Pedestal *ped, const size_t &size);
    void UpdatePedestal(const Pedestal &ped, const size_t &index);
    void ZeroSuppression(std::vector<GEM_Data> &hits, int *buf, const size_t &time_sample);
    void CommonModeCorrection(int *buf, const size_t &size);
    std::vector<GEM_Data> GetZeroSupHits();

    std::string plane;
    int fec_id;
    int adc_ch;
    size_t time_samples;
    int orient;
    int index;
    int header_level;

    float common_thres;
    float zerosup_thres;
    std::string status;
    Pedestal pedestal[TIME_SAMPLE_SIZE];
    size_t buffer_size;
    int *raw_data;
};

#endif
