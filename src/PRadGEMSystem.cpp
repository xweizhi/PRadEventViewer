//============================================================================//
// GEM DAQ System class                                                       //
// Algorithm is based on the code from Kondo Gnanvo and Xinzhan Bai           //
//                                                                            //
// Chao Peng                                                                  //
// 06/17/2016                                                                 //
//============================================================================//

#include "PRadGEMSystem.h"
#include "ConfigParser.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "TH1.h"
#include "TF1.h"
#include "TList.h"
#include "TFile.h"

using namespace std;

PRadGEMSystem::PRadGEMSystem()
{

}

PRadGEMSystem::~PRadGEMSystem()
{
    Clear();
}

void PRadGEMSystem::Clear()
{
    for(auto &det : det_list)
    {
        delete det;
    }

    for(auto &fec : fec_list)
    {
        delete fec;
    }

    det_list.clear();
    det_map_name.clear();
    det_map_plane_x.clear();
    det_map_plane_y.clear();
    fec_list.clear();
    fec_map.clear();
    apv_map.clear();
}

void PRadGEMSystem::LoadConfiguration(const std::string &path) throw(PRadException)
{
    ConfigParser c_parser;
    c_parser.SetSplitters(",");

    if(!c_parser.OpenFile(path)) {
        throw PRadException("GEM System", "cannot open configuration file " + path);
    }

    while(c_parser.ParseLine())
    {
        string type = c_parser.TakeFirst();
        if(type == "DET") {
            string readout = c_parser.TakeFirst();
            string detector_type = c_parser.TakeFirst();
            string name = c_parser.TakeFirst();
            PRadGEMDET *new_det = new PRadGEMDET(readout, detector_type, name);

            if(readout == "CARTESIAN") {
                string plane_x = c_parser.TakeFirst();
                float size_x = c_parser.TakeFirst().Double();
                int connect_x = c_parser.TakeFirst().Int();
                int orient_x = c_parser.TakeFirst().Int();

                string plane_y = c_parser.TakeFirst();
                float size_y = c_parser.TakeFirst().Float();
                int connect_y = c_parser.TakeFirst().Int();
                int orient_y = c_parser.TakeFirst().Int();

                new_det->AddPlane(PRadGEMDET::Plane_X, PRadGEMDET::Plane(plane_x, size_x, connect_x, orient_x));
                new_det->AddPlane(PRadGEMDET::Plane_Y, PRadGEMDET::Plane(plane_y, size_y, connect_y, orient_y));

                RegisterDET(new_det);
            } else {
                cerr << "GEM System: Unsupported detector readout type " << readout << endl;
                delete new_det;
            }
        } else if(type == "FEC") {
            int fec_id = c_parser.TakeFirst().Int();
            string fec_ip = c_parser.TakeFirst();

            PRadGEMFEC *new_fec = new PRadGEMFEC(fec_id, fec_ip);
            RegisterFEC(new_fec);
        } else if(type == "APV") {
            int fec_id = c_parser.TakeFirst().Int();
            int adc_ch = c_parser.TakeFirst().Int();
            string det_plane = c_parser.TakeFirst();
            int orient = c_parser.TakeFirst().Int();
            int index = c_parser.TakeFirst().Int();
            unsigned short header = c_parser.TakeFirst().UShort();
            string status = c_parser.TakeFirst();

            PRadGEMAPV *new_apv = new PRadGEMAPV(det_plane, fec_id, adc_ch, orient, index, header, status);

            // default levels
            new_apv->SetTimeSample(3);
            new_apv->SetCommonModeThresLevel(20.);
            new_apv->SetZeroSupThresLevel(6.);

            RegisterAPV(new_apv);
        }
    }

    SortFECList();
}

void PRadGEMSystem::SortFECList()
{
    sort(fec_list.begin(), fec_list.end(), [this](PRadGEMFEC *fec1, PRadGEMFEC *fec2){return fec1->id < fec2->id;});

    for(auto &fec : fec_list)
    {
        fec->SortAPVList();
    }
}

