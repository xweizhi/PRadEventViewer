//============================================================================//
// The data handler and container class                                       //
// Dealing with the data from all the channels                                //
//                                                                            //
// Chao Peng                                                                  //
// 02/07/2016                                                                 //
//============================================================================//

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include "PRadDataHandler.h"
#include "PRadEvioParser.h"
#include "PRadDSTParser.h"
#include "PRadReconstructor.h"
#include "PRadGEMSystem.h"
#include "PRadDAQUnit.h"
#include "PRadTDCGroup.h"
#include "ConfigParser.h"
#include "PRadBenchMark.h"
#include "TFile.h"
#include "TList.h"
#include "TF1.h"
#include "TH1.h"
#include "TH2.h"

#define EPICS_UNDEFINED_VALUE -9999.9
#define TAGGER_CHANID 30000 // Tagger tdc id will start from this number
#define TAGGER_T_CHANID 1000 // Start from TAGGER_CHANID, more than 1000 will be t channel

using namespace std;

PRadDataHandler::PRadDataHandler()
: parser(new PRadEvioParser(this)), dst_parser(new PRadDSTParser(this)),
  gem_srs(new PRadGEMSystem()), hycal_recon(new PRadReconstructor(this)),
  totalE(0), onlineMode(false), replayMode(false), current_event(0)
{
    // total energy histogram
    energyHist = new TH1D("HyCal Energy", "Total Energy (MeV)", 2500, 0, 2500);
    TagEHist = new TH2I("Tagger E", "Tagger E counter", 2000, 0, 20000, 384, 0, 383);
    TagTHist = new TH2I("Tagger T", "Tagger T counter", 2000, 0, 20000, 128, 0, 127);

    onlineInfo.add_trigger("Lead Glass Sum", 0);
    onlineInfo.add_trigger("Total Sum", 1);
    onlineInfo.add_trigger("LMS Led", 2);
    onlineInfo.add_trigger("LMS Alpha Source", 3);
    onlineInfo.add_trigger("Tagger Master OR", 4);
    onlineInfo.add_trigger("Scintillator", 5);
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
    delete dst_parser;
    delete gem_srs;
    delete hycal_recon;
}

void PRadDataHandler::ReadConfig(const string &path)
{
    ConfigParser c_parser;
    c_parser.SetSplitters(":,");

    if(!c_parser.OpenFile(path)) {
        cerr << "Data Handler: Cannot open configuration file "
             << "\"" << path << "\"."
             << endl;
    }

    while(c_parser.ParseLine())
    {
        string func_name = c_parser.TakeFirst();
        if((func_name.find("TDC List") != string::npos)) {
            const string var1 = c_parser.TakeFirst().String();
            ExecuteConfigCommand(&PRadDataHandler::ReadTDCList, var1);
        }
        if((func_name.find("Channel List") != string::npos)) {
            const string var1 = c_parser.TakeFirst().String();
            ExecuteConfigCommand(&PRadDataHandler::ReadChannelList, var1);
        }
        if((func_name.find("EPICS Channel") != string::npos)) {
            const string var1 = c_parser.TakeFirst().String();
            ExecuteConfigCommand(&PRadDataHandler::ReadEPICSChannels, var1);
        }
        if((func_name.find("HyCal Pedestal") != string::npos)) {
            const string var1 = c_parser.TakeFirst().String();
            ExecuteConfigCommand(&PRadDataHandler::ReadPedestalFile, var1);
        }
        if((func_name.find("HyCal Calibration") != string::npos)) {
            const string var1 = c_parser.TakeFirst().String();
            ExecuteConfigCommand(&PRadDataHandler::ReadCalibrationFile, var1);
        }
        if((func_name.find("GEM Configuration") != string::npos)) {
            const string var1 = c_parser.TakeFirst().String();
            ExecuteConfigCommand(&PRadDataHandler::ReadGEMConfiguration, var1);
        }
        if((func_name.find("GEM Pedestal") != string::npos)) {
            const string var1 = c_parser.TakeFirst().String();
            ExecuteConfigCommand(&PRadDataHandler::ReadGEMPedestalFile, var1);
        }
        if((func_name.find("Run Number") != string::npos)) {
            const int var1 = c_parser.TakeFirst().Int();
            ExecuteConfigCommand(&PRadDataHandler::SetRunNumber, var1);
        }
        if((func_name.find("Initialize File") != string::npos)) {
            const string var1 = c_parser.TakeFirst().String();
            ExecuteConfigCommand(&PRadDataHandler::InitializeByData, var1, -1, 2);
        }
    }
}

// execute command
template<typename... Args>
void PRadDataHandler::ExecuteConfigCommand(void (PRadDataHandler::*act)(Args...), Args&&... args)
{
    (this->*act)(std::forward<Args>(args)...);
}

// decode event buffer
void PRadDataHandler::Decode(const void *buffer)
{
    PRadEventHeader *header = (PRadEventHeader *)buffer;
    parser->ParseEventByHeader(header);

    WaitEventProcess();
}

