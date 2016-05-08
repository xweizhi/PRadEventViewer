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
#include "PRadTDCGroup.h"
#include "TH1.h"

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

// register DAQ channels
void PRadDataHandler::RegisterChannel(PRadDAQUnit *channel)
{
    channel->AssignID(channelList.size());
    channelList.push_back(channel);
}

void PRadDataHandler::BuildChannelMap()
{
    // build unordered maps separately improves its access speed
    // name map
    for(auto &channel : channelList)
        map_name[channel->GetName()] = channel;

    // DAQ configuration map
    for(auto &channel : channelList)
        map_daq[channel->GetDAQInfo()] = channel;

    // TDC groups
    for(auto &channel : channelList)
    {
        string tdcName = channel->GetTDCName();
        if(tdcName.empty() || tdcName == "N/A")
            continue; // not belongs to any tdc group
        PRadTDCGroup *tdcGroup = GetTDCGroup(tdcName);
        if(tdcGroup == nullptr) {
            tdcGroup = new PRadTDCGroup(tdcName);
            map_tdc[tdcName] = tdcGroup;
        }
        tdcGroup->AddChannel(channel);
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
    // find the channel with this DAQ configuration
    daq_iter it = map_daq.find(adcData.config);

    // did not find any channel
    if(it == map_daq.end())
        return;

    // get the channel
    PRadDAQUnit *channel = it->second;

    // fill adc value histogram
    channel->adcHist->Fill(adcData.val);

    // zero suppression
    unsigned short sparVal = channel->Sparsification(adcData.val);

    if(sparVal) // only store events above pedestal in memory
    {
        ChannelData word = { channel->GetID(), sparVal }; // save id because it saves memory
#ifdef MULTI_THREAD
        // unfortunately, we have some non-local variable to deal with
        // so lock the thread to prevent concurrent access
        myLock.lock();
#endif
        totalE += channel->Calibration(sparVal); // calculate total energy of this event
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

    PRadDAQUnit *channel = it->second;
    HyCalModule *module = dynamic_cast<HyCalModule *>(channel);
    if(module == nullptr)
        return;

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
void PRadDataHandler::UpdateEvent(int idx)
{

    vector< ChannelData > event;

    // != avoids operator definition for non-standard map
    for(auto &channel : channelList)
    {
        channel->UpdateEnergy(0);
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
        channelList[hit.id]->UpdateEnergy(hit.adcValue);
    }

}

// find channels
PRadDAQUnit *PRadDataHandler::FindChannel(const ChannelAddress &daqInfo)
{
    daq_iter it = map_daq.find(daqInfo);
    if(it == map_daq.end())
        return nullptr;
    return it->second;
}

PRadDAQUnit *PRadDataHandler::FindChannel(const string &name)
{
    name_iter it = map_name.find(name);
    if(it == map_name.end())
        return nullptr;
    return it->second;
}

PRadDAQUnit *PRadDataHandler::FindChannel(const unsigned short &id)
{
    if(id >= channelList.size())
        return nullptr;
    return channelList[id];
}

PRadTDCGroup *PRadDataHandler::GetTDCGroup(string &name)
{
    tdc_iter it = map_tdc.find(name);
    if(it == map_tdc.end())
        return nullptr; // return empty vector
    return it->second;
}