void PRadGEMSystem::LoadPedestal(const string &path)
{
    ConfigParser c_parser;
    c_parser.SetSplitters(",:");

    if(!c_parser.OpenFile(path)) {
        throw PRadException("GEM System", "cannot open configuration file " + path);
    }

    PRadGEMAPV *apv = nullptr;

    while(c_parser.ParseLine())
    {
        ConfigValue first = c_parser.TakeFirst();
        if(first == "FEC") {
            int fec = c_parser.TakeFirst().Int();
            string second = c_parser.TakeFirst();
            int adc = c_parser.TakeFirst().Int();
            apv = GetAPV(fec, adc);
        } else {
            float offset = c_parser.TakeFirst().Float();
            float noise = c_parser.TakeFirst().Float();
            if(apv)
                apv->UpdatePedestal(PRadGEMAPV::Pedestal(offset, noise), first.Int());
        }
    } 
}

void PRadGEMSystem::RegisterDET(PRadGEMDET *det)
{
    if(det == nullptr)
        return;

    det->AssignID(det_list.size());
    det_list.push_back(det);

    det_map_name[det->name] = det;
    det_map_plane_x[det->GetPlaneX().name] = det;
    det_map_plane_y[det->GetPlaneY().name] = det;
}

void PRadGEMSystem::RegisterFEC(PRadGEMFEC *fec)
{
    if(fec == nullptr)
        return;

    fec_list.push_back(fec);
    fec_map[fec->id] = fec;
}

void PRadGEMSystem::RegisterAPV(PRadGEMAPV *apv)
{
    if(apv == nullptr)
        return;

    PRadGEMFEC *fec = GetFEC(apv->fec_id);

    if(fec == nullptr) {
        cerr << "GEM System: Cannot add apv to fec, make sure you have fec defined in configuration map first." << endl;
        return;
    }

    fec->AddAPV(apv);

    GEMChannelAddress addr(apv->fec_id, apv->adc_ch);

    apv_map[addr] = apv;
}

void PRadGEMSystem::BuildAPVMap()
{
    apv_map.clear();

    for(auto &fec : fec_list)
    {
        vector<PRadGEMAPV *> apv_list = fec->GetAPVList();
        for(auto &apv : apv_list)
        {
            GEMChannelAddress addr(apv->fec_id, apv->adc_ch);
            apv_map[addr] = apv;
        }
    }
}

PRadGEMDET *PRadGEMSystem::GetDetector(const int &id)
{
    if((unsigned int) id >= det_list.size()) {
        cerr << "GEM System: Cannot find detector with id " << id << endl;
        return nullptr;
    }
    return det_list[id];
}

PRadGEMDET *PRadGEMSystem::GetDetector(const std::string &name)
{
    auto it = det_map_name.find(name);
    if(it == det_map_name.end()) {
        cerr << "GEM System: Cannot find detector with name " << name << endl;
        return nullptr;
    }
    return it->second;
}

PRadGEMDET *PRadGEMSystem::GetDetectorByPlaneX(const std::string &plane_x)
{
    auto it = det_map_plane_x.find(plane_x);
    if(it == det_map_plane_x.end()) {
        cerr << "GEM System: Cannot find detector with plane " << plane_x << endl;
        return nullptr;
    }
    return it->second;
}

PRadGEMDET *PRadGEMSystem::GetDetectorByPlaneY(const std::string &plane_y)
{
    auto it = det_map_plane_y.find(plane_y);
    if(it == det_map_plane_y.end()) {
        cerr << "GEM System: Cannot find detector with plane " << plane_y << endl;
        return nullptr;
    }
    return it->second;
}

PRadGEMFEC *PRadGEMSystem::GetFEC(const int &id)
{
    auto it = fec_map.find(id);
    if(it == fec_map.end()) {
        cerr << "GEM System: Cannot find FEC with id " << id << endl;
        return nullptr;
    }
    return it->second;
}

PRadGEMAPV *PRadGEMSystem::GetAPV(const int &fec_id, const int &apv_id)
{
    return GetAPV(GEMChannelAddress(fec_id, apv_id));
}