void PRadDataHandler::SetOnlineMode(const bool &mode)
{
    onlineMode = mode;
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

void PRadDataHandler::RegisterEPICS(const string &name, const uint32_t &id, const float &value)
{
    if(id >= (uint32_t)epics_values.size())
    {
        epics_values.resize(id, EPICS_UNDEFINED_VALUE);
    }

    epics_map[name] = id;
    epics_values.at(id) = value;
}

void PRadDataHandler::AddTDCGroup(PRadTDCGroup *group)
{
    if(GetTDCGroup(group->GetName()) || GetTDCGroup(group->GetAddress())) {
        cout << "WARNING: Atempt to add existing TDC group "
             << group->GetName()
             << "." << endl;
        return;
    }

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
//________________________________________________________________________________
void PRadDataHandler::UpdateHyCalGain(const std::string path)
{
    ConfigParser c_parser;

    if(!c_parser.OpenFile(path)) {
        cerr << "Data Handler: Cannot open gain file "
             << "\"" << path << "\"."
             << endl;
    }
  
    while(c_parser.ParseLine()){
      if (c_parser.NbofElements() > 0){
        int id = c_parser.TakeFirst().Int();
        if (id == 0) continue;
        string channelName;
        if (id < 1000) { channelName = "G"; }
        else {channelName = "W"; id -= 1000; }

        channelName += to_string(id);
        PRadDAQUnit *thisUnit = GetChannel(channelName);
        if (thisUnit == nullptr || !thisUnit->IsHyCalModule()) continue;

        double p0 = c_parser.TakeFirst().Double();
        double p1 = c_parser.TakeFirst().Double()*1.e-3;
        thisUnit->SetGainLinearity(p0, p1);
        c_parser.TakeFirst(); 
      }
    }
}

//________________________________________________________________________________
// erase the data container
void PRadDataHandler::Clear()
{
    // used memory won't be released, but it can be used again for new data file
    energyData = deque<EventData>();
    epicsData = deque<EPICSData>();
    runInfo.clear();

    parser->SetEventNumber(0);
    totalE = 0;

    energyHist->Reset();
    TagEHist->Reset();
    TagTHist->Reset();
    for(auto &channel : channelList)
    {
        channel->CleanBuffer();
    }

    for(auto &tdc : tdcList)
    {
        tdc->CleanBuffer();
    }
}

void PRadDataHandler::ResetChannelHists()
{
    for(auto &channel : channelList)
    {
        channel->ResetHistograms();
    }

    for(auto &tdc : tdcList)
    {
        tdc->ResetHistograms();
    }
}

void PRadDataHandler::UpdateTrgType(const unsigned char &trg)
{
    if(newEvent->trigger && (newEvent->trigger != trg)) {
        cerr << "ERROR: Trigger type mismatch at event "
             << parser->GetEventNumber()
             << ", was " << (int) newEvent->trigger
             << " now " << (int) trg
             << endl;
    }
    newEvent->trigger = trg;
}

void PRadDataHandler::UpdateEPICS(const string &name, const float &value)
{
    auto it = epics_map.find(name);

    if(it == epics_map.end()) {
        cout << "Data Handler:: Received data from unregistered EPICS channel " << name
             << ". Assign a new channel id " << epics_values.size()
             << "." << endl;
        epics_map[name] = epics_values.size();
        epics_values.push_back(value);
    } else {
        epics_values.at(it->second) = value;
    }
}

void PRadDataHandler::AccumulateBeamCharge(EventData &event)
{
    if(event.is_physics_event())
        runInfo.beam_charge += event.get_beam_charge();
}

void PRadDataHandler::UpdateLiveTimeScaler(EventData &event)
{
    if(event.is_physics_event()) {
        runInfo.ungated_count += event.get_ref_channel().ungated_count;
        runInfo.dead_count += event.get_ref_channel().gated_count;
    }
}

void PRadDataHandler::UpdateOnlineInfo(EventData &event)
{
    // update triggers
    for(auto trg_ch : onlineInfo.trigger_info)
    {
        if(trg_ch.id < event.dsc_data.size())
        {
            // get ungated trigger counts
            unsigned int counts = event.get_dsc_channel(trg_ch.id).ungated_count;

            // calculate the frequency
            trg_ch.freq = (double)counts / event.get_beam_time();
        }

        else {
            cerr << "Data Handler: Unmatched discriminator data from event "
                 << event.event_number
                 << ", expect trigger " << trg_ch.name
                 << " at channel " << trg_ch.id
                 << ", but the event only has " << event.dsc_data.size()
                 << " dsc channels." << endl;
        }
    }

    // update live time
    onlineInfo.live_time = event.get_live_time();

    //update beam current
    onlineInfo.beam_current = event.get_beam_current();
}

float PRadDataHandler::GetEPICSValue(const string &name)
{
    auto it = epics_map.find(name);
    if(it == epics_map.end()) {
        cerr << "Data Handler: Did not find EPICS channel " << name << endl;
        return EPICS_UNDEFINED_VALUE;
    }

    return epics_values.at(it->second);
}

float PRadDataHandler::GetEPICSValue(const string &name, const int &index)
{
    if((unsigned int)index >= energyData.size())
        return GetEPICSValue(name);

    return GetEPICSValue(name, energyData.at(index));
}

float PRadDataHandler::GetEPICSValue(const string &name, const EventData &event)
{
    float result = EPICS_UNDEFINED_VALUE;

    auto it = epics_map.find(name);
    if(it == epics_map.end()) {
        cerr << "Data Handler: Did not find EPICS channel " << name << endl;
        return result;
    }

    uint32_t channel_id = it->second;
    int event_number = event.event_number;

    // find the epics event before this event
    for(size_t i = 0; i < epicsData.size(); ++i)
    {
        if(epicsData.at(i).event_number >= event_number) {
            if(i > 0) result = epicsData.at(i-1).values.at(channel_id);
            break;
        }
    }

    return result;
}

void PRadDataHandler::FeedData(JLabTIData &tiData)
{
    newEvent->timestamp = tiData.time_high;
    newEvent->timestamp <<= 32;
    newEvent->timestamp |= tiData.time_low;
}

void PRadDataHandler::FeedData(JLabDSCData &dscData)
{
    for(uint32_t i = 0; i < dscData.size; ++i)
    {
        newEvent->dsc_data.emplace_back(dscData.gated_buf[i], dscData.ungated_buf[i]);
    }
}

// feed ADC1881M data
void PRadDataHandler::FeedData(ADC1881MData &adcData)
{
    // find the channel with this DAQ configuration
    auto it = map_daq.find(adcData.config);

    // did not find any channel
    if(it == map_daq.end())
        return;

    // get the channel
    PRadDAQUnit *channel = it->second;

    if(newEvent->is_physics_event()) {
        if(channel->Sparsification(adcData.val)) {
            newEvent->add_adc(ADC_Data(channel->GetID(), adcData.val)); // store this data word
        }
    } else if (newEvent->is_monitor_event()) {
        newEvent->add_adc(ADC_Data(channel->GetID(), adcData.val));
    }

}

void PRadDataHandler::FeedData(TDCV767Data &tdcData)
{
    auto it = map_daq_tdc.find(tdcData.config);
    if(it == map_daq_tdc.end())
        return;

    PRadTDCGroup *tdc = it->second;

    newEvent->tdc_data.push_back(TDC_Data(tdc->GetID(), tdcData.val));
}

void PRadDataHandler::FeedData(TDCV1190Data &tdcData)
{
    if(tdcData.config.crate == PRadTS) {
        auto it = map_daq_tdc.find(tdcData.config);
        if(it == map_daq_tdc.end()) {
            return;
        }

        PRadTDCGroup *tdc = it->second;
        newEvent->add_tdc(TDC_Data(tdc->GetID(), tdcData.val));
    } else {
        FeedTaggerHits(tdcData);
    }
}

void PRadDataHandler::FeedTaggerHits(TDCV1190Data &tdcData)
{
    if(tdcData.config.slot == 3 || tdcData.config.slot == 5 || tdcData.config.slot == 7)
    {
        int e_ch = tdcData.config.channel + (tdcData.config.slot - 3)*64 + TAGGER_CHANID;
        // E Channel 30000 + channel
        newEvent->add_tdc(TDC_Data(e_ch, tdcData.val));
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
        newEvent->add_tdc(TDC_Data(t_ch + TAGGER_CHANID + TAGGER_T_CHANID, tdcData.val));
    }
}

// feed GEM data
void PRadDataHandler::FeedData(GEMRawData &gemData)
{
    gem_srs->FillRawData(gemData, newEvent->gem_data, newEvent->is_monitor_event());
}

void PRadDataHandler::FillHistograms(EventData &data)
{
    double energy = 0.;

    // for all types of events
    for(auto &adc : data.adc_data)
    {
        if(adc.channel_id >= channelList.size())
            continue;

        channelList[adc.channel_id]->FillHist(adc.value, data.trigger);
        energy += channelList[adc.channel_id]->GetEnergy(adc.value);
    }

    if(!data.is_physics_event())
        return;

    // for only physics events
    energyHist->Fill(energy);

    for(auto &tdc : data.tdc_data)
    {
        if(tdc.channel_id < tdcList.size()) {
            tdcList[tdc.channel_id]->FillHist(tdc.value);
        } else if(tdc.channel_id >= TAGGER_CHANID) {
            int id = tdc.channel_id - TAGGER_CHANID;
            if(id >= TAGGER_T_CHANID)
                TagTHist->Fill(tdc.value, id - TAGGER_T_CHANID);
            else
                TagEHist->Fill(tdc.value, id - TAGGER_CHANID);
        }
    }
}

// signal of new event
void PRadDataHandler::StartofNewEvent(const unsigned char &tag)
{
    newEvent = new EventData(tag);
}

// signal of event end, save event or discard event in online mode
void PRadDataHandler::EndofThisEvent(const unsigned int &ev)
{
    newEvent->event_number = ev;
    // wait for the process thread
    WaitEventProcess();

    end_thread = thread(&PRadDataHandler::EndProcess, this, newEvent);
}

void PRadDataHandler::WaitEventProcess()
{
    if(end_thread.joinable())
        end_thread.join();
}

void PRadDataHandler::EndProcess(EventData *data)
{
    if(data->type == EPICS_Info) {

        if(onlineMode && epicsData.size())
            epicsData.pop_front();

        if(replayMode)
            dst_parser->WriteEPICS(EPICSData(data->event_number, epics_values));
        else
            epicsData.emplace_back(data->event_number, epics_values);

    } else { // event or sync event

        FillHistograms(*data);

        if(data->type == CODA_Sync) {
            AccumulateBeamCharge(*data);
            UpdateLiveTimeScaler(*data);
            if(onlineMode)
                UpdateOnlineInfo(*data);
        }

        if(onlineMode && energyData.size()) // online mode only saves the last event, to reduce usage of memory
            energyData.pop_front();

        if(replayMode)
            dst_parser->WriteEvent(*data);
        else
            energyData.emplace_back(move(*data)); // save event

    }

    delete data; // new data memory is released here
}

// show the event to event viewer
void PRadDataHandler::ChooseEvent(const int &idx)
{
    if (energyData.size()) { // offline mode, pick the event given by console
        if((unsigned int) idx >= energyData.size())
            ChooseEvent(energyData.back());
        else
            ChooseEvent(energyData.at(idx));
    } else if(!onlineMode) {
        cout << "Data Handler: Data bank is empty, no event is chosen." << endl;
        return;
    }
}

void PRadDataHandler::ChooseEvent(const EventData &event)
{
    totalE = 0;
    // != avoids operator definition for non-standard map
    for(auto &channel : channelList)
    {
        channel->UpdateADC(0);
    }

    for(auto &tdc_ch : tdcList)
    {
        tdc_ch->ClearTimeMeasure();
    }

    for(auto &adc : event.adc_data)
    {
        if(adc.channel_id >= channelList.size())
            continue;

        channelList[adc.channel_id]->UpdateADC(adc.value);
        totalE += channelList[adc.channel_id]->GetEnergy();
    }

    for(auto &tdc : event.tdc_data)
    {
        if(tdc.channel_id >= tdcList.size())
            continue;

        tdcList[tdc.channel_id]->AddTimeMeasure(tdc.value);
    }

    current_event = event.event_number;
}

double PRadDataHandler::GetEnergy(const EventData &event)
{
    double energy = 0.;
    for(auto &adc : event.adc_data)
    {
        if(adc.channel_id >= channelList.size())
            continue;
        energy += channelList[adc.channel_id]->GetEnergy(adc.value);
    }

    return energy;
}

// find channels
PRadDAQUnit *PRadDataHandler::GetChannel(const ChannelAddress &daqInfo)
{
    auto it = map_daq.find(daqInfo);
    if(it == map_daq.end())
        return nullptr;
    return it->second;
}

PRadDAQUnit *PRadDataHandler::GetChannel(const string &name)
{
    auto it = map_name.find(name);
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
    auto it = map_name_tdc.find(name);
    if(it == map_name_tdc.end())
        return nullptr; // return empty vector
    return it->second;
}

PRadTDCGroup *PRadDataHandler::GetTDCGroup(const ChannelAddress &addr)
{
    auto it = map_daq_tdc.find(addr);
    if(it == map_daq_tdc.end())
        return nullptr;
    return it->second;
}

int PRadDataHandler::GetCurrentEventNb()
{
    return current_event;
}

vector<epics_ch> PRadDataHandler::GetSortedEPICSList()
{
    vector<epics_ch> epics_list;

    for(auto &ch : epics_map)
    {
        float value = epics_values.at(ch.second);
        epics_list.emplace_back(ch.first, ch.second, value);
    }

    sort(epics_list.begin(), epics_list.end(), [](const epics_ch &a, const epics_ch &b) {return a.id < b.id;});

    return epics_list;
}

void PRadDataHandler::PrintOutEPICS()
{
    vector<epics_ch> epics_list = GetSortedEPICSList();

    for(auto &ch : epics_list)
    {
        cout << ch.name << ": " << epics_values.at(ch.id) << endl;
    }
}

void PRadDataHandler::PrintOutEPICS(const string &name)
{
    auto it = epics_map.find(name);
    if(it == epics_map.end()) {
        cout << "Did not find the EPICS channel "
             << name << endl;
        return;
    }

    cout << name << ": " << epics_values.at(it->second) << endl;
}

void PRadDataHandler::SaveEPICSChannels(const string &path)
{
    ofstream out(path);

    if(!out.is_open()) {
        cerr << "Cannot open file "
             << "\"" << path << "\""
             << " to save EPICS channels!"
             << endl;
        return;
    }

    vector<epics_ch> epics_list = GetSortedEPICSList();

    for(auto &ch : epics_list)
    {
        out << ch.name << endl;
    }

    out.close();
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
        vector<TH1*> hists = channel->GetHistList();
        for(auto hist : hists)
        {
            hlist.Add(hist);
        }
        hlist.Write(channel->GetName().c_str(), TObject::kSingleKey);
    }

    f->Close();
    delete f;
}

EventData &PRadDataHandler::GetEvent(const unsigned int &index)
{
    if(index >= energyData.size()) {
        return energyData.back();
    } else {
        return energyData.at(index);
    }
}

EPICSData &PRadDataHandler::GetEPICSEvent(const unsigned int &index)
{
    if(index >= epicsData.size()) {
        return epicsData.back();
    } else {
        return epicsData.at(index);
    }
}

void PRadDataHandler::FitHistogram(const string &channel,
                                   const string &hist_name,
                                   const string &fit_function,
                                   const double &range_min,
                                   const double &range_max) throw(PRadException)
{
    // If the user didn't dismiss the dialog, do something with the fields
    PRadDAQUnit *ch = GetChannel(channel);
    if(ch == nullptr) {
        throw PRadException("Fit Histogram Failure", "Channel " + channel + " does not exist!");
    }

    TH1 *hist = ch->GetHist(hist_name);
    if(hist == nullptr) {
        throw PRadException("Fit Histogram Failure", "Histogram " + hist_name + " does not exist!");
    }

    int beg_bin = hist->GetXaxis()->FindBin(range_min);
    int end_bin = hist->GetXaxis()->FindBin(range_max) - 1;

    if(!hist->Integral(beg_bin, end_bin)) {
        throw PRadException("Fit Histogram Failure", "Histogram does not have entries in specified range!");
    }

    TF1 *fit = new TF1("newfit", fit_function.c_str(), range_min, range_max);

    hist->Fit(fit, "qR");

    TF1 *myfit = (TF1*) hist->GetFunction("newfit");

    // print out result
    cout << "Fit histogram " << hist->GetTitle() << endl;
    for(int i = 0; i < myfit->GetNpar(); ++i)
    {
        cout << "Parameter " << i+1 << ": " << myfit->GetParameter(i+1) << endl;
    }
    cout << endl;

    delete fit;
}

void PRadDataHandler::FitPedestal()
{
    for(auto &channel : channelList)
    {
        TH1 *pedHist = channel->GetHist("PED");

        if(pedHist == nullptr || pedHist->Integral() < 1000)
            continue;

        pedHist->Fit("gaus", "qww");

        TF1 *myfit = (TF1*) pedHist->GetFunction("gaus");
        double p0 = myfit->GetParameter(1);
        double p1 = myfit->GetParameter(2);

        channel->UpdatePedestal(p0, p1);
    }
}

void PRadDataHandler::CorrectGainFactor(const int &ref)
{
#define PED_LED_REF 1000  // separation value for led signal and pedestal signal of reference PMT
#define PED_LED_HYC 30 // separation value for led signal and pedestal signal of all HyCal Modules
#define ALPHA_CORR 1228 // least run number that needs alpha correction


    if(ref < 0 || ref > 2) {
        cerr << "Unknown Reference PMT " << ref
             << ", please choose Ref. PMT 1 - 3" << endl;
        return;
    }

    string reference = "LMS" + to_string(ref);

    // we had adjusted the timing window since run 1229, and the adjustment result in larger ADC
    // value from alpha source , the correction is to bring the alpha source value to the same 
    // level as before
    const double correction[3] = {-597.3, -335.4, -219.4};

    // firstly, get the reference factor from LMS PMT
    // LMS 2 seems to be the best one for fitting
    PRadDAQUnit *ref_ch = GetChannel(reference);
    if(ref_ch == nullptr) {
        cerr << "Cannot find the reference PMT channel " << reference
             << " for gain factor correction, abort gain correction." << endl;
        return;
    }

    // reference pmt has both pedestal and alpha source signals in this histogram
    TH1* ref_alpha = ref_ch->GetHist("PHYS");
    TH1* ref_led = ref_ch->GetHist("LMS");
    if(ref_alpha == nullptr || ref_led == nullptr) {
        cerr << "Cannot find the histograms of reference PMT, abort gain correction."  << endl;
        return;
    }

    int sep_bin = ref_alpha->GetXaxis()->FindBin(PED_LED_REF);
    int end_bin = ref_alpha->GetNbinsX(); // 1 for overflow bin and 1 for underflow bin

    if(ref_alpha->Integral(0, sep_bin) < 1000 || ref_alpha->Integral(sep_bin, end_bin) < 1000) {
        cerr << "Not enough entries in pedestal histogram of reference PMT, abort gain correction." << endl;
        return;
    }

    auto fit_gaussian = [] (TH1* hist,
                            const int &range_min = 0,
                            const int &range_max = 8191,
                            const double &warn_ratio = 0.06)
                        {
                            int beg_bin = hist->GetXaxis()->FindBin(range_min);
                            int end_bin = hist->GetXaxis()->FindBin(range_max) - 1;

                            if(hist->Integral(beg_bin, end_bin) < 1000) {
                                cout << "WARNING: Not enough entries in histogram " << hist->GetName()
                                     << ". Abort fitting!" << endl;
                                return 0.;
                            }

                            TF1 *fit = new TF1("tmpfit", "gaus", range_min, range_max);

                            hist->Fit(fit, "qR");
                            TF1 *hist_fit = hist->GetFunction("tmpfit");
                            double mean = hist_fit->GetParameter(1);
                            double sigma = hist_fit->GetParameter(2);
                            if(sigma/mean > warn_ratio) {
                                cout << "WARNING: Bad fitting for "
                                     << hist->GetTitle()
                                     << ". Mean: " << mean
                                     << ", sigma: " << sigma
                                     << endl;
                            }

                            delete fit;

                            return mean;
                        };

    double ped_mean = fit_gaussian(ref_alpha, 0, PED_LED_REF, 0.02);
    double alpha_mean = fit_gaussian(ref_alpha, PED_LED_REF + 1, 8191, 0.05);
    double led_mean = fit_gaussian(ref_led);

    if(ped_mean == 0. || alpha_mean == 0. || led_mean == 0.) {
        cerr << "Failed to get gain factor from reference PMT, abort gain correction." << endl;
        return;
    }

    double ref_factor = led_mean - ped_mean;

    if(runInfo.run_number >= ALPHA_CORR)
        ref_factor /= alpha_mean - ped_mean + correction[ref];
    else
        ref_factor /= alpha_mean - ped_mean;

    for(auto channel : channelList)
    {
        if(!channel->IsHyCalModule())
            continue;

        TH1 *hist = channel->GetHist("LMS");
        if(hist != nullptr) {
            double ch_led = fit_gaussian(hist) - channel->GetPedestal().mean;
            if(ch_led > PED_LED_HYC) {// meaningful led signal
                channel->GainCorrection(ch_led/ref_factor, ref);
            } else {
                cout << "WARNING: Gain factor of " << channel->GetName()
                     << " is not updated due to bad fitting of LED signal."
                     << endl;
            }
        }
    }
}

void PRadDataHandler::ReadGEMConfiguration(const string &path)
{
    gem_srs->LoadConfiguration(path);
}

void PRadDataHandler::ReadTDCList(const string &path)
{
    ConfigParser c_parser;

    if(!c_parser.OpenFile(path)) {
        cout << "WARNING: Fail to open tdc group list file "
             << "\"" << path << "\""
             << ", no tdc groups created from this file."
             << endl;
    }

    string name;
    ChannelAddress addr;

    while (c_parser.ParseLine())
    {
       if(c_parser.NbofElements() == 4) {
            name = c_parser.TakeFirst();
            addr.crate = c_parser.TakeFirst().ULong();
            addr.slot = c_parser.TakeFirst().ULong();
            addr.channel = c_parser.TakeFirst().ULong();

            AddTDCGroup(new PRadTDCGroup(name, addr));
        } else {
            cout << "Unrecognized input format in tdc group list file, skipped one line!"
                 << endl;
        }
    }

    c_parser.CloseFile();
}

void PRadDataHandler::ReadChannelList(const string &path)
{
    ConfigParser c_parser;

    if(!c_parser.OpenFile(path)) {
        cerr << "WARNING: Fail to open channel list file "
                  << "\"" << path << "\""
                  << ", no channel created from this file."
                  << endl;
        return;
    }

    string moduleName;
    ChannelAddress daqAddr;
    string tdcGroup;
    PRadDAQUnit::Geometry geo;

    // some info that is not read from list
    while (c_parser.ParseLine())
    {
        if(c_parser.NbofElements() >= 10) {
            moduleName = c_parser.TakeFirst();
            daqAddr.crate = c_parser.TakeFirst().ULong();
            daqAddr.slot = c_parser.TakeFirst().ULong();
            daqAddr.channel = c_parser.TakeFirst().ULong();
            tdcGroup = c_parser.TakeFirst();

            geo.type = PRadDAQUnit::ChannelType(c_parser.TakeFirst().Int());
            geo.size_x = c_parser.TakeFirst().Double();
            geo.size_y = c_parser.TakeFirst().Double();
            geo.x = c_parser.TakeFirst().Double();
            geo.y = c_parser.TakeFirst().Double();
 
            PRadDAQUnit *new_ch = new PRadDAQUnit(moduleName, daqAddr, tdcGroup, geo);
            AddChannel(new_ch);
        } else {
            cout << "Unrecognized input format in channel list file, skipped one line!"
                 << endl;
        }
    }

    c_parser.CloseFile();

    BuildChannelMap();
}

void PRadDataHandler::ReadEPICSChannels(const string &path)
{
    ConfigParser c_parser;

    if(!c_parser.OpenFile(path)) {
        cout << "WARNING: Fail to open EPICS channel file "
             << "\"" << path << "\""
             << ", no EPICS channel created!"
             << endl;
        return;
    }

    string name;
    float initial_value = EPICS_UNDEFINED_VALUE;

    while(c_parser.ParseLine())
    {
        if(c_parser.NbofElements() == 1) {
            name = c_parser.TakeFirst();
            if(epics_map.find(name) == epics_map.end()) {
                epics_map[name] = epics_values.size();
                epics_values.push_back(initial_value);
            } else {
                cout << "Duplicated epics channel " << name
                     << ", its channel id is " << epics_map[name]
                     << endl;
            }
        } else {
            cout << "Unrecognized input format in  epics channel file, skipped one line!"
                 << endl;
        }
    }

    c_parser.CloseFile();
};

void PRadDataHandler::ReadPedestalFile(const string &path)
{
    ConfigParser c_parser;

    if(!c_parser.OpenFile(path)) {
        cout << "WARNING: Fail to open pedestal file "
             << "\"" << path << "\""
             << ", no pedestal data are read!"
             << endl;
        return;
    }

    double val, sigma;
    ChannelAddress daqInfo;
    PRadDAQUnit *tmp;

    while(c_parser.ParseLine())
    {
        if(c_parser.NbofElements() == 5) {
            daqInfo.crate = c_parser.TakeFirst().ULong();
            daqInfo.slot = c_parser.TakeFirst().ULong();
            daqInfo.channel = c_parser.TakeFirst().ULong();
            val = c_parser.TakeFirst().Double();
            sigma = c_parser.TakeFirst().Double();

            if((tmp = GetChannel(daqInfo)) != nullptr)
                tmp->UpdatePedestal(val, sigma);
        } else {
            cout << "Unrecognized input format in pedestal data file, skipped one line!"
                 << endl;
        }
    }

    c_parser.CloseFile();
};

void PRadDataHandler::ReadGEMPedestalFile(const string &path)
{
    gem_srs->LoadPedestal(path);
}

void PRadDataHandler::ReadCalibrationFile(const string &path)
{
    ConfigParser c_parser;

    if(!c_parser.OpenFile(path)) {
        cout << "WARNING: Failed to calibration factor file "
             << " \"" << path << "\""
             << " , no calibration factors updated!"
             << endl;
        return;
    }

    string name;
    double calFactor;
    PRadDAQUnit *tmp;

    while(c_parser.ParseLine())
    {
        if(c_parser.NbofElements() == 5) {
            vector<double> ref_gain;
            name = c_parser.TakeFirst();
            calFactor = c_parser.TakeFirst().Double();

            ref_gain.push_back(c_parser.TakeFirst().Double()); // ref 1
            ref_gain.push_back(c_parser.TakeFirst().Double()); // ref 2
            ref_gain.push_back(c_parser.TakeFirst().Double()); // ref 3

            PRadDAQUnit::CalibrationConstant calConst(calFactor, ref_gain);

            if((tmp = GetChannel(name)) != nullptr)
                tmp->UpdateCalibrationConstant(calConst);
        } else {
            cout << "Unrecognized input format in calibration factor file, skipped one line!"
                 << endl;
        }

    }

    c_parser.CloseFile();
}

void PRadDataHandler::ReadGainFactor(const string &path, const int &ref)
{
    if(ref < 0 || ref > 2) {
        cerr << "Unknown Reference PMT " << ref
             << ", please choose Ref. PMT 1 - 3" << endl;
        return;
    }

    ConfigParser c_parser;

    if(!c_parser.OpenFile(path)) {
        cout << "WARNING: Failed to gain factor file "
             << " \"" << path << "\""
             << " , no gain factors updated!"
             << endl;
        return;
    }

    string name;
    double ref_gain[3];
    PRadDAQUnit *tmp;

    while(c_parser.ParseLine())
    {
        if(c_parser.NbofElements() == 2) {
            name = c_parser.TakeFirst();
            ref_gain[0] = c_parser.TakeFirst().Double();
            ref_gain[1] = c_parser.TakeFirst().Double();
            ref_gain[2] = c_parser.TakeFirst().Double();

            if((tmp = GetChannel(name)) != nullptr)
                tmp->GainCorrection(ref_gain[ref-1], ref); //TODO we only use reference 2 for now
        } else {
            cout << "Unrecognized input format in gain factor file, skipped one line!"
                 << endl;
        }

    }

    c_parser.CloseFile();
}

// Refill energy hist after correct gain factos
void PRadDataHandler::RefillEnergyHist()
{
    energyHist->Reset();

    for(auto &event : energyData)
    {
        if(!event.is_physics_event())
            continue;

        double ene = 0.;
        for(auto &adc : event.adc_data)
        {
            if(adc.channel_id >= channelList.size())
                continue;

            if(channelList[adc.channel_id]->IsHyCalModule())
                ene += channelList[adc.channel_id]->GetEnergy(adc.value);
        }
        energyHist->Fill(ene);
    }
}

void PRadDataHandler::ReadFromEvio(const string &path, const int &evt, const bool &verbose)
{
    parser->ReadEvioFile(path.c_str(), evt, verbose);
    WaitEventProcess();
}

void PRadDataHandler::ReadFromSplitEvio(const string &path, const int &split, const bool &verbose)
{
    if(split < 0) {// default input, no split
        ReadFromEvio(path.c_str(), -1, verbose);
    } else {
        for(int i = 0; i <= split; ++i)
        {
            string split_path = path + "." + to_string(i);
            ReadFromEvio(split_path.c_str(), -1, verbose);
        }
    }
}

void PRadDataHandler::InitializeByData(const string &path, int run, int ref)
{
    PRadBenchMark timer;

    cout << "Data Handler: Initializing from Data File "
         << "\"" << path << "\"."
         << endl;

    if(!path.empty()) {
        // auto update run number
        if(run < 0)
            GetRunNumberFromFileName(path);
        else
            SetRunNumber(run);

        gem_srs->SetPedestalMode(true);

        parser->ReadEvioFile(path.c_str(), 20000);
    }

    cout << "Data Handler: Fitting Pedestal for HyCal." << endl;
    FitPedestal();

    cout << "Data Handler: Correct HyCal Gain Factor, Run Number: " << runInfo.run_number << "." << endl;
    CorrectGainFactor(ref);

    cout << "Data Handler: Fitting Pedestal for GEM." << endl;
    gem_srs->FitPedestal();
//    gem_srs->SavePedestal("gem_ped_" + to_string(runInfo.run_number) + ".dat");
//    gem_srs->SaveHistograms("gem_ped_" + to_string(runInfo.run_number) + ".root");

    cout << "Data Handler: Releasing Memeory." << endl;
    gem_srs->SetPedestalMode(false);

    // save run number
    int run_number = runInfo.run_number;
    Clear();
    SetRunNumber(run_number);

    cout << "Data Handler: Done initialization, took " << timer.GetElapsedTime()/1000. << " s" << endl;
}

void PRadDataHandler::GetRunNumberFromFileName(const string &name, const size_t &pos, const bool &verbose)
{
    // get rid of suffix
    auto nameEnd = name.find(".evio");

    if(nameEnd == string::npos)
        nameEnd = name.size();
    else
        nameEnd -= 1;

    // get rid of directories
    auto nameBeg = name.find_last_of("/");
    if(nameBeg == string::npos)
        nameBeg = 0;
    else
        nameBeg += 1;

    int number = ConfigParser::find_integer(name.substr(nameBeg, nameEnd - nameBeg + 1), pos);

    if(number > 0) {

        if(verbose) {
            cout << "Data Handler: Run number is automatcially determined from file name."
                 << endl
                 << "File name: " << name
                 << endl
                 << "Run number: " << number
                 << endl;
        }

        SetRunNumber(number);
    }
}

// find event by its event number
// it is assumed the files decoded are all from 1 single run and they are loaded in order
// otherwise this function will not work properly
int PRadDataHandler::FindEventIndex(const int &ev)
{
    int result = -1;

    if(ev < 0) {
        cout << "Data Handler: Cannot find event with negative event number!" << endl;
        return result;
    }

    if(!energyData.size()) {
        cout << "Data Handler: No event found since data bank is empty." << endl;
        return result;
    }

    int data_begin = 0;
    int data_end = (int)energyData.size() - 1;

    int first_event = energyData.at(0).event_number;

    if(ev > first_event) {
        int index = ev - first_event;
        if(index >= data_end)
            index = data_end;
        int diff = energyData.at(index).event_number - ev;
        while(diff > 0)
        {
            index -= diff;
            if(index <= 0) {
                index = 0;
                break;
            }
            diff = energyData.at(index).event_number - ev;
        }

        data_begin = index;
    }

    for(int i = data_begin; i < data_end; ++i) {
        if(energyData.at(i) == ev)
            return i;
    }

    return result;
}

vector<HyCalHit> &PRadDataHandler::GetHyCalCluster(const int &event_index)
{
    return hycal_recon->CoarseHyCalReconstruct(event_index);
}

vector<HyCalHit> &PRadDataHandler::GetHyCalCluster(EventData &event)
{
    return hycal_recon->CoarseHyCalReconstruct(event);
}

void PRadDataHandler::Replay(const string &r_path, const int &split, const string &w_path)
{
    if(w_path.empty()) {
        string file = "prad_" + to_string(runInfo.run_number) + ".dst";
        dst_parser->OpenOutput(file);
    } else {
        dst_parser->OpenOutput(w_path);
    }

    cout << "Replay started!" << endl;
    PRadBenchMark timer;

    dst_parser->WriteHyCalInfo();
    dst_parser->WriteGEMInfo();
    dst_parser->WriteEPICSMap();

    replayMode = true;

    ReadFromSplitEvio(r_path, split);

    dst_parser->WriteRunInfo();

    replayMode = false;

    cout << "Replay done, took " << timer.GetElapsedTime()/1000. << " s!" << endl;
    dst_parser->CloseOutput();
}

void PRadDataHandler::ReadFromDST(const string &path, const uint32_t &mode)
{
    try {
        dst_parser->OpenInput(path);

        dst_parser->SetMode(mode);

        cout << "Data Handler: Reading events from DST file "
             << "\"" << path << "\""
             << endl;

        while(dst_parser->Read())
        {
            switch(dst_parser->EventType())
            {
            case PRad_DST_Event:
                FillHistograms(dst_parser->GetEvent());
                energyData.push_back(dst_parser->GetEvent());
                break;
            case PRad_DST_Epics:
                epicsData.push_back(dst_parser->GetEPICSEvent());
                break;
            default:
                break;
            }
        }

    } catch(PRadException &e) {
        cerr << e.FailureType() << ": "
             << e.FailureDesc() << endl
             << "Write to DST Aborted!" << endl;
    } catch(exception &e) {
        cerr << e.what() << endl
             << "Write to DST Aborted!" << endl;
    }
    dst_parser->CloseInput();
 }

void PRadDataHandler::WriteToDST(const string &path)
{
    try {
        dst_parser->OpenOutput(path);

        cout << "Data Handler: Saving DST file "
             << "\"" << path << "\""
             << endl;

        dst_parser->WriteHyCalInfo();
        dst_parser->WriteGEMInfo();
        dst_parser->WriteEPICSMap();

        for(auto &epics : epicsData)
        {
            dst_parser->WriteEPICS(epics);
        }

        for(auto &event : energyData)
        {
            dst_parser->WriteEvent(event);
        }

        dst_parser->WriteRunInfo();

    } catch(PRadException &e) {
        cerr << e.FailureType() << ": "
             << e.FailureDesc() << endl
             << "Write to DST Aborted!" << endl;
    } catch(exception &e) {
        cerr << e.what() << endl
             << "Write to DST Aborted!" << endl;
    }

    dst_parser->CloseOutput();
}

