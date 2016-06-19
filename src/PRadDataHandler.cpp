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
#include <iomanip>
#include <algorithm>
#include "PRadDataHandler.h"
#include "PRadEvioParser.h"
#include "PRadGEMSystem.h"
#include "PRadDAQUnit.h"
#include "PRadTDCGroup.h"
#include "ConfigParser.h"
#include "TFile.h"
#include "TList.h"
#include "TF1.h"
#include "TH1.h"
#include "TH2.h"

#define EPICS_UNDEFINED_VALUE -9999.9

#define DST_FILE_VERSION 0x12  // 0xff

using namespace std;

PRadDataHandler::PRadDataHandler()
: parser(new PRadEvioParser(this)), gem_srs(new PRadGEMSystem()),
  totalE(0), onlineMode(false), current_event(0)
{
    // total energy histogram
    energyHist = new TH1D("HyCal Energy", "Total Energy (MeV)", 2500, 0, 2500);
    TagEHist = new TH2I("Tagger E", "Tagger E counter", 2000, 0, 20000, 384, 0, 383);
    TagTHist = new TH2I("Tagger T", "Tagger T counter", 2000, 0, 20000, 128, 0, 127);

    triggerScalars.push_back(ScalarChannel("Lead Glass Sum"));
    triggerScalars.push_back(ScalarChannel("Total Sum"));
    triggerScalars.push_back(ScalarChannel("LMS Led"));
    triggerScalars.push_back(ScalarChannel("LMS Alpha Source"));
    triggerScalars.push_back(ScalarChannel("Tagger Master OR"));
    triggerScalars.push_back(ScalarChannel("Scintillator"));
    triggerScalars.push_back(ScalarChannel("Faraday Cage"));
    triggerScalars.push_back(ScalarChannel("External Pulser"));
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
    delete gem_srs;
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

// erase the data container
void PRadDataHandler::Clear()
{
    // used memory won't be released, but it can be used again for new data file
    energyData.clear();
    epicsData.clear();
    newEvent.clear();
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
    if(newEvent.trigger && (newEvent.trigger != trg)) {
        cerr << "ERROR: Trigger type mismatch at event "
             << parser->GetEventNumber()
             << ", was " << (int) newEvent.trigger
             << " now " << (int) trg
             << endl;
    }
    newEvent.trigger = trg;
}

void PRadDataHandler::UpdateScalarGroup(const unsigned int &size, const unsigned int *gated, const unsigned int *ungated)
{
    if(size > triggerScalars.size())
    {
        cerr << "ERROR: Received too many scalar channels, scalar counts will not be updated" << endl;
        return;
    }

    for(unsigned int i = 0; i < size; ++i)
    {
        triggerScalars[i].gated_count = gated[i];
        triggerScalars[i].count = ungated[i];
    }
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

void PRadDataHandler::AccumulateBeamCharge(const double &c)
{
    if(newEvent.isPhysicsEvent())
        runInfo.beam_charge += c;
}

void PRadDataHandler::UpdateLiveTimeScaler(const unsigned int &ungated, const unsigned int &gated)
{
    if(newEvent.isPhysicsEvent()) {
        runInfo.ungated_count += (double) ungated;
        runInfo.dead_count += (double) gated;
    }
}

float PRadDataHandler::GetEPICSValue(const string &name)
{
    auto it = epics_map.find(name);
    if(it == epics_map.end())
        return EPICS_UNDEFINED_VALUE;

    return epics_values.at(it->second);
}

float PRadDataHandler::GetEPICSValue(const string &name, const int &index)
{
    if((unsigned int)index >= energyData.size())
        return GetEPICSValue(name);

    auto it = epics_map.find(name);
    if(it == epics_map.end())
        return EPICS_UNDEFINED_VALUE;

    size_t channel_id = it->second;
    unsigned int epics_idx = (unsigned int) energyData.at(index).last_epics;

    if( (epics_idx > epicsData.size()) ||
        (channel_id > epicsData.at(epics_idx).values.size()) )
    {
        return EPICS_UNDEFINED_VALUE;
    }
        
    return epicsData.at(epics_idx).values.at(channel_id);
}

void PRadDataHandler::FeedData(JLabTIData &tiData)
{
    newEvent.timestamp = tiData.time_high;
    newEvent.timestamp <<= 32;
    newEvent.timestamp |= tiData.time_low;
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

    if(newEvent.isPhysicsEvent()) {
        unsigned short sparsify = channel->Sparsification(adcData.val);

        if(sparsify) {
            // fill histograms by trigger type, only sparsified channel get through for physics events
            channel->FillHist(adcData.val, newEvent.trigger);

#ifdef MULTI_THREAD
            // unfortunately, we have some non-local variable to deal with
            // so lock the thread to prevent concurrent access
            myLock.lock();
#endif
            if(channel->IsHyCalModule())
                totalE += channel->Calibration(sparsify); // calculate total energy of this event

            newEvent.add_adc(ADC_Data(channel->GetID(), adcData.val)); // store this data word
#ifdef MULTI_THREAD
            myLock.unlock();
#endif
        }

    } else if (newEvent.isMonitorEvent()) {
        // fill histograms by trigger type
        channel->FillHist(adcData.val, newEvent.trigger);

#ifdef MULTI_THREAD
        myLock.lock();
#endif
        newEvent.add_adc(ADC_Data(channel->GetID(), adcData.val)); 
#ifdef MULTI_THREAD
        myLock.unlock();
#endif
    }
    
}

void PRadDataHandler::FeedData(TDCV767Data &tdcData)
{
    auto it = map_daq_tdc.find(tdcData.config);
    if(it == map_daq_tdc.end())
        return;

    PRadTDCGroup *tdc = it->second;
    if(newEvent.isPhysicsEvent())
        tdc->FillHist(tdcData.val);


    newEvent.tdc_data.push_back(TDC_Data(tdc->GetID(), tdcData.val));
}

void PRadDataHandler::FeedData(TDCV1190Data &tdcData)
{
    if(tdcData.config.crate == PRadTS) {
        auto it = map_daq_tdc.find(tdcData.config);
        if(it == map_daq_tdc.end()) {
            return;
        }

        PRadTDCGroup *tdc = it->second;
        if(newEvent.isPhysicsEvent())
            tdc->FillHist(tdcData.val);

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
    }
}

// feed GEM data
void PRadDataHandler::FeedData(GEMRawData &gemData)
{
    PRadGEMAPV *apv = gem_srs->GetAPV(gemData.fec, gemData.adc);

    if(apv) {
        apv->FillRawData(gemData.buf, gemData.size);
    }
}

// signal of new event
void PRadDataHandler::StartofNewEvent(const unsigned char &tag)
{
    // clear buffer for nes event
    newEvent.initialize(tag);
    totalE = 0;
}

// signal of event end, save event or discard event in online mode
void PRadDataHandler::EndofThisEvent(const unsigned int &ev)
{
    if(newEvent.type == CODA_Event) {

        newEvent.event_number = ev;
        newEvent.gem_data = gem_srs->GetZeroSupData();
        if(onlineMode && energyData.size()) { // online mode only saves the last event, to reduce usage of memory
            energyData.pop_front();
        }

        energyData.push_back(newEvent); // save event

        if(newEvent.isPhysicsEvent()) {
            energyHist->Fill(totalE); // fill energy histogram
        }

    } else if(newEvent.type == EPICS_Info) {

        if(onlineMode && epicsData.size()) {
            epicsData.pop_front();
        }

        epicsData.push_back(EPICSData(ev, epics_values));
        newEvent.last_epics = epicsData.size() - 1; // update the epics info on newEvent

    }
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
        epics_list.push_back(epics_ch(ch.first, ch.second));
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
}

EventData &PRadDataHandler::GetEventData(const unsigned int &index)
{
    if(index >= energyData.size()) {
        return energyData.back();
    } else {
        return energyData.at(index);
    }
}

EPICSData &PRadDataHandler::GetEPICSData(const unsigned int &index)
{
    if(index >= epicsData.size()) {
        return epicsData.back();
    } else {
        return epicsData.at(index);
    }
}

unsigned int PRadDataHandler::GetScalarCount(const unsigned int &group, const bool &gated)
{
    if(group >= triggerScalars.size())
        return 0;
    if(gated)
        return triggerScalars[group].gated_count;
    else
        return triggerScalars[group].count;
}

vector<unsigned int> PRadDataHandler::GetScalarsCount(const bool &gated)
{
    vector<unsigned int> result;

    for(auto scalar : triggerScalars)
    {
        if(gated)
            result.push_back(scalar.gated_count);
        else
            result.push_back(scalar.count);
    }

    return result;
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
        if(!event.isPhysicsEvent())
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

void PRadDataHandler::ReadFromDST(const string &path, ios::openmode mode)
{
    ifstream input;

    try {
        input.open(path, mode);

        if(!input.is_open()) {
            cerr << "Data Handler: Cannot open input dst file "
                 << "\"" << path << "\""
                 << ", stop reading!" << endl;
            return;
        }

        input.seekg(0, input.end);
        int length = input.tellg();
        input.seekg(0, input.beg);

        uint32_t version_info, event_info;
        input.read((char*) &version_info, sizeof(version_info));

        if((version_info >> 8) != PRad_DST_Header) {
            cerr << "Data Handler: Unrecognized PRad dst file, stop reading!" << endl;
            return;
        }
        if((version_info & 0xff) != DST_FILE_VERSION) {
            cerr << "Data Handler: Version mismatch between the file and library. "
                 << endl
                 << "Expected version " << (DST_FILE_VERSION >> 4)
                 << "." << (DST_FILE_VERSION & 0xf)
                 << ", but the file version is "
                 << ((version_info >> 4) & 0xf)
                 << "." << (version_info & 0xf)
                 << ", please use correct library to open this file."
                 << endl;
            return;
        }

        cout << "Data Handler: Reading DST file "
             << "\"" << path << "\"." << endl;

        while(input.tellg() < length && input.tellg() != -1)
        {
            input.read((char*) &event_info, sizeof(event_info));

            if((event_info >> 8) != PRad_DST_EvHeader) {
                cerr << "Data Handler: Unrecognized event header "
                     << hex << setw(8) << setfill('0') << event_info
                     <<" in PRad dst file, probably corrupted file!"
                     << dec << endl;
                return;
            }
            // read by event type
            switch(event_info&0xff)
            {
            case PRad_DST_Event:
              {
                EventData event;
                ReadFromDST(input, event);
                energyData.push_back(event);
              } break;
            case PRad_DST_Epics:
              {
                EPICSData epics;
                ReadFromDST(input, epics);
                epicsData.push_back(epics);
              } break;
            case PRad_DST_Epics_Map:
                ReadEPICSMapFromDST(input);
                break;
            case PRad_DST_Run_Info:
                ReadRunInfoFromDST(input);
                break;
            case PRad_DST_HyCal_Info:
                ReadHyCalInfoFromDST(input);
                break;
            default:
                break;
            }
        }

    } catch(PRadException &e) {
        cerr << e.FailureType() << ": "
             << e.FailureDesc() << endl
             << "Read from DST Aborted!" << endl;
    } catch(exception &e) {
        cerr << e.what() << endl
             << "Read from DST Aborted!" << endl;
    }

    input.close();

}

void PRadDataHandler::ReadFromDST(ifstream &dst_file, EventData &data) throw(PRadException)
{
    if(!dst_file.is_open())
        throw PRadException("READ DST", "input file is not opened!");

    // event information
    dst_file.read((char*) &data.event_number, sizeof(data.event_number));
    dst_file.read((char*) &data.type        , sizeof(data.type));
    dst_file.read((char*) &data.trigger     , sizeof(data.trigger));
    dst_file.read((char*) &data.timestamp   , sizeof(data.timestamp));
    dst_file.read((char*) &data.last_epics  , sizeof(data.last_epics));

    size_t adc_size, tdc_size, gem_size, value_size;
    ADC_Data adc;
    TDC_Data tdc;
 
    dst_file.read((char*) &adc_size, sizeof(adc_size));
    totalE = 0;
    for(size_t i = 0; i < adc_size; ++i)
    {
        dst_file.read((char*) &adc, sizeof(adc));
        data.add_adc(adc);
        if(adc.channel_id < channelList.size()) {
            channelList[adc.channel_id]->FillHist(adc.value, data.trigger);
            totalE += channelList[adc.channel_id]->GetEnergy(adc.value);
        }
    }

    if(data.isPhysicsEvent())
        energyHist->Fill(totalE);
    
    dst_file.read((char*) &tdc_size, sizeof(tdc_size));
    for(size_t i = 0; i < tdc_size; ++i)
    {
        dst_file.read((char*) &tdc, sizeof(tdc));
        data.add_tdc(tdc);
        if(data.isPhysicsEvent() && (tdc.channel_id < tdcList.size())) {
            tdcList[tdc.channel_id]->FillHist(tdc.value);
        }
    }

    float value;
    dst_file.read((char*) &gem_size, sizeof(gem_size));
    for(size_t i = 0; i < gem_size; ++i)
    {
        GEM_Data gemhit;
        dst_file.read((char*) &gemhit.addr, sizeof(gemhit.addr));
        dst_file.read((char*) &value_size, sizeof(value_size));
        for(size_t i = 0; i < value_size; ++i)
        {
            dst_file.read((char*) &value, sizeof(value));
            gemhit.add_value(value);
        }
        data.add_gemhit(gemhit);
    }

}

void PRadDataHandler::ReadFromDST(ifstream &dst_file, EPICSData &data) throw(PRadException)
{
    if(!dst_file.is_open())
        throw PRadException("READ DST", "input file is not opened!");

    dst_file.read((char*) &data.event_number, sizeof(data.event_number));

    size_t value_size;
    dst_file.read((char*) &value_size, sizeof(value_size));

    for(size_t i = 0; i < value_size; ++i)
    {
        float value;
        dst_file.read((char*) &value, sizeof(value));
        data.values.push_back(value);
    }
}


void PRadDataHandler::WriteToDST(const string &path, ios::openmode mode)
{
    ofstream output;

    try {
        output.open(path, mode);

        if(!output.is_open()) {
            cerr << "Data Handler: Cannot open output file " << path
                 << ", stop writing!" << endl;
            return;
        }

        uint32_t version_info = (PRad_DST_Header << 8) | DST_FILE_VERSION;
        output.write((char*) &version_info, sizeof(version_info));

        WriteRunInfoToDST(output);
        WriteHyCalInfoToDST(output);
        WriteEPICSMapToDST(output);

        for(auto &event : energyData)
        {
            WriteToDST(output, event);
        }

        for(auto &epics : epicsData)
        {
            WriteToDST(output, epics);
        }

    } catch(PRadException &e) {
        cerr << e.FailureType() << ": "
             << e.FailureDesc() << endl
             << "Write to DST Aborted!" << endl;
    } catch(exception &e) {
        cerr << e.what() << endl
             << "Write to DST Aborted!" << endl;
    }

    output.close();
}

void PRadDataHandler::WriteToDST(ofstream &dst_file, const EventData &data) throw(PRadException)
{
    if(!dst_file.is_open())
        throw PRadException("WRITE DST", "output file is not opened!");

    // write header
    uint32_t event_info = (PRad_DST_EvHeader << 8) | PRad_DST_Event;
    dst_file.write((char*) &event_info, sizeof(event_info));

    // event information
    dst_file.write((char*) &data.event_number, sizeof(data.event_number));
    dst_file.write((char*) &data.type        , sizeof(data.type));
    dst_file.write((char*) &data.trigger     , sizeof(data.trigger));
    dst_file.write((char*) &data.timestamp   , sizeof(data.timestamp));
    dst_file.write((char*) &data.last_epics  , sizeof(data.last_epics));

    // all data banks
    size_t adc_size = data.adc_data.size();
    size_t tdc_size = data.tdc_data.size();
    size_t gem_size = data.gem_data.size();

    dst_file.write((char*) &adc_size, sizeof(adc_size));
    for(auto &adc : data.adc_data)
        dst_file.write((char*) &adc, sizeof(adc));

    dst_file.write((char*) &tdc_size, sizeof(tdc_size));
    for(auto &tdc : data.tdc_data)
        dst_file.write((char*) &tdc, sizeof(tdc));

    dst_file.write((char*) &gem_size, sizeof(gem_size));
    for(auto &gem : data.gem_data)
    {
        dst_file.write((char*) &gem.addr, sizeof(gem.addr));
        size_t hit_size = gem.values.size();
        dst_file.write((char*) &hit_size, sizeof(hit_size));
        for(auto &value : gem.values)
            dst_file.write((char*) &value, sizeof(value));
    }

}

void PRadDataHandler::WriteToDST(ofstream &dst_file, const EPICSData &data) throw(PRadException)
{
    if(!dst_file.is_open())
        throw PRadException("WRITE DST", "output file is not opened!");

    // write header
    uint32_t event_info = (PRad_DST_EvHeader << 8) | PRad_DST_Epics;
    dst_file.write((char*) &event_info, sizeof(event_info));

    dst_file.write((char*) &data.event_number, sizeof(data.event_number));

    size_t value_size = data.values.size();
    dst_file.write((char*) &value_size, sizeof(value_size));

    for(auto value : data.values)
        dst_file.write((char*) &value, sizeof(value));
}

void PRadDataHandler::ReadFromEvio(const string &path, const int &split, const bool &verbose)
{
    if(split < 0) {// default input, no split
        parser->ReadEvioFile(path.c_str(), verbose);
    } else {
        for(int i = 0; i <= split; ++i)
        {
            string split_path = path + "." + to_string(i);
            parser->ReadEvioFile(split_path.c_str(), verbose);
        }
    }
}

void PRadDataHandler::InitializeByData(const string &path, int run, int ref)
{
    if(!path.empty()) {
        // auto update run number
        if(run < 0)
            GetRunNumberFromFileName(path);
        else
            SetRunNumber(run);

        cout << "Data Handler: Initializing from data file "
             << "\"" << path << "\", "
             << "run number: " << runInfo.run_number
             << "." << endl;

        parser->ReadEvioFile(path.c_str());
    }

    FitPedestal();

    CorrectGainFactor(ref);

    Clear();

    cout << "Data Handler: Done initialization." << endl;
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

void PRadDataHandler::WriteEPICSMapToDST(ofstream &dst_file) throw(PRadException)
{
    if(!dst_file.is_open())
        throw PRadException("WRITE DST", "output file is not opened!");

    // write header
    uint32_t event_info = (PRad_DST_EvHeader << 8) | PRad_DST_Epics_Map;
    dst_file.write((char*) &event_info, sizeof(event_info));

    vector<epics_ch> epics_channels = GetSortedEPICSList();

    size_t ch_size = epics_channels.size();
    dst_file.write((char*) &ch_size, sizeof(ch_size));

    for(auto &ch : epics_channels)
    {
        size_t str_size = ch.name.size();
        dst_file.write((char*) &str_size, sizeof(str_size));
        for(auto &c : ch.name)
            dst_file.write((char*) &c, sizeof(c));
        dst_file.write((char*) &ch.id, sizeof(ch.id));
        dst_file.write((char*) &epics_values.at(ch.id), sizeof(epics_values.at(ch.id)));
    }
}

void PRadDataHandler::ReadEPICSMapFromDST(ifstream &dst_file) throw(PRadException)
{
    if(!dst_file.is_open())
        throw PRadException("READ DST", "input file is not opened!");

    epics_map.clear();
    epics_values.clear();

    size_t ch_size, str_size, id;
    string str;
    float value;

    dst_file.read((char*) &ch_size, sizeof(ch_size));
    for(size_t i = 0; i < ch_size; ++i)
    {
        str = "";
        dst_file.read((char*) &str_size, sizeof(str_size));
        for(size_t i = 0; i < str_size; ++i)
        {
            char c;
            dst_file.read(&c, sizeof(c));
            str.push_back(c);
        }
        dst_file.read((char*) &id, sizeof(id));
        dst_file.read((char*) &value, sizeof(value));
        epics_map[str] = id;
        epics_values.push_back(value);
    }
}

void PRadDataHandler::WriteRunInfoToDST(ofstream &dst_file) throw(PRadException)
{
    if(!dst_file.is_open())
        throw PRadException("WRITE DST", "output file is not opened!");

    // write header
    uint32_t event_info = (PRad_DST_EvHeader << 8) | PRad_DST_Run_Info;
    dst_file.write((char*) &event_info, sizeof(event_info));
    dst_file.write((char*) &runInfo, sizeof(runInfo));
}

void PRadDataHandler::ReadRunInfoFromDST(ifstream &dst_file) throw(PRadException)
{
    if(!dst_file.is_open())
        throw PRadException("READ DST", "input file is not opened!");

    dst_file.read((char*) &runInfo, sizeof(runInfo));
}

void PRadDataHandler::WriteHyCalInfoToDST(ofstream &dst_file) throw(PRadException)
{
    if(!dst_file.is_open())
        throw PRadException("WRITE DST", "output file is not opened!");

    // write header
    uint32_t event_info = (PRad_DST_EvHeader << 8) | PRad_DST_HyCal_Info;
    dst_file.write((char*) &event_info, sizeof(event_info));

    size_t ch_size = channelList.size();
    dst_file.write((char*) &ch_size, sizeof(ch_size));

    for(auto channel : channelList)
    {
        PRadDAQUnit::Pedestal ped = channel->GetPedestal();
        dst_file.write((char*) &ped, sizeof(ped));

        PRadDAQUnit::CalibrationConstant cal = channel->GetCalibrationConstant();
        size_t gain_size = cal.base_gain.size();
        dst_file.write((char*) &cal.factor, sizeof(cal.factor)); 
        dst_file.write((char*) &cal.base_factor, sizeof(cal.base_factor));
        dst_file.write((char*) &gain_size, sizeof(gain_size));
        for(auto &gain : cal.base_gain)
            dst_file.write((char*) &gain, sizeof(gain));
    }
}

void PRadDataHandler::ReadHyCalInfoFromDST(ifstream &dst_file) throw(PRadException)
{
    if(!dst_file.is_open())
        throw PRadException("READ DST", "input file is not opened!");

    size_t ch_size;
    dst_file.read((char*) &ch_size, sizeof(ch_size));

    for(size_t i = 0; i < ch_size; ++i)
    {
        PRadDAQUnit::Pedestal ped;
        dst_file.read((char*) &ped, sizeof(ped));

        PRadDAQUnit::CalibrationConstant cal;
        dst_file.read((char*) &cal.factor, sizeof(cal.factor));
        dst_file.read((char*) &cal.base_factor, sizeof(cal.base_factor));

        double gain;
        size_t gain_size;
        dst_file.read((char*) &gain_size, sizeof(gain_size));

        for(size_t i = 0; i < gain_size; ++i)
        {
            dst_file.read((char*) &gain, sizeof(gain));
            cal.base_gain.push_back(gain);
        }

        if(i < channelList.size()) {
            channelList.at(i)->UpdatePedestal(ped);
            channelList.at(i)->UpdateCalibrationConstant(cal);
        }
    }
}
