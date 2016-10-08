#ifndef PRAD_GEM_APV_H
#define PRAD_GEM_APV_H

#include <vector>
#include <fstream>
#include "PRadGEMDetector.h"
#include "PRadEventStruct.h"
#include "datastruct.h"

class TH1I;

class PRadGEMAPV
{
#define TIME_SAMPLE_SIZE 128
#define TIME_SAMPLE_DIFF 140
#define SPLIT_SIZE 16
public:
    struct Pedestal
    {
        float offset;
        float noise;

        // initialize with large noise level so there will be no hits instead
        // of maximum hits when gem is not correctly initialized
        Pedestal() : offset(0.), noise(5000.)
        {};
        Pedestal(const float &o, const float &n)
        : offset(o), noise(n)
        {};
    };

    struct StripNb
    {
        unsigned char local;
        int plane;
    };

public:
    PRadGEMAPV(const int &fec_id,
               const int &adc_ch,
               const int &orientation,
               const int &plane_idx,
               const int &header_level,
               const std::string &status);
    virtual ~PRadGEMAPV();

    void SetDetectorPlane(PRadGEMDetector::Plane *p);
    void ClearData();
    void ClearPedestal();
    void CreatePedHist();
    void ReleasePedHist();
    void FillPedHist();
    void FitPedestal();
    void SetTimeSample(const size_t &t);
    void SetCommonModeThresLevel(const float &t) {common_thres = t;};
    void SetZeroSupThresLevel(const float &t) {zerosup_thres = t;};
    void FillRawData(const uint32_t *buf, const size_t &siz);
    void FillZeroSupData(const size_t &ch, const size_t &ts, const unsigned short &val);
    void SplitData(const uint32_t &buf, float &word1, float &word2);
    void UpdatePedestal(std::vector<Pedestal> &ped);
    void UpdatePedestal(const Pedestal &ped, const size_t &index);
    void UpdatePedestal(const float &offset, const float &noise, const size_t &index);
    void ZeroSuppression();
    void CommonModeCorrection(float *buf, const size_t &size);
    void CommonModeCorrection_Split(float *buf, const size_t &size);
    void CollectZeroSupHits(std::vector<GEM_Data> &hits);
    void BuildStripMap();
    void ResetHitPos();
    void PrintOutPedestal(std::ofstream &out);
    std::vector<TH1I *> GetHistList();
    std::vector<Pedestal> GetPedestalList();
    StripNb MapStrip(int ch);
    int GetLocalStripNb(const size_t &ch);
    int GetPlaneStripNb(const size_t &ch);
    void GetAverage(float &ave, const float *buf, const size_t &set = 0);
    size_t GetTimeSampleStart();
    size_t GetTimeSampleSize() {return TIME_SAMPLE_SIZE;};
    size_t GetNbOfTimeSamples() {return time_samples;};

public:
    PRadGEMDetector::Plane *plane;
    int fec_id;
    int adc_ch;
    size_t time_samples;
    int orientation;
    int plane_index;
    int header_level;

    bool split;

    float common_thres;
    float zerosup_thres;
    std::string status;
    Pedestal pedestal[TIME_SAMPLE_SIZE];
    StripNb strip_map[TIME_SAMPLE_SIZE];
    bool hit_pos[TIME_SAMPLE_SIZE];
    size_t buffer_size;
    size_t ts_index;
    float *raw_data;
    TH1I *offset_hist[TIME_SAMPLE_SIZE];
    TH1I *noise_hist[TIME_SAMPLE_SIZE];
};

#endif
