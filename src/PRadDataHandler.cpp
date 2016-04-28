//============================================================================//
// The data handler and container class                                       //
// Dealing with the data from all the channels                                //
// Provided multi-thread support, can be disabled by comment the definition   //
// in PRadDataHandler.h                                                       //
//                                                                            //
// Chao Peng                                                                  //
// 02/07/2016                                                                 //
//============================================================================//

#include <iostream>
#include "PRadDataHandler.h"
#include "PRadEvioParser.h"
#include "HyCalModule.h"
#include "HyCalClusters.h"
#include "TH1D.h"

PRadDataHandler::PRadDataHandler()
: totalE(0), onlineMode(false)
{
    // total energy histogram
    energyHist = new TH1D("HyCalEnergy", "Total Energy (MeV)", 2500, 0, 2500);
}

PRadDataHandler::~PRadDataHandler()
{
    delete energyHist;
}

// register HyCal modules
void PRadDataHandler::RegisterModule(HyCalModule* hyCalModule)
{
    hyCalModule->AssignID(moduleList.size());
    moduleList.push_back(hyCalModule);
}

void PRadDataHandler::BuildModuleMap()
{
    // build unordered maps separately improves its access speed
    // module name map
    for(auto &module : moduleList)
        map_name[module->GetReadID().toStdString()] = module;

    // module DAQ configuration map
    for(auto &module : moduleList)
        map_daq[module->GetDAQInfo()] = module;

    // module TDC groups
    for(auto &module : moduleList)
    {
        int id = module->GetTDCID();
        vector< HyCalModule* > tdcGroup = GetTDCGroup(id);
        tdcGroup.push_back(module);
        map_tdc[id] = tdcGroup;
    }
}

// erase the data container
void PRadDataHandler::Clear()
{
    // used memory won't be released, but it can be used again for new data file
    energyData.erase(energyData.begin(), energyData.end());
    totalE = 0;
    newEvent.clear();
    energyHist->Reset();
}

// feed ADC1881M data
void PRadDataHandler::FeedData(ADC1881MData &adcData)
{
    // find the module with this DAQ configuration
    daq_iter it = map_daq.find(adcData.config);

    // did not find it
    if(it == map_daq.end())
        return;

    // get the module pointer
    HyCalModule *module = it->second;

    // fill module adc value histogram
    module->adcHist->Fill(adcData.val);

    // zero suppression
    unsigned short sparVal = module->Sparsification(adcData.val);

    if(sparVal) // only store events above pedestal in memory
    {
        ModuleEnergyData word = { module->GetID(), sparVal }; // save id because it saves memory
#ifdef MULTI_THREAD
        // unfortunately, we have some non-local variable to deal with
        // so lock the thread to prevent concurrent access
        myLock.lock();
#endif
        totalE += module->Calibration(sparVal); // calculate total energy of this event
        newEvent.push_back(word); // store this data word
#ifdef MULTI_THREAD
        myLock.unlock();
#endif
    }

}

// feed GEM data
void PRadDataHandler::FeedData(GEMAPVData &)
{
    // implement later
}

// update High Voltage
void PRadDataHandler::FeedData(CAENHVData &hvData)
{
    name_iter it = map_name.find(hvData.name);

    if(it == map_name.end())
        return;

    HyCalModule *module = it->second;
    if(module->GetHVInfo() == hvData.config) {
        module->UpdateHV(hvData.Vmon, hvData.Vset, hvData.ON);
    } else {
        cerr << "ERROR: incorrect HV Configuration! "
             << "Module: " << hvData.name << endl;
        return;
    }

}

// signal of event end, save event or discard event in online mode
void PRadDataHandler::EndofThisEvent()
{
    if(onlineMode) { // online mode only saves the last event, to reduce usage of memory
        lastEvent = newEvent;
    } else {
        energyData.push_back(newEvent); // save event
    }

    energyHist->Fill(totalE); // fill energy histogram

    // clear buffer for next event
    newEvent.clear();
    totalE = 0;
}

// show the event to event viewer
void PRadDataHandler::ShowEvent(int idx)
{
    vector< ModuleEnergyData > event;

    // != avoids operator definition for non-standard map
    for(auto &module : moduleList)
    {
        module->DeEnergize();
    }

    if(onlineMode) { // online mode only show the last event
        event = lastEvent;
    } else { // offline mode, pick the event given by console
        if((unsigned int)idx >= energyData.size())
            return;
        event = energyData[idx];
    }

    for(auto &hit : event)
    {
        moduleList[hit.id]->Energize(hit.adcValue);
    }
}

// find modules
HyCalModule* PRadDataHandler::FindModule(ChannelAddress &daqInfo)
{
    daq_iter it = map_daq.find(daqInfo);
    if(it == map_daq.end())
        return nullptr;
    return it->second;
}

HyCalModule* PRadDataHandler::FindModule(const unsigned short &id)
{
    if(id >= moduleList.size())
        return nullptr;
    return moduleList[id];
}

vector< HyCalModule* > PRadDataHandler::GetTDCGroup(int &id)
{
    tdc_iter it = map_tdc.find(id);
    if(it == map_tdc.end())
        return vector< HyCalModule* >(); // return empty vector
    return it->second;
}

vector< int > PRadDataHandler::GetTDCGroupIDList()
{
    vector< int > list;
    for(tdc_iter it = map_tdc.begin(); it != map_tdc.end(); ++it)
    {
        list.push_back(it->first);
    }
    return list;
}
