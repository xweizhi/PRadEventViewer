//============================================================================//
// GEM FEC class                                                              //
// FEC contains several APVs                                                  //
//                                                                            //
// Chao Peng                                                                  //
// 10/07/2016                                                                 //
//============================================================================//

#include "PRadGEMFEC.h"
#include <iostream>
#include <algorithm>

PRadGEMFEC::~PRadGEMFEC()
{
    Clear();
}

void PRadGEMFEC::AddAPV(PRadGEMAPV *apv)
{
    if(apv == nullptr)
        return;

    auto it = adc_map.find(apv->adc_ch);

    if(it != adc_map.end()) {
        std::cerr << "GEM FEC " << id
                  << ": Abort to add existing apv to adc channel "
                  << apv->adc_ch << std::endl;
        return;
    }

    adc_list.push_back(apv);
    adc_map[apv->adc_ch] = apv;
}

void PRadGEMFEC::RemoveAPV(const int &id)
{
    auto it = adc_map.find(id);

    if(it != adc_map.end()) {
        auto list_it = find(adc_list.begin(), adc_list.end(), it->second);
        if(list_it != adc_list.end())
            adc_list.erase(list_it);
        adc_map.erase(it);
    }
}

void PRadGEMFEC::SortAPVList()
{
    sort(adc_list.begin(), adc_list.end(), [this](PRadGEMAPV *apv1, PRadGEMAPV *apv2){return apv1->adc_ch < apv2->adc_ch;});
}

PRadGEMAPV *PRadGEMFEC::GetAPV(const int &id)
{
    auto it = adc_map.find(id);

    if(it == adc_map.end()) {
        std::cerr << "GEM FEC " << id
                  << ": Cannot find APV at adc channel " << id
                  << std::endl;
        return nullptr;
    }

    return it->second;
}

void PRadGEMFEC::Clear()
{
    for(auto &adc : adc_list)
    {
        delete adc, adc = nullptr;
    }

    adc_list.clear();
    adc_map.clear();
}

void PRadGEMFEC::ClearAPVData()
{
    for(auto &adc : adc_list)
    {
        adc->ClearData();
    }
}

void PRadGEMFEC::CollectZeroSupHits(std::vector<GEM_Data> &hits)
{
    for(auto &adc : adc_list)
    {
        adc->CollectZeroSupHits(hits);
    }
}

void PRadGEMFEC::FitPedestal()
{
    for(auto &adc : adc_list)
    {
        adc->FitPedestal();
    }
}

