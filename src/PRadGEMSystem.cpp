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
#include <algorithm>

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
            new_apv->SetZeroSupThresLevel(3.);

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
        vector<GEM_Data> apv_hits = apv_it.second->GetZeroSupHits();
        if(apv_hits.size()) {
            gem_data.insert(gem_data.end(), apv_hits.begin(), apv_hits.end());
        }
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
#define DEFAULT_MAX_CHANNEL 3096
    // should calculate this according to time_sample
    buffer_size = DEFAULT_MAX_CHANNEL;

    raw_data = new int[buffer_size];

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
#define APV_HEADER_SIZE 164 //TODO, arbitrary number, need to know exact buffer size the apv need

    buffer_size = t*TIME_SAMPLE_SIZE + APV_HEADER_SIZE;

    // reallocate the memory for proper size
    delete[] raw_data;

    raw_data = new int[buffer_size];

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

void PRadGEMAPV::SplitData(const uint32_t &data, int &word1, int &word2)
{
    word1 = (((data>>16)&0xff)<<8) | (data>>24);
    word2 = ((data&0xff)<<8) | ((data>>8)&0xff);
}

vector<GEM_Data> PRadGEMAPV::GetZeroSupHits()
{
    vector<GEM_Data> result;
    size_t time_sample = 0;

    for(size_t i = 2; (i < buffer_size) && (time_sample < time_samples); ++i)
    {
       if( (raw_data[i]   < header_level) &&
           (raw_data[i-1] < header_level) &&
           (raw_data[i-2] < header_level) )
       {
           i += 10;

           if( i + TIME_SAMPLE_SIZE >= buffer_size) {
               cerr << "GEM APV: Warning, get a time sample ends at index "
                    << i << ", it exceeds the buffer size " << buffer_size
                    << endl;
               break;
           }

           ZeroSuppression(result, &raw_data[i], time_sample);

           i += TIME_SAMPLE_SIZE;
           ++time_sample;
       } 
    }

    return result;
}

void PRadGEMAPV::ZeroSuppression(vector<GEM_Data> &hits, int *buf, const size_t &time_sample)
{
    if(split)
        CommonModeCorrection_Split(buf, TIME_SAMPLE_SIZE);
    else
        CommonModeCorrection(buf, TIME_SAMPLE_SIZE);

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(buf[i] > pedestal[i].noise * zerosup_thres) {
            hits.push_back(GEM_Data(fec_id, adc_ch, time_sample, i, (unsigned short)buf[i]));
        }
    }
}

void PRadGEMAPV::CommonModeCorrection(int *buf, const size_t &size)
{
    int count = 0;
    float average = 0;

    for(size_t i = 0; i < size; ++i)
    {
        buf[i] -= pedestal[i].offset;

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

void PRadGEMAPV::CommonModeCorrection_Split(int *buf, const size_t &size)
{
    int count1 = 0, count2 = 0;
    float average1 = 0, average2 = 0;

    for(size_t i = 0; i < size; ++i)
    {
        buf[i] -= pedestal[i].offset;

        if(buf[i] < pedestal[i].noise * common_thres) {
            if(strip_map[i] < 16) {
                average1 += buf[i];
                count1++;
            } else {
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

