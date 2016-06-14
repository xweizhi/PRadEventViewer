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
#include "PRadDAQUnit.h"
#include "PRadTDCGroup.h"
#include "ConfigParser.h"
#include "TFile.h"
#include "TList.h"
#include "TF1.h"
#include "TH1.h"
#include "TH2.h"

using namespace std;

static bool operator < (const int &e, const EPICSValue &v)
{
    return e < v.att_event;
}


PRadDataHandler::PRadDataHandler()
: parser(new PRadEvioParser(this)), totalE(0), charge(0), onlineMode(false)
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
    totalE = 0;
    charge = 0;
    newEvent.clear();
    energyHist->Reset();
    parser->SetEventNumber(0);

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
             << parser->GetEventNumber()
             << ", was " << (int) newEvent.type
             << " now " << (int) trg
             << endl;
    }
    newEvent.type = trg;
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
    auto it = epics_channels.find(name);
    int ev = parser->GetEventNumber();

    if(it == epics_channels.end()) {
        vector<EPICSValue> epics_ch;
        epics_ch.push_back(EPICSValue(ev, value));
        epics_channels[name] = epics_ch;
    } else {
        if(onlineMode) {
            (*it).second.back() = EPICSValue(ev, value);
        } else {
            (*it).second.push_back(EPICSValue(ev, value));
        }
    }
}

void PRadDataHandler::AccumulateBeamCharge(const double &c)
{
    if(newEvent.isPhysicsEvent())
        charge += c;
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
    if((unsigned int)event >= energyData.size())
        return FindEPICSValue(name);

    int index = energyData.at(event).event_number;
 
    auto it = epics_channels.find(name);
    if(it == epics_channels.end())
        return 0;

    vector<EPICSValue> &data = (*it).second;

    if(data.size() < 1)
        return 0;

    if(index < data.at(0))
        return data.at(0).value;

    auto idx = upper_bound(data.begin(), data.end(), index) - data.begin() - 1;

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
    auto it = map_daq.find(adcData.config);

    // did not find any channel
    if(it == map_daq.end())
        return;

    // get the channel
    PRadDAQUnit *channel = it->second;

    channel->FillHist(adcData.val, (PRadTriggerType)newEvent.type);

    unsigned short sparVal = channel->Sparsification(adcData.val, newEvent.isPhysicsEvent());

    if(sparVal) // only store events above pedestal in memory
    {
        ADC_Data word(channel->GetID(), sparVal); // save id because it saves memory
#ifdef MULTI_THREAD
        // unfortunately, we have some non-local variable to deal with
        // so lock the thread to prevent concurrent access
        myLock.lock();
#endif
        if(channel->IsHyCalModule())
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
    auto it = map_daq_tdc.find(tdcData.config);
    if(it == map_daq_tdc.end())
        return;

    PRadTDCGroup *tdc = it->second;
    tdc->GetHist()->Fill(tdcData.val);

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


// signal of new event
void PRadDataHandler::StartofNewEvent()
{
    // clear buffer for nes event
    newEvent.clear();
    totalE = 0; 
}

// signal of event end, save event or discard event in online mode
void PRadDataHandler::EndofThisEvent(const unsigned int &ev)
{
    newEvent.event_number = ev;

    if(onlineMode && energyData.size()) { // online mode only saves the last event, to reduce usage of memory
        energyData.pop_front();
    }

    energyData.push_back(newEvent); // save event

    if(newEvent.isPhysicsEvent()) {
        energyHist->Fill(totalE); // fill energy histogram
    }
}

// show the event to event viewer
void PRadDataHandler::ChooseEvent(const int &idx)
{
    totalE = 0;
    EventData event;

    // != avoids operator definition for non-standard map
    for(auto &channel : channelList)
    {
        channel->UpdateEnergy(0);
    }

    for(auto &tdc_ch : tdcList)
    {
        tdc_ch->ClearTimeMeasure();
    }

    if (energyData.size()) { // offline mode, pick the event given by console
        if((unsigned int) idx >= energyData.size())
            event = energyData.back();
        else
            event = energyData.at(idx);
    } else {
        return;
    }

    for(auto &adc : event.adc_data)
    {
        channelList[adc.channel_id]->UpdateEnergy(adc.value);
        if(channelList[adc.channel_id]->IsHyCalModule())
            totalE += channelList[adc.channel_id]->GetEnergy();
    }

    for(auto &tdc : event.tdc_data)
    {
        tdcList[tdc.channel_id]->AddTimeMeasure(tdc.value);
    }
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
    return (int)parser->GetEventNumber();
}

void PRadDataHandler::PrintOutEPICS()
{
    for(auto &epics_ch : epics_channels)
    {
        cout << epics_ch.first << ": "
             << epics_ch.second.back().value
             << endl;
    }
}

void PRadDataHandler::PrintOutEPICS(const string &name)
{
    auto it = epics_channels.find(name);
    if(it == epics_channels.end()) {
        cout << "Did not find the EPICS channel "
             << name << endl;
        return;
    }

    auto channel_data = it->second;

    cout << "Channel " << name << " data are: " << endl;
    for(auto &data : channel_data)
    {
        cout << data.att_event << "  " << data.value << endl;
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

        pedHist->Fit("gaus", "qw");

        TF1 *myfit = (TF1*) pedHist->GetFunction("gaus");
        double p0 = myfit->GetParameter(1);
        double p1 = myfit->GetParameter(2);

        channel->UpdatePedestal(p0, p1);
    }
}

void PRadDataHandler::CorrectGainFactor(const int &run, const int &ref)
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

    if(run >= ALPHA_CORR)
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
        if(!c_parser.NbofElements())
            continue; // comment

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
        if(!c_parser.NbofElements())
            continue;

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
}

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
        if(!c_parser.NbofElements())
            continue;

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
        if(!c_parser.NbofElements())
            continue;

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
        if(!c_parser.NbofElements())
            continue;

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
            if(channelList[adc.channel_id]->IsHyCalModule())
                ene += channelList[adc.channel_id]->Calibration(adc.value);
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
            cerr << "Cannot open input file " << path
                 << ", stop reading!" << endl;
            return;
        }

        input.seekg(0, input.end);
        int length = input.tellg();
        input.seekg(0, input.beg);

        while(input.tellg() < length && input.tellg() != -1)
        {
            EventData event;
            ReadFromDST(input, event);
            energyData.push_back(event);
        }

    } catch(PRadException &e) {
        cerr << e.FailureType() << ": "
             << e.FailureDesc() << endl
             << "Write to DST Aborted!" << endl;
    } catch(exception &e) {
        cerr << e.what() << endl
             << "Write to DST Aborted!" << endl;
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
     dst_file.read((char*) &data.latch_word  , sizeof(data.latch_word));
     dst_file.read((char*) &data.lms_phase   , sizeof(data.lms_phase));
     dst_file.read((char*) &data.timestamp   , sizeof(data.timestamp));

     size_t adc_size, tdc_size, gem_size;
     unsigned short channel_id, value;
     unsigned char fec, adc, strip;

     dst_file.read((char*) &adc_size, sizeof(adc_size));
     for(size_t i = 0; i < adc_size; ++i)
     {
         dst_file.read((char*) &channel_id, sizeof(channel_id));
         dst_file.read((char*) &value     , sizeof(value));
         data.add_adc(ADC_Data(channel_id, value));
     }
    
     dst_file.read((char*) &tdc_size, sizeof(tdc_size));
     for(size_t i = 0; i < tdc_size; ++i)
     {
         dst_file.read((char*) &channel_id, sizeof(channel_id));
         dst_file.read((char*) &value     , sizeof(value));
         data.add_tdc(TDC_Data(channel_id, value));
     }

     dst_file.read((char*) &gem_size, sizeof(gem_size));
     for(size_t i = 0; i < gem_size; ++i)
     {
         dst_file.read((char*) &fec  , sizeof(fec));
         dst_file.read((char*) &adc  , sizeof(adc));
         dst_file.read((char*) &strip, sizeof(strip));
         dst_file.read((char*) &value, sizeof(value));
         data.add_gem(GEM_Data(fec, adc, strip, value));
     }
 }

