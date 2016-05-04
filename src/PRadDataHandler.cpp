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
#include "TH1.h"

PRadDataHandler::PRadDataHandler()
: totalE(0), totalAnode(0), onlineMode(false)
{
    // total energy histogram
    energyHist = new TH1D("HyCalEnergy", "Total Energy (MeV)", 2500, 0, 2500);
    anodeSum = new TH1I("HyCalAnodeSum", "Anode Sum", 8192, 0, 8191);
    dynodeSum = new TH1I("HyCalDynodeSum", "Dynode Sum", 8192, 0, 8191);
    lmsHist[0] = new TH1I("HyCalLMSPMT1", "LMS PMT 1", 8192, 0, 8191);
    lmsHist[1] = new TH1I("HyCalLMSPMT2", "LMS PMT 2", 8192, 0, 8191);
    lmsHist[2] = new TH1I("HyCalLMSPMT3", "LMS PMT 3", 8192, 0, 8191);
}

PRadDataHandler::~PRadDataHandler()
{
    delete energyHist;
    delete lmsHist[0];
    delete lmsHist[1];
    delete lmsHist[2];
    delete anodeSum;
    delete dynodeSum;
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
    for(size_t i = 0; i < moduleList.size(); ++i)
        map_name[moduleList[i]->GetReadID().toStdString()] = moduleList[i];

    // module DAQ configuration map
    for(size_t i = 0; i < moduleList.size(); ++i)
        map_daq[moduleList[i]->GetDAQInfo()] = moduleList[i];

    // module TDC groups
    for(size_t i = 0; i < moduleList.size(); ++i)
    {
        int id = moduleList[i]->GetTDCID();
        vector< HyCalModule* > tdcGroup = GetTDCGroup(id);
        tdcGroup.push_back(moduleList[i]);
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
    if(it == map_daq.end()) {
        if(adcData.config.crate == 6 && adcData.config.slot == 4) { // special channels
            switch(adcData.config.channel)
            {
                default: break;
                case 0: dynodeSum->Fill(adcData.val); break;
                case 1: lmsHist[0]->Fill(adcData.val); break;
                case 2: lmsHist[1]->Fill(adcData.val); break;
                case 3: lmsHist[2]->Fill(adcData.val); break;
            }
        }
        return;
    }

    // get the module pointer
    HyCalModule *module = it->second;

    // fill module adc value histogram
    module->adcHist->Fill(adcData.val);

    // zero suppression
    unsigned short sparVal = module->Sparsification(adcData.val);

    if(sparVal) // only store events above pedestal in memory
    {
        ModuleEnergyData word = { module->GetID(), sparVal }; // save id because it saves memory
        totalAnode += sparVal;
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
    anodeSum->Fill(totalAnode);

    // clear buffer for next event
    newEvent.clear();
    totalE = 0;
    totalAnode = 0;
}

// show the event to event viewer
void PRadDataHandler::ShowEvent(int idx)
{
    vector< ModuleEnergyData > event;

    // != avoids operator definition for non-standard map
    for(size_t i = 0; i < moduleList.size(); ++i)
    {
        moduleList[i]->DeEnergize();
    }

    if(onlineMode) { // online mode only show the last event
        event = lastEvent;
    } else { // offline mode, pick the event given by console
        if((unsigned int)idx >= energyData.size())
            return;
        event = energyData[idx];
    }

    for(size_t i = 0; i < event.size(); ++i)
    {
        moduleList[event[i].id]->Energize(event[i].adcValue);
    }
}

// find modules
HyCalModule* PRadDataHandler::FindModule(CrateConfig &daqInfo)
{
    daq_iter it = map_daq.find(daqInfo);
    if(it == map_daq.end())
        return NULL;
    return it->second;
}

HyCalModule* PRadDataHandler::FindModule(const unsigned short &id)
{
    if(id >= moduleList.size())
        return NULL;
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
