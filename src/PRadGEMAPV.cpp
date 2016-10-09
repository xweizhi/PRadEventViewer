//============================================================================//
// GEM APV class                                                              //
// APV is the basic unit for GEM DAQ system                                   //
//                                                                            //
// Chao Peng                                                                  //
// 10/07/2016                                                                 //
//============================================================================//


#include <iostream>
#include <iomanip>
#include "PRadGEMPlane.h"
#include "PRadGEMAPV.h"
#include "TF1.h"
#include "TH1.h"


PRadGEMAPV::PRadGEMAPV(const int &f,
                       const int &ch,
                       const int &o,
                       const int &idx,
                       const int &hl,
                       const std::string &s)
: plane(nullptr), fec_id(f), adc_ch(ch), orientation(o),
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

    if(status.find("split") != std::string::npos)
        split = true;
    else
        split = false;

    ClearData();
}

PRadGEMAPV::~PRadGEMAPV()
{
    if(plane != nullptr)
        plane->DisconnectAPV(plane_index);

    delete[] raw_data;
}

void PRadGEMAPV::SetDetectorPlane(PRadGEMPlane *p)
{
    plane = p;

    // strip map is related to plane that connected, thus build the map
    if(p != nullptr)
        BuildStripMap();
}

void PRadGEMAPV::SetPlaneIndex(const int &p)
{
    plane_index = p;

    // re-connect to the current plane
    if(plane != nullptr)
        plane->ConnectAPV(this);
}

void PRadGEMAPV::CreatePedHist()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(offset_hist[i] == nullptr) {
            std::string name = "CH_" + std::to_string(i) + "_OFFSET_"
                             + std::to_string(fec_id) + "_"
                             + std::to_string(adc_ch);
            offset_hist[i] = new TH1I(name.c_str(), "Pedestal", 500, 2000, 3500);
        }
        if(noise_hist[i] == nullptr) {
            std::string name = "CH_" + std::to_string(i) + "_NOISE_"
                             + std::to_string(fec_id) + "_"
                             + std::to_string(adc_ch);
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
    // set to a high value that won't trigger zero suppression
    for(size_t i = 0; i < buffer_size; ++i)
        raw_data[i] = 5000.;

    ResetHitPos();
}

void PRadGEMAPV::ResetHitPos()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
        hit_pos[i] = false;
}

void PRadGEMAPV::ClearPedestal()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
        pedestal[i] = Pedestal(0, 0);
}

void PRadGEMAPV::UpdatePedestal(std::vector<Pedestal> &ped)
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
        std::cerr << "Received " << size * 2 << " adc words, "
                  << "but APV " << adc_ch << " in FEC " << fec_id
                  << " has only " << buffer_size << " channels" << std::endl;
        return;
    }

    for(size_t i = 0; i < size; ++i)
    {
        SplitData(buf[i], raw_data[2*i], raw_data[2*i+1]);
    }

    ts_index = GetTimeSampleStart();
}

void PRadGEMAPV::FillZeroSupData(const size_t &ch, const size_t &ts, const unsigned short &val)
{
    size_t idx = ch + ts_index + ts*TIME_SAMPLE_DIFF;
    if(ts >= time_samples ||
       ch >= TIME_SAMPLE_SIZE ||
       idx >= buffer_size)
    {
        std::cerr << "GEM APV Error: Failed to fill zero suppressed data, "
                  << " channel " << ch << " or time sample " << ts
                  << " is not allowed."
                  << std::endl;
        return;
    }

    hit_pos[ch] = true;
    raw_data[idx] = val;
}

void PRadGEMAPV::FillZeroSupData(const size_t &ch, const std::vector<float> &vals)
{
    if(vals.size() != time_samples || ch >= TIME_SAMPLE_SIZE)
    {
        std::cerr << "GEM APV Error: Failed to fill zero suppressed data, "
                  << " channel " << ch << " or time sample " << vals.size()
                  << " is not allowed."
                  << std::endl;
        return;
    }

    hit_pos[ch] = true;

    for(size_t i = 0; i < vals.size(); ++i)
    {
        size_t idx = ch + ts_index + i*TIME_SAMPLE_DIFF;
        raw_data[idx] = vals[i];
    }

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
                if(strip_map[i].local < 16)
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
           (set == 1 && strip_map[i].local < 16) ||
           (set == 2 && strip_map[i].local >= 16))
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
    if(plane == nullptr)
    {
        std::cerr << "GEM APV Error: APV "
                  << fec_id << ", " << adc_ch
                  << " is not connected to a detector plane, "
                  << "cannot handle data without correct mapping."
                  << std::endl;
        return;
    }

    if((ts_index + TIME_SAMPLE_DIFF*(time_samples - 1) + TIME_SAMPLE_SIZE) >= buffer_size)
    {
        std::cout << fec_id << ", " << adc_ch << "  "
                  << "incorrect time sample position: "  << ts_index
                  << " " << buffer_size << " " << time_samples
                  << std::endl;
        return;
    }

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