void PRadDataHandler::WriteToDST(const string &path, ios::openmode mode)
{
    ofstream output;

    try {
        output.open(path, mode);

        if(!output.is_open()) {
            cerr << "Cannot open output file " << path
                 << ", stop writing!" << endl;
            return;
        }

        for(auto &event : energyData)
        {
            WriteToDST(output, event);
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

     // event information
     dst_file.write((char*) &data.event_number, sizeof(data.event_number));
     dst_file.write((char*) &data.type        , sizeof(data.type));
     dst_file.write((char*) &data.latch_word  , sizeof(data.latch_word));
     dst_file.write((char*) &data.lms_phase   , sizeof(data.lms_phase));
     dst_file.write((char*) &data.timestamp   , sizeof(data.timestamp));

     // adc data bank
     size_t adc_size = data.adc_data.size();
     size_t tdc_size = data.tdc_data.size();
     size_t gem_size = data.gem_data.size();

     dst_file.write((char*) &adc_size, sizeof(adc_size));
     for(auto &adc : data.adc_data)
     {
         dst_file.write((char*) &adc.channel_id, sizeof(adc.channel_id));
         dst_file.write((char*) &adc.value     , sizeof(adc.value));
     }

     dst_file.write((char*) &tdc_size, sizeof(tdc_size));
     for(auto &tdc : data.tdc_data)
     {
         dst_file.write((char*) &tdc.channel_id, sizeof(tdc.channel_id));
         dst_file.write((char*) &tdc.value     , sizeof(tdc.value));
     }
 
     dst_file.write((char*) &gem_size, sizeof(gem_size));
     for(auto &gem : data.gem_data)
     {
         dst_file.write((char*) &gem.fec  , sizeof(gem.fec));
         dst_file.write((char*) &gem.adc  , sizeof(gem.adc));
         dst_file.write((char*) &gem.strip, sizeof(gem.strip));
         dst_file.write((char*) &gem.value, sizeof(gem.value));
     }
}

void PRadDataHandler::ReadFromEvio(const string &path)
{
    parser->ReadEvioFile(path.c_str());
}

void PRadDataHandler::InitializeByData(const string &path, int run)
{
    if(!path.empty()) {
        // auto update run number
        if(run < 0)
            run = GetRunNumberFromFileName(path);

        cout << "Data Handler: Initializing from data file "
             << "\"" << path << "\", "
             << "run number: " << run
             << " ." << endl;

         parser->ReadEvioFile(path.c_str());
    }

    FitPedestal();

    CorrectGainFactor(run);

    Clear();

    cout << "Data Handler: Channel Pedestal and Gain Factors are adjusted according to data." << endl;
}

int PRadDataHandler::GetRunNumberFromFileName(const string &name, const size_t &pos, const bool &verbose)
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

    vector<int> numbers = ConfigParser::find_integer(name.substr(nameBeg, nameEnd - nameBeg + 1));

    if(numbers.size() > pos) {

        if(verbose) {
            cout << "Data Handler: Run number is automatcially determined from file name."
                 << endl
                 << "File name: " << name
                 << endl
                 << "Run number: " << numbers.at(pos)
                 << endl;
        }

        return numbers.at(pos);
    }

    return 0;
}
