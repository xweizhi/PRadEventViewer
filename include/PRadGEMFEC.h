#ifndef PRAD_GEM_FEC_H
#define PRAD_GEM_FEC_H

#include <string>
#include <vector>
#include <unordered_map>
#include "PRadEventStruct.h"

class PRadGEMAPV;

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
    void FitPedestal();
    void ClearAPVData();
    void ResetAPVHits();
    void CollectZeroSupHits(std::vector<GEM_Data> &hits);
    void Clear();

    // get parameters
    int GetID() {return id;};
    std::string GetIP() {return ip;};
    PRadGEMAPV *GetAPV(const int &id);
    std::vector<PRadGEMAPV *> &GetAPVList() {return adc_list;};

private:
    int id;
    std::string ip;
    std::unordered_map<int, PRadGEMAPV*> adc_map;
    std::vector<PRadGEMAPV *> adc_list;
};

#endif