PRadGEMAPV *PRadGEMSystem::GetAPV(const GEMChannelAddress &addr)
{
    auto it = apv_map.find(addr);
    if(it == apv_map.end()) {
        cerr << "GEM System: Cannot find APV with id " << addr.adc_ch
             << " in FEC " << addr.fec_id << endl;
        return nullptr;
    }
    return it->second;
}

void PRadGEMSystem::ClearAPVData()
{
    for(auto &fec : fec_list)
    {
        fec->ClearAPVData();
    }
}

void PRadGEMSystem::FillRawData(GEMRawData &raw, vector<GEM_Data> &container, const bool &fill_hist)
{
    auto it = apv_map.find(raw.addr);
    if(it != apv_map.end())
    {
        PRadGEMAPV *apv = it->second;
        apv->FillRawData(raw.buf, raw.size);

        if(fill_hist) {
            if(PedestalMode)
                apv->FillPedHist();
        } else {
            apv->ZeroSuppression();
#ifdef MULTI_THREAD
            locker.lock();
#endif
            apv->CollectZeroSupHits(container);
#ifdef MULTI_THREAD
            locker.unlock();
#endif
        }
    }
}

void PRadGEMSystem::FitPedestal()
{
    for(auto &fec : fec_list)
    {
        fec->FitPedestal();
    }
}

void PRadGEMSystem::SavePedestal(const std::string &name)
{
    ofstream in_file(name);

    if(!in_file.is_open()) {
        cerr << "GEM System: Failed to save pedestal, file "
             << name << " cannot be opened."
             << endl;
    }

    for(auto &fec : fec_list)
    {
        for(auto &apv : fec->GetAPVList())
        {
            apv->PrintOutPedestal(in_file);
        }
    }
}

void PRadGEMSystem::SetPedestalMode(const bool &m)
{
    PedestalMode = m;

    for(auto &apv_it : apv_map)
    {
        if(m)
            apv_it.second->CreatePedHist();
        else
            apv_it.second->ReleasePedHist();
    }
}

vector<GEM_Data> PRadGEMSystem::GetZeroSupData()
{
    vector<GEM_Data> gem_data;
    for(auto &apv_it : apv_map)
    {
        apv_it.second->CollectZeroSupHits(gem_data);
    }

    return gem_data;
}

void PRadGEMSystem::SetUnivCommonModeThresLevel(const float &thres)
{
    for(auto &apv_it : apv_map)
    {
        apv_it.second->SetCommonModeThresLevel(thres);
    }
}

void PRadGEMSystem::SetUnivZeroSupThresLevel(const float &thres)
{
    for(auto &apv_it : apv_map)
    {
        apv_it.second->SetZeroSupThresLevel(thres);
    }
}

void PRadGEMSystem::SetUnivTimeSample(const size_t &ts)
{
    for(auto &apv_it : apv_map)
    {
        apv_it.second->SetTimeSample(ts);
    }
}

void PRadGEMSystem::SaveHistograms(const string &path)
{
    TFile *f = new TFile(path.c_str(), "recreate");

    for(auto fec : fec_list)
    {
        string fec_name = "FEC " + to_string(fec->id);
        TDirectory *cdtof = f->mkdir(fec_name.c_str());
        cdtof->cd();

        for(auto apv : fec->GetAPVList())
        {
            string adc_name = "ADC " + to_string(apv->adc_ch);
            TDirectory *cur_dir = cdtof->mkdir(adc_name.c_str());
            cur_dir->cd();

            for(auto hist : apv->GetHistList())
                hist->Write();
        }
    }

    f->Close();
    delete f;
}

vector<PRadGEMAPV *> PRadGEMSystem::GetAPVList()
{
    vector<PRadGEMAPV *> list;
    for(auto &fec : fec_list)
    {
        vector<PRadGEMAPV *> sub_list = fec->GetAPVList();
        list.insert(list.end(), sub_list.begin(), sub_list.end());
    }

    return list;
}

