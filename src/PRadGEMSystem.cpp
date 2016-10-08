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
#include "TFile.h"
#include "TH1.h"

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
    det_plane_map.clear();
    fec_list.clear();
    fec_map.clear();
    apv_map.clear();
}

void PRadGEMSystem::LoadConfiguration(const string &path) throw(PRadException)
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

            string readout, detector_type, name;
            c_parser >> readout >> detector_type >> name;
            PRadGEMDetector *new_det = new PRadGEMDetector(readout, detector_type, name);

            if(readout == "CARTESIAN") {

                string plane_x, plane_y;
                float size_x, size_y;
                int connect_x, connect_y, orient_x, orient_y;

                c_parser >> plane_x >> size_x >> connect_x >> orient_x
                         >> plane_y >> size_y >> connect_y >> orient_y;

                new_det->AddPlane(PRadGEMDetector::Plane_X, plane_x, size_x, connect_x, orient_x);
                new_det->AddPlane(PRadGEMDetector::Plane_Y, plane_y, size_y, connect_y, orient_y);

                RegisterDetector(new_det);

            } else {

                cerr << "GEM System: Unsupported detector readout type "
                     << readout << endl;
                delete new_det;

            }

        } else if(type == "FEC") {

            int fec_id;
            string fec_ip;

            c_parser >> fec_id >> fec_ip;

            PRadGEMFEC *new_fec = new PRadGEMFEC(fec_id, fec_ip);
            RegisterFEC(new_fec);

        } else if(type == "APV") {

            string det_plane, status;
            int fec_id, adc_ch, orient, index;
            unsigned short header;

            c_parser >> fec_id >> adc_ch >> det_plane >> orient >> index >> header >> status;

            PRadGEMAPV *new_apv = new PRadGEMAPV(fec_id, adc_ch, orient, index, header, status);

            // default levels
            new_apv->SetTimeSample(3);
            new_apv->SetCommonModeThresLevel(20.);
            new_apv->SetZeroSupThresLevel(5.);

            RegisterAPV(det_plane, new_apv);

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

void PRadGEMSystem::LoadPedestal(const string &path) throw(PRadException)
{
    ConfigParser c_parser;
    c_parser.SetSplitters(",: \t");

    if(!c_parser.OpenFile(path)) {
        throw PRadException("GEM System", "cannot open configuration file " + path);
    }

    PRadGEMAPV *apv = nullptr;

    while(c_parser.ParseLine())
    {
        ConfigValue first = c_parser.TakeFirst();

        if(first == "APV") { // a new APV

            int fec, adc;
            c_parser >> fec >> adc;
            apv = GetAPV(fec, adc);

        } else { // different adc channel in this APV

            float offset, noise;
            c_parser >> offset >> noise;

            if(apv)
                apv->UpdatePedestal(PRadGEMAPV::Pedestal(offset, noise), first.Int());

        }
    }
}

void PRadGEMSystem::RegisterDetector(PRadGEMDetector *det)
{
    if(det == nullptr)
        return;

    if(det_map_name.find(det->name) != det_map_name.end())
    {
        cerr << "GEM System Error: Detector "
             << det->name << " exists, "
             << " abort detector registration."
             << endl;
        return;
    }

    det_map_name[det->name] = det;

    // register planes
    auto planes = det->GetPlaneList();

    for(auto &plane : planes)
    {
        if(det_plane_map.find(plane->name) != det_plane_map.end())
        {
            cerr << "GEM System Error: Detector Plane "
                 << plane->name << " has been registered, "
                 << "please make sure different planes are assigned "
                 << "different names."
                 << endl;
            continue;
        }

        det_plane_map[plane->name] = plane;
    }

    det->AssignID(det_list.size());
    det_list.push_back(det);
}

void PRadGEMSystem::RegisterFEC(PRadGEMFEC *fec)
{
    if(fec == nullptr)
        return;

    fec_list.push_back(fec);
    fec_map[fec->id] = fec;
}

void PRadGEMSystem::RegisterAPV(const string &plane_name, PRadGEMAPV *apv)
{
    if(apv == nullptr)
        return;

    // find detector plane that connects to this apv
    auto *det_plane = GetDetectorPlane(plane_name);
    if(det_plane == nullptr) {
        cerr << "GEM System Error: Cannot connect apv to detector plane "
             << plane_name << ", make sure you have detectors defined "
             << "before the apv list in configuration map."
             << endl;
        return;
    }
    det_plane->ConnectAPV(apv);


    PRadGEMFEC *fec = GetFEC(apv->fec_id);

    if(fec == nullptr) {
        cerr << "GEM System Error: Cannot add apv to fec "
             << apv->fec_id << ", make sure you have fec defined "
             << "before the aplv list in configuration map."
             << endl;
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

PRadGEMDetector *PRadGEMSystem::GetDetector(const int &id)
{
    if((unsigned int) id >= det_list.size()) {
        cerr << "GEM System: Cannot find detector with id " << id << endl;
        return nullptr;
    }
    return det_list[id];
}

PRadGEMDetector *PRadGEMSystem::GetDetector(const string &name)
{
    auto it = det_map_name.find(name);
    if(it == det_map_name.end()) {
        cerr << "GEM System: Cannot find detector " << name << endl;
        return nullptr;
    }
    return it->second;
}

PRadGEMDetector::Plane *PRadGEMSystem::GetDetectorPlane(const string &plane)
{
    auto it = det_plane_map.find(plane);
    if(it == det_plane_map.end()) {
        cerr << "GEM System: Cannot find plane " << plane << endl;
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

void PRadGEMSystem::SavePedestal(const string &name)
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