void PRadGEMAPV::CollectZeroSupHits(std::vector<GEM_Data> &hits)
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(hit_pos[i] == false)
            continue;

        GEM_Data hit(fec_id, adc_ch, i);
        for(size_t j = 0; j < time_samples; ++j)
        {
            hit.values.emplace_back(raw_data[i + ts_index + j*TIME_SAMPLE_DIFF]);
        }
        hits.emplace_back(hit);
    }
}

void PRadGEMAPV::CollectZeroSupHits()
{
    if(plane == nullptr)
        return;

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(hit_pos[i] == false)
            continue;

        std::vector<float> charges;
        for(size_t j = 0; j < time_samples; ++j)
        {
            charges.push_back(raw_data[i + ts_index + j*TIME_SAMPLE_DIFF]);
        }
        plane->AddPlaneHit(strip_map[i].plane, charges);
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
        if(strip_map[i].local < 16) {
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
        if(strip_map[i].local < 16)
            buf[i] -= average1;
        else
            buf[i] -= average2;
    }
}

// both local strip map and plane strip map are related to the connected plane
// thus this function will only be called when the APV is connected to the plane
void PRadGEMAPV::BuildStripMap()
{
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        strip_map[i] = MapStrip(i);
    }
}

int PRadGEMAPV::GetLocalStripNb(const size_t &ch)
{
   if(ch >= TIME_SAMPLE_SIZE) {
       std::cerr << "GEM APV Get Local Strip Error:"
                 << " APV " << adc_ch
                 << " in FEC " << fec_id
                 << " only has " << TIME_SAMPLE_SIZE
                 << " channels." << std::endl;
       return -1;
   }

   return (int)strip_map[ch].local;
}

int PRadGEMAPV::GetPlaneStripNb(const size_t &ch)
{
   if(ch >= TIME_SAMPLE_SIZE) {
       std::cerr << "GEM APV Get Plane Strip Error:"
                 << " APV " << adc_ch
                 << " in FEC " << fec_id
                 << " only has " << TIME_SAMPLE_SIZE
                 << " channels." << std::endl;
       return -1;
   }

   return strip_map[ch].plane;
}

PRadGEMAPV::StripNb PRadGEMAPV::MapStrip(int ch)
{
    StripNb result;

    // calculate local strip mapping
    // APV25 Internal Channel Mapping
    int strip = 32*(ch%4) + 8*(ch/4) - 31*(ch/16);

    // APV25 Channel to readout strip Mapping
    if((plane->GetType() == PRadGEMPlane::Plane_X) && (plane_index == 11)) {
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
    result.local = strip;

    // calculate plane strip mapping
    // reverse strip number by orientation
    if(orientation != plane->GetOrientation())
        strip = 127 - strip;

    // special APV
    if((plane->GetType() == PRadGEMPlane::Plane_X) && (plane_index == 11)) {
        strip += -16 + TIME_SAMPLE_SIZE * (plane_index - 1);
    } else {
        strip += TIME_SAMPLE_SIZE * plane_index;
    }

    result.plane = strip;

    return result;
}

void PRadGEMAPV::PrintOutPedestal(std::ofstream &out)
{
    out << "APV "
        << std::setw(12) << fec_id
        << std::setw(12) << adc_ch
        << std::endl;

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        out << std::setw(12) << i
            << std::setw(12) << pedestal[i].offset
            << std::setw(12) << pedestal[i].noise
            << std::endl;
    }
}

std::vector<TH1I *> PRadGEMAPV::GetHistList()
{
    std::vector<TH1I *> hist_list;

    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        if(offset_hist[i])
            hist_list.push_back(offset_hist[i]);
        if(noise_hist[i])
            hist_list.push_back(noise_hist[i]);
    }

    return hist_list;
}

std::vector<PRadGEMAPV::Pedestal> PRadGEMAPV::GetPedestalList()
{
    std::vector<Pedestal> ped_list;
    for(size_t i = 0; i < TIME_SAMPLE_SIZE; ++i)
    {
        ped_list.push_back(pedestal[i]);
    }

    return ped_list;
}

//============================================================================//
// Outside Class                                                              //
//============================================================================//
std::ostream &operator <<(std::ostream &os, const GEMChannelAddress &ad)
{
    return os << ad.fec_id << ", " << ad.adc_ch;
}