//===========================================================================//
//   GEM FEC                                                                 //
//===========================================================================//

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
        cerr << "GEM FEC " << id
             << ": Abort to add existing apv to adc channel "
             << apv->adc_ch << endl;
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
        cerr << "GEM FEC " << id
             << ": Cannot find APV at adc channel " << id
             << endl;
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

void PRadGEMFEC::FitPedestal()
{
    for(auto &adc : adc_list)
    {
        adc->FitPedestal();
    }
}

//===========================================================================//
//   GEM APV                                                                 //
//===========================================================================//

PRadGEMAPV::PRadGEMAPV(const std::string &p,
                       const int &f,
                       const int &ch,
                       const int &o,
                       const int &idx,
                       const int &hl,
                       const std::string &s)
: plane(p), fec_id(f), adc_ch(ch), orient(o),
  plane_index(idx), header_level(hl), status(s)
{
#define DEFAULT_MAX_CHANNEL 550
    // initialize
    buffer_size = DEFAULT_MAX_CHANNEL;
    time_samples = 3;
    raw_data = new float[buffer_size];

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        offset_hist[i] = nullptr;
        noise_hist[i] = nullptr;
    }

    if(status.find("split") != string::npos)
        split = true;
    else
        split = false;

    BuildStripMap();

    ClearData();
}

PRadGEMAPV::~PRadGEMAPV()
{
    delete[] raw_data;
}

void PRadGEMAPV::CreatePedHist()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(offset_hist[i] == nullptr) {
            string name = "CH_" + to_string(i) + "_OFFSET_" + to_string(fec_id) + "_" + to_string(adc_ch);
            offset_hist[i] = new TH1I(name.c_str(), "Pedestal", 500, 2000, 3500);
        }
        if(noise_hist[i] == nullptr) {
            string name = "CH_" + to_string(i) + "_NOISE_" + to_string(fec_id) + "_" + to_string(adc_ch);
            noise_hist[i] = new TH1I(name.c_str(), "Noise", 400, -200, 200); 
        }
    }
}

void PRadGEMAPV::ReleasePedHist()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(offset_hist != nullptr) {
            delete offset_hist[i], offset_hist[i] = nullptr;
        }
        if(noise_hist != nullptr) {
            delete noise_hist[i], noise_hist[i] = nullptr;
        }
    }
}

void PRadGEMAPV::SetTimeSample(const size_t &t)
{
#define APV_EXTEND_SIZE 166 //TODO, arbitrary number, need to know exact buffer size the apv need
    time_samples = t;
    buffer_size = t*TIME_SAMPLE_SIZE + APV_EXTEND_SIZE;

    // reallocate the memory for proper size
    delete[] raw_data;

    raw_data = new float[buffer_size];

    ClearData();
}

void PRadGEMAPV::ClearData()
{
    for(size_t i = 0; i < buffer_size; ++i)
        raw_data[i] = 5000.;

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
        hit_pos[i] = false;
}

void PRadGEMAPV::ClearPedestal()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
        pedestal[i] = Pedestal(0, 0);
}

void PRadGEMAPV::UpdatePedestal(vector<Pedestal> &ped)
{
    for(size_t i = 0; (i < ped.size()) && (i < TIME_SAMPLE_SIZE); ++i)
        pedestal[i] = ped[i];
}

void PRadGEMAPV::UpdatePedestal(const Pedestal &ped, const size_t &index)
{
    if(index >= TIME_SAMPLE_SIZE)
        return;

    pedestal[index] = ped;
}

void PRadGEMAPV::UpdatePedestal(const float &offset, const float &noise, const size_t &index)
{
    if(index >= TIME_SAMPLE_SIZE)
        return;

    pedestal[index].offset = offset;
    pedestal[index].noise = noise;
}

void PRadGEMAPV::FillRawData(const uint32_t *buf, const size_t &size)
{
    if(2*size > buffer_size) {
        cerr << "Received " << size * 2 << " adc words, "
             << "but APV " << adc_ch << " in FEC " << fec_id
             << " has only " << buffer_size << " channels" << endl;
        return;
    }

    for(size_t i = 0; i < size; ++i)
    {
        SplitData(buf[i], raw_data[2*i], raw_data[2*i+1]);
    }

    ts_index = GetTimeSampleStart();
}

