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
#include <algorithm>
#include "PRadDataHandler.h"
#include "PRadEvioParser.h"
#include "HyCalModule.h"
#include "PRadTDCGroup.h"
#include "TFile.h"
#include "TList.h"
#include "TH1.h"
#include "TH2.h"

//#define recon_test

#ifdef recon_test
#include <fstream>
#include "HyCalClusters.h"
#endif

static bool operator < (const int &e, const EPICSValue &v)
{
    return e < v.att_event;
}


PRadDataHandler::PRadDataHandler()
: parser(new PRadEvioParser(this)), totalE(0), onlineMode(false)
{
    // total energy histogram
    energyHist = new TH1D("HyCal Energy", "Total Energy (MeV)", 2500, 0, 2500);
    TagEHist = new TH2I("Tagger E", "Tagger E counter", 2000, 0, 20000, 384, 0, 383);
    TagTHist = new TH2I("Tagger T", "Tagger T counter", 2000, 0, 20000, 128, 0, 127);
}

PRadDataHandler::~PRadDataHandler()
{
    delete energyHist;
    delete TagEHist;
    delete TagTHist;

    for(auto &ele : freeList)
    {
        delete ele, ele = nullptr;
    }

    for(auto &tdc : tdcList)
    {
        delete tdc, tdc = nullptr;
    }

    delete parser;
}

// decode event buffer
void PRadDataHandler::Decode(const void *buffer)
{
    PRadEventHeader *header = (PRadEventHeader *)buffer;
    parser->parseEventByHeader(header);
}

// add DAQ channels
void PRadDataHandler::AddChannel(PRadDAQUnit *channel)
{
    RegisterChannel(channel);
    freeList.push_back(channel);
}

// register DAQ channels, memory is managed by other process
void PRadDataHandler::RegisterChannel(PRadDAQUnit *channel)
{
    channel->AssignID(channelList.size());
    channelList.push_back(channel);
}

void PRadDataHandler::AddTDCGroup(PRadTDCGroup *group)
{
    group->AssignID(tdcList.size());
    tdcList.push_back(group);

    map_name_tdc[group->GetName()] = group;
    map_daq_tdc[group->GetAddress()] = group;
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
        if(tdcName.empty() || tdcName == "N/A" || tdcName == "NONE")
            continue; // not belongs to any tdc group
        PRadTDCGroup *tdcGroup = GetTDCGroup(tdcName);
        if(tdcGroup == nullptr) {
            cerr << "Cannot find TDC group: " << tdcName
                 << " make sure you added all the tdc groups"
                 << endl;
            continue;
        }
        tdcGroup->AddChannel(channel);
    }
}

// erase the data container
void PRadDataHandler::Clear()
{
    // used memory won't be released, but it can be used again for new data file
    energyData.clear();
    totalE = 0;
    parser->eventNb = 0;
    newEvent.clear();
    energyHist->Reset();

    for(auto &channel : channelList)
    {
        channel->CleanBuffer();
    }

    for(auto &tdc : tdcList)
    {
        tdc->CleanBuffer();
    }
}

void PRadDataHandler::UpdateTrgType(const unsigned char &trg)
{
    if(newEvent.type && (newEvent.type != trg)) {
        cerr << "ERROR: Trigger type mismatch at event "
             << parser->eventNb
             << ", was " << (int) newEvent.type
             << " now " << (int) trg
             << endl;
    }
    newEvent.type = trg;
}

void PRadDataHandler::UpdateEPICS(const string &name, const float &value)
{
    auto it = epics_channels.find(name);

    if(it == epics_channels.end()) {
        vector<EPICSValue> epics_ch;
        epics_ch.push_back(EPICSValue((int)parser->eventNb, value));
        epics_channels[name] = epics_ch;
    } else {
        (*it).second.push_back(EPICSValue(parser->eventNb, value));
    }
}

