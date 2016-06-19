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
#include <thread>

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
        delete det, det = nullptr;
    }

    for(auto &fec_it : fec_map)
    {
        delete fec_it.second, fec_it.second = nullptr;
    }

    det_list.clear();
    det_map_name.clear();
    det_map_plane_x.clear();
    det_map_plane_y.clear();
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
            new_apv->SetZeroSupThresLevel(5.);

            RegisterAPV(new_apv);
        }
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

    for(auto &fec_it : fec_map)
    {
        vector<PRadGEMAPV *> apv_list = fec_it.second->GetAPVList();
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
    auto it = apv_map.find(GEMChannelAddress(fec_id, apv_id));
    if(it == apv_map.end()) {
        cerr << "GEM System: Cannot find APV with id " << apv_id
             << " in FEC " << fec_id << endl;
        return nullptr;
    }
    return it->second;
}

void PRadGEMSystem::ClearAPVData()
{
    for(auto &fec_it : fec_map)
    {
        fec_it.second->ClearAPVData();
    }
}

vector<GEM_Data> PRadGEMSystem::GetZeroSupData()
{
    vector<GEM_Data> gem_data;
    for(auto &apv_it : apv_map)
    {
        apv_it.second->CollectZeroSupHits(gem_data);
    }
/*
    vector<thread> bank_threads;

    for(auto &fec_it : fec_map)
    {
        bank_threads.push_back(thread(&PRadGEMFEC::ZeroSuppression, fec_it.second));
    }

    for(auto &thread : bank_threads)
    {
        if(thread.joinable()) thread.join();
    }

    for(auto &fec_it : fec_map)
    {
        vector<GEM_Data> fec_hits = fec_it.second->GetZeroSupHits();
        if(fec_hits.size()) {
            gem_data.insert(gem_data.end(), fec_hits.begin(), fec_hits.end());
        }
    }
*/
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

void PRadGEMFEC::ZeroSuppression()
{
    hits_collection.clear();

    for(auto &adc : adc_list)
    {
        adc->CollectZeroSupHits(hits_collection);
    }
}

vector<GEM_Data> &PRadGEMFEC::GetZeroSupHits()
{
    return hits_collection;
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
    // should calculate this according to time_sample
    buffer_size = DEFAULT_MAX_CHANNEL;
    time_samples = 3;
    raw_data = new float[buffer_size];

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
        raw_data[i] = 0;
}

void PRadGEMAPV::ClearPedestal()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
        pedestal[i] = Pedestal(0, 0);
}

void PRadGEMAPV::UpdatePedestalArray(Pedestal *ped, const size_t &size)
{
    for(size_t i = 0; i < size; ++i)
        pedestal[i] = ped[i];
}

void PRadGEMAPV::UpdatePedestal(const Pedestal &ped, const size_t &index)
{
    pedestal[index] = ped;
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
}

void PRadGEMAPV::SplitData(const uint32_t &data, float &word1, float &word2)
{
    int data1 = (((data>>16)&0xff)<<8) | (data>>24);
    int data2 = ((data&0xff)<<8) | ((data>>8)&0xff);
    word1 = (float)data1;
    word2 = (float)data2;
}

void PRadGEMAPV::CollectZeroSupHits(vector<GEM_Data> &hits)
{
    size_t sample_diff = TIME_SAMPLE_SIZE + APV_HEADER_SIZE;
    size_t data_size = time_samples * sample_diff  - APV_HEADER_SIZE;

    for(size_t i = 2; i < buffer_size; ++i)
    { 
       if( (raw_data[i]   < header_level) &&
           (raw_data[i-1] < header_level) &&
           (raw_data[i-2] < header_level) )
       {
           ts_index = i + 10;
           if(ts_index + data_size >= buffer_size) {
               cerr << "GEM APV: FEC " << fec_id
                    << ", APV " << adc_ch << " Warning, get time samples ends at index "
                    << ts_index + data_size
                    << ", it exceeds the buffer size " << buffer_size
                    << endl;
                    return;
           }
           break;
        }
    }

    for(size_t ts = 0; ts < time_samples; ++ts)
    {
        if(split)
            CommonModeCorrection_Split(&raw_data[ts_index + sample_diff*ts], TIME_SAMPLE_SIZE);
        else
            CommonModeCorrection(&raw_data[ts_index + sample_diff*ts], TIME_SAMPLE_SIZE);
    }

    ZeroSuppression(hits, &raw_data[ts_index], sample_diff); 
}

void PRadGEMAPV::ZeroSuppression(vector<GEM_Data> &hits, float *buf, const size_t &ts_diff)
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        float average = 0.;
        for(size_t j = 0; j < time_samples; ++j)
        {
            average += buf[i + j*ts_diff];
        }
        average /= time_samples;

        if(average > pedestal[i].noise * zerosup_thres) {
            GEM_Data hit(fec_id, adc_ch, i);

            for(size_t j = 0; j < time_samples; ++j)
            {
                hit.add_value(buf[i + j*ts_diff]);
            }

            hits.push_back(hit);
        }
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

/* strip comparison
  //------------ APV25 Internal Channel Mapping
  int ref = (32 * (ch%4)) + (8 * (ch/4)) - (31 * (ch/16)) ;

  //------------ APV25 Channel to readout strip Mapping
  if((plane.find("X") != string::npos) && (plane_index == 11)) {
    if (ref % 2 == 0)
      ref = ( ref / 2) + 48 ;
    else
      if (ref < 96)
        ref = (95 - ref) / 2 ;
      else
        ref = 127 + (97 - ref) / 2 ;
  }
  else { // NON (fDetectorType == "PRADGEM") && (fPlane.Contains("Y")) && (fAPVIndex == 11)
    if (ref % 2 == 0)
      ref = ( ref / 2) + 32 ;
    else
      if (ref < 64)
        ref = (63 - ref) / 2 ;
      else
        ref = 127 + (65 - ref) / 2 ;
  }
  //   printf("PRDPedestal::PRadStripsFMapping ==>  APVID=%d, ref=%d, stripNo=%d, \n",fAPVID, chno, ref) ;

    cout << fec_id << "  " << adc_ch << "  " << ch << "  " << strip << "  " << ref << endl;
*/