void PRadGEMAPV::SplitData(const uint32_t &data, float &word1, float &word2)
{
    int data1 = (((data>>16)&0xff)<<8) | (data>>24);
    int data2 = ((data&0xff)<<8) | ((data>>8)&0xff);
    word1 = (float)data1;
    word2 = (float)data2;
}

void PRadGEMAPV::FillPedHist()
{
    float average[2][3];

    for(size_t i = 0; i < time_samples; ++i)
    {
        if(split) {
            GetAverage(average[0][i], &raw_data[ts_index + i*TIME_SAMPLE_DIFF], 1);
            GetAverage(average[1][i], &raw_data[ts_index + i*TIME_SAMPLE_DIFF], 2);
        } else {
            GetAverage(average[0][i], &raw_data[ts_index + i*TIME_SAMPLE_DIFF]);
        }
    }

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        float ch_average = 0.;
        float noise_average = 0.;
        for(size_t j = 0; j < time_samples; ++j)
        {
            ch_average += raw_data[i + ts_index + j*TIME_SAMPLE_DIFF];
            if(split) {
                if(strip_map[i] < 16)
                    noise_average += raw_data[i + ts_index + j*TIME_SAMPLE_DIFF] - average[0][j];
                else
                    noise_average += raw_data[i + ts_index + j*TIME_SAMPLE_DIFF] - average[1][j];
            } else {
                noise_average += raw_data[i + ts_index + j*TIME_SAMPLE_DIFF] - average[0][j];
            }
        }

        if(offset_hist[i])
            offset_hist[i]->Fill(ch_average/time_samples);

        if(noise_hist[i])
            noise_hist[i]->Fill(noise_average/time_samples);
    }
}

void PRadGEMAPV::GetAverage(float &average, const float *buf, const size_t &set)
{
    average = 0.;
    int count = 0;

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if((set == 0) ||
           (set == 1 && strip_map[i] < 16) ||
           (set == 2 && strip_map[i] >= 16))
        {
            average += buf[i];
            count++;
        }
    }

    average /= (float)count;
}

void PRadGEMAPV::FitPedestal()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if( (offset_hist[i] == nullptr) ||
            (noise_hist[i] == nullptr) ||
            (offset_hist[i]->Integral() < 1000) ||
            (noise_hist[i]->Integral() < 1000) )
            continue;

        offset_hist[i]->Fit("gaus", "qww");
        noise_hist[i]->Fit("gaus", "qww");
        TF1 *myfit = (TF1*) offset_hist[i]->GetFunction("gaus");
        double p0 = myfit->GetParameter(1);
        myfit = (TF1*) noise_hist[i]->GetFunction("gaus");
        double p1 = myfit->GetParameter(2);
//        double p0 = ped_hist[i]->GetMean();
//        double p1 = ped_hist[i]->GetRMS();
        UpdatePedestal((float)p0, (float)p1, i);
    }
}

size_t PRadGEMAPV::GetTimeSampleStart()
{
    for(size_t i = 2; i < buffer_size; ++i)
    { 
        if( (raw_data[i]   < header_level) &&
            (raw_data[i-1] < header_level) &&
            (raw_data[i-2] < header_level) )
            return i + 10;
    }

    return buffer_size;
}

void PRadGEMAPV::ZeroSuppression()
{
    // common mode correction
    for(size_t ts = 0; ts < time_samples; ++ts)
    {
        if(split)
            CommonModeCorrection_Split(&raw_data[ts_index + ts*TIME_SAMPLE_DIFF], TIME_SAMPLE_SIZE);
        else
            CommonModeCorrection(&raw_data[ts_index + ts*TIME_SAMPLE_DIFF], TIME_SAMPLE_SIZE);
    }

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        float average = 0.;
        for(size_t j = 0; j < time_samples; ++j)
        {
            average += raw_data[i + ts_index + j*TIME_SAMPLE_DIFF];
        }
        average /= time_samples;

        if(average > pedestal[i].noise * zerosup_thres)
            hit_pos[i] = true;
        else
            hit_pos[i] = false;
    }
}