float PRadDataHandler::FindEPICSValue(const string &name)
{
    auto it = epics_channels.find(name);
    if(it == epics_channels.end())
        return 0;

    vector<EPICSValue> &data = (*it).second;
    return data.back().value;
}

float PRadDataHandler::FindEPICSValue(const string &name, const int &event)
{
    auto it = epics_channels.find(name);
    if(it == epics_channels.end())
        return 0;

    vector<EPICSValue> &data = (*it).second;

    if(data.size() < 1)
        return 0;

    if(event < data.at(0))
        return data.at(0).value;

    auto idx = upper_bound(data.begin(), data.end(), event) - data.begin() - 1;

    return data.at(idx).value;
}

void PRadDataHandler::FeedData(JLabTIData &tiData)
{
    newEvent.lms_phase = tiData.lms_phase;
    newEvent.latch_word = tiData.latch_word;
    newEvent.timestamp = tiData.time_high;
    newEvent.timestamp <<= 32;
    newEvent.timestamp |= tiData.time_low;
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

    channel->FillHist(adcData.val, (PRadTriggerType)newEvent.type);

    unsigned short sparVal = channel->Sparsification(adcData.val, (newEvent.type != LMS_Led));

    if(sparVal) // only store events above pedestal in memory
    {
        ADC_Data word(channel->GetID(), sparVal); // save id because it saves memory
#ifdef MULTI_THREAD
        // unfortunately, we have some non-local variable to deal with
        // so lock the thread to prevent concurrent access
        myLock.lock();
#endif
        if(channel->GetType() == PRadDAQUnit::HyCalModule)
            totalE += channel->Calibration(sparVal); // calculate total energy of this event
        newEvent.add_adc(word); // store this data word
#ifdef MULTI_THREAD
        myLock.unlock();
#endif
    }

}

// feed GEM data
void PRadDataHandler::FeedData(GEMAPVData & /*gemData*/)
{
    // implement later
}

void PRadDataHandler::FeedData(TDCV767Data &tdcData)
{
    tdc_daq_iter it = map_daq_tdc.find(tdcData.config);
    if(it == map_daq_tdc.end())
        return;

    PRadTDCGroup *tdc = it->second;
    tdc->GetHist()->Fill(tdcData.val);

    newEvent.tdc_data.push_back(TDC_Data(tdc->GetID(), tdcData.val));
}

void PRadDataHandler::FeedData(TDCV1190Data &tdcData)
{
    if(tdcData.config.crate == PRadTS) {
        tdc_daq_iter it = map_daq_tdc.find(tdcData.config);
        if(it == map_daq_tdc.end()) {
            return;
        }

        PRadTDCGroup *tdc = it->second;
        tdc->GetHist()->Fill(tdcData.val);

        newEvent.add_tdc(TDC_Data(tdc->GetID(), tdcData.val));
    } else {
        FillTaggerHist(tdcData);
    }
}