void PRadGEMAPV::CollectZeroSupHits(vector<GEM_Data> &hits)
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(hit_pos[i] == false)
            continue;

        GEM_Data hit(fec_id, adc_ch, i);
        for(size_t j = 0; j < time_samples; ++j)
        {
            hit.add_value(raw_data[i + ts_index + j*TIME_SAMPLE_DIFF]);
        }
        hits.push_back(hit);
    }
}

void PRadGEMAPV::CommonModeCorrection(float *buf, const size_t &size)
{
    int count = 0;
    float average = 0;

    for(size_t i = 0; i < size; ++i)
    {
        buf[i] = pedestal[i].offset - buf[i];

        if(buf[i] < pedestal[i].noise * common_thres) {
            average += buf[i];
            count++;
        }
    }

    if(count)
        average /= (float)count;

    for(size_t i = 0; i < size; ++i)
    {
        buf[i] -= average;
    }
}

void PRadGEMAPV::CommonModeCorrection_Split(float *buf, const size_t &size)
{
    int count1 = 0, count2 = 0;
    float average1 = 0, average2 = 0;

    for(size_t i = 0; i < size; ++i)
    {
        buf[i] = pedestal[i].offset - buf[i];
        if(strip_map[i] < 16) {
            if(buf[i] < pedestal[i].noise * common_thres * 10.) {
                average1 += buf[i];
                count1++;
            }
        } else {
            if(buf[i] < pedestal[i].noise * common_thres) {
                average2 += buf[i];
                count2++;
            }
        }
    }

    if(count1)
        average1 /= (float)count1;
    if(count2)
        average2 /= (float)count2;

    for(size_t i = 0; i < size; ++i)
    {
        if(strip_map[i] < 16)
            buf[i] -= average1;
        else
            buf[i] -= average2;
    }
}

void PRadGEMAPV::BuildStripMap()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        strip_map[i] = (unsigned char)MapStrip((int)i);
    }
}

int PRadGEMAPV::GetStrip(const size_t &ch)
{
   if(ch >= TIME_SAMPLE_SIZE) {
       cerr << "GEM APV: APV " << adc_ch
            << " in FEC " << fec_id
            << " only has " << TIME_SAMPLE_SIZE
            << " channels." << endl;
   }

   return (int)strip_map[ch];
}

int PRadGEMAPV::MapStrip(const int &ch)
{
    // APV25 Internal Channel Mapping
    int strip = 32*(ch%4) + 8*(ch/4) - 31*(ch/16);

    // APV25 Channel to readout strip Mapping
    if((plane.find("X") != string::npos) && (plane_index == 11)) {
        if(strip & 1)
            strip = 48 - (strip + 1)/2;
        else 
            strip = 48 + strip/2;
    } else {
        if(strip & 1)
            strip = 32 - (strip + 1)/2;
        else
            strip = 32 + strip/2;
    }

    strip &= 0x7f;

    return strip;
}

void PRadGEMAPV::PrintOutPedestal(ofstream &out)
{
    out << "APV address: "
        << setw(12) << fec_id
        << setw(12) << adc_ch
        << endl;

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        out << setw(12) << i
            << setw(12) << pedestal[i].offset
            << setw(12) << pedestal[i].noise
            << endl;
    }
}

vector<TH1I *> PRadGEMAPV::GetHistList()
{
    vector<TH1I *> hist_list;

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(offset_hist[i])
            hist_list.push_back(offset_hist[i]);
        if(noise_hist[i])
            hist_list.push_back(noise_hist[i]);
    }

    return hist_list;
}

vector<PRadGEMAPV::Pedestal> PRadGEMAPV::GetPedestalList()
{
    vector<Pedestal> ped_list;
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        ped_list.push_back(pedestal[i]);
    }

    return ped_list;
}