void PRadDataHandler::FillTaggerHist(TDCV1190Data &tdcData)
{
    if(tdcData.config.slot == 3 || tdcData.config.slot == 5 || tdcData.config.slot == 7)
    {
        int e_ch = tdcData.config.channel + (tdcData.config.slot - 3)*64;
        TagEHist->Fill(tdcData.val, e_ch);
        newEvent.add_tdc(TDC_Data(e_ch+1001, tdcData.val));
    }
    if(tdcData.config.slot == 14)
    {
        int t_lr = tdcData.config.channel/64;
        int t_ch = tdcData.config.channel%64;
        if(t_ch > 31)
            t_ch = 32 + (t_ch + 16)%32;
        else
            t_ch = (t_ch + 16)%32;
        t_ch += t_lr*64;

        TagTHist->Fill(tdcData.val, t_ch);
        newEvent.add_tdc(TDC_Data(t_ch+2001, tdcData.val));
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

    if(newEvent.type == PHYS_TotalSum ||
       newEvent.type == PHYS_TaggerE  ||
       newEvent.type == PHYS_Scintillator
      )
        energyHist->Fill(totalE); // fill energy histogram

#ifdef recon_test
    ofstream outfile;
    outfile.open("HyCal_Hits.txt", ofstream::app);
    // reconstruct it
    HyCalClusters cluster;
    for(auto &adc : newEvent.adc_data)
    {
        HyCalModule *module = dynamic_cast<HyCalModule*>(GetChannel(adc.channel_id));
        if(module)
            cluster.AddModule(module->GetGeometry().x, module->GetGeometry().y, module->Calibration(adc.value));
    }
    vector<HyCalClusters::HyCal_Hits> hits = cluster.ReconstructHits();
    for(auto &hit : hits)
    {
        outfile << energyData.size() << "  " <<  hit.x << "  " << hit.y << "  "  << hit.E << endl;
    }
    outfile.close();
#endif

    // clear buffer for next event
    newEvent.clear();
    totalE = 0;
}

// show the event to event viewer
void PRadDataHandler::UpdateEvent(int idx)
{

    EventData event;

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

    for(auto &adc : event.adc_data)
    {
        channelList[adc.channel_id]->UpdateEnergy(adc.value);
    }

}

// find channels
PRadDAQUnit *PRadDataHandler::GetChannel(const ChannelAddress &daqInfo)
{
    daq_iter it = map_daq.find(daqInfo);
    if(it == map_daq.end())
        return nullptr;
    return it->second;
}

PRadDAQUnit *PRadDataHandler::GetChannel(const string &name)
{
    name_iter it = map_name.find(name);
    if(it == map_name.end())
        return nullptr;
    return it->second;
}

PRadDAQUnit *PRadDataHandler::GetChannel(const unsigned short &id)
{
    if(id >= channelList.size())
        return nullptr;
    return channelList[id];
}

PRadTDCGroup *PRadDataHandler::GetTDCGroup(const string &name)
{
    tdc_name_iter it = map_name_tdc.find(name);
    if(it == map_name_tdc.end())
        return nullptr; // return empty vector
    return it->second;
}

PRadTDCGroup *PRadDataHandler::GetTDCGroup(const ChannelAddress &addr)
{
    tdc_daq_iter it = map_daq_tdc.find(addr);
    if(it == map_daq_tdc.end())
        return nullptr;
    return it->second;
}

int PRadDataHandler::GetCurrentEventNb()
{
    return (int)parser->eventNb;
}

void PRadDataHandler::PrintOutEPICS(const int &event)
{
    if(event) {
        for(auto &epics_ch : epics_channels)
        {
            cout << epics_ch.first << ": "
                 << epics_ch.second.back().value
                 << endl;
        }
    }
}

void PRadDataHandler::SaveHistograms(const string &path)
{

    TFile *f = new TFile(path.c_str(), "recreate");

    energyHist->Write();

    // tdc histograms
    auto tList = tdcList;
    sort(tList.begin(), tList.end(), [](PRadTDCGroup *a, PRadTDCGroup *b) {return (*a) < (*b);} );

    TList thList;
    thList.Add(TagEHist);
    thList.Add(TagTHist);

    for(auto tdc : tList)
    {
        thList.Add(tdc->GetHist());
    }
    thList.Write("TDC Hists", TObject::kSingleKey);


    // adc histograms
    auto chList = channelList;
    sort(chList.begin(), chList.end(), [](PRadDAQUnit *a, PRadDAQUnit *b) {return (*a) < (*b);} );

    for(auto channel : chList)
    {
        TList hlist;
        std::vector<TH1*> hists = channel->GetHistList();
        for(auto hist : hists)
        {
            hlist.Add(hist);
        }
        hlist.Write(channel->GetName().c_str(), TObject::kSingleKey);
    }

    f->Close();
}

EventData &PRadDataHandler::GetEventData(const unsigned int &index)
{
    if(onlineMode) {
        return lastEvent;
    } else {
        if(index >= energyData.size()) {
            return energyData.back();
        } else {
            return energyData.at(index);
        }
    }
}
