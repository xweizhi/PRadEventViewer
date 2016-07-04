//===========================================================================//
//  Binary data file reading and writing related functions                   //
//                                                                           //
//  Chao Peng                                                                //
//  07/04/2016                                                               //
//===========================================================================//

#include <iostream>
#include <iomanip>
#include "PRadDSTParser.h"
#include "PRadDataHandler.h"
#include "PRadDAQUnit.h"
#include "PRadGEMSystem.h"

#define DST_FILE_VERSION 0x13  // 0xff

using namespace std;

PRadDSTParser::PRadDSTParser(PRadDataHandler *h)
: handler(h), input_length(0), type(PRad_DST_Undefined)
{
}

PRadDSTParser::~PRadDSTParser()
{
    CloseInput();
    CloseOutput();
}

void PRadDSTParser::OpenOutput(const string &path, ios::openmode mode)
{
    dst_out.open(path, mode);

    if(!dst_out.is_open()) {
        cerr << "DST Parser: Cannot open output file " << path
             << ", stop writing!" << endl;
        return;
    }

    uint32_t version_info = (PRad_DST_Header << 8) | DST_FILE_VERSION;
    dst_out.write((char*) &version_info, sizeof(version_info));
}

void PRadDSTParser::CloseOutput()
{
    dst_out.close();
}

void PRadDSTParser::OpenInput(const string &path, ios::openmode mode)
{
    dst_in.open(path, mode);

    if(!dst_in.is_open()) {
        cerr << "DST Parser: Cannot open input dst file "
             << "\"" << path << "\""
             << ", stop reading!" << endl;
        return;
    }

    dst_in.seekg(0, dst_in.end);
    input_length = dst_in.tellg();
    dst_in.seekg(0, dst_in.beg);

    uint32_t version_info;
    dst_in.read((char*) &version_info, sizeof(version_info));

    if((version_info >> 8) != PRad_DST_Header) {
        cerr << "DST Parser: Unrecognized PRad dst file, stop reading!" << endl;
        CloseInput();
        return;
    }
    if((version_info & 0xff) != DST_FILE_VERSION) {
        cerr << "DST Parser: Version mismatch between the file and library. "
             << endl
             << "Expected version " << (DST_FILE_VERSION >> 4)
             << "." << (DST_FILE_VERSION & 0xf)
             << ", but the file version is "
             << ((version_info >> 4) & 0xf)
             << "." << (version_info & 0xf)
             << ", please use correct library to open this file."
             << endl;
        CloseInput();
        return;
    }
}

void PRadDSTParser::CloseInput()
{
    dst_in.close();
}

void PRadDSTParser::WriteEvent(const EventData &data) throw(PRadException)
{
    if(!dst_out.is_open())
        throw PRadException("WRITE DST", "output file is not opened!");

    // write header
    uint32_t event_info = (PRad_DST_EvHeader << 8) | PRad_DST_Event;
    dst_out.write((char*) &event_info, sizeof(event_info));

    // event information
    dst_out.write((char*) &data.event_number, sizeof(data.event_number));
    dst_out.write((char*) &data.type        , sizeof(data.type));
    dst_out.write((char*) &data.trigger     , sizeof(data.trigger));
    dst_out.write((char*) &data.timestamp   , sizeof(data.timestamp));

    // all data banks
    uint32_t adc_size = data.adc_data.size();
    uint32_t tdc_size = data.tdc_data.size();
    uint32_t gem_size = data.gem_data.size();
    uint32_t dsc_size = data.dsc_data.size();

    dst_out.write((char*) &adc_size, sizeof(adc_size));
    for(auto &adc : data.adc_data)
        dst_out.write((char*) &adc, sizeof(adc));

    dst_out.write((char*) &tdc_size, sizeof(tdc_size));
    for(auto &tdc : data.tdc_data)
        dst_out.write((char*) &tdc, sizeof(tdc));

    dst_out.write((char*) &gem_size, sizeof(gem_size));
    for(auto &gem : data.gem_data)
    {
        dst_out.write((char*) &gem.addr, sizeof(gem.addr));
        uint32_t hit_size = gem.values.size();
        dst_out.write((char*) &hit_size, sizeof(hit_size));
        for(auto &value : gem.values)
            dst_out.write((char*) &value, sizeof(value));
    }

    dst_out.write((char*) &dsc_size, sizeof(dsc_size));
    for(auto &dsc : data.dsc_data)
        dst_out.write((char*) &dsc, sizeof(dsc));

}

void PRadDSTParser::readEvent(EventData &data) throw(PRadException)
{
    if(!dst_in.is_open())
        throw PRadException("READ DST", "input file is not opened!");

    data.clear();

    // event information
    dst_in.read((char*) &data.event_number, sizeof(data.event_number));
    dst_in.read((char*) &data.type        , sizeof(data.type));
    dst_in.read((char*) &data.trigger     , sizeof(data.trigger));
    dst_in.read((char*) &data.timestamp   , sizeof(data.timestamp));

    uint32_t adc_size, tdc_size, gem_size, value_size, dsc_size;
    ADC_Data adc;
    TDC_Data tdc;
    DSC_Data dsc;

    dst_in.read((char*) &adc_size, sizeof(adc_size));
    for(uint32_t i = 0; i < adc_size; ++i)
    {
        dst_in.read((char*) &adc, sizeof(adc));
        data.add_adc(adc);
    }

    dst_in.read((char*) &tdc_size, sizeof(tdc_size));
    for(uint32_t i = 0; i < tdc_size; ++i)
    {
        dst_in.read((char*) &tdc, sizeof(tdc));
        data.add_tdc(tdc);
    }

    float value;
    dst_in.read((char*) &gem_size, sizeof(gem_size));
    for(uint32_t i = 0; i < gem_size; ++i)
    {
        GEM_Data gemhit;
        dst_in.read((char*) &gemhit.addr, sizeof(gemhit.addr));
        dst_in.read((char*) &value_size, sizeof(value_size));
        for(uint32_t j = 0; j < value_size; ++j)
        {
            dst_in.read((char*) &value, sizeof(value));
            gemhit.add_value(value);
        }
        data.add_gemhit(gemhit);
    }

    dst_in.read((char*) &dsc_size, sizeof(dsc_size));
    for(uint32_t i = 0; i < dsc_size; ++i)
    {
        dst_in.read((char*) &dsc, sizeof(dsc));
        data.add_dsc(dsc);
    }
}

void PRadDSTParser::WriteEPICS(const EPICSData &data) throw(PRadException)
{
    if(!dst_out.is_open())
        throw PRadException("WRITE DST", "output file is not opened!");

    // write header
    uint32_t event_info = (PRad_DST_EvHeader << 8) | PRad_DST_Epics;
    dst_out.write((char*) &event_info, sizeof(event_info));

    dst_out.write((char*) &data.event_number, sizeof(data.event_number));

    uint32_t value_size = data.values.size();
    dst_out.write((char*) &value_size, sizeof(value_size));

    for(auto value : data.values)
        dst_out.write((char*) &value, sizeof(value));
}

void PRadDSTParser::readEPICS(EPICSData &data) throw(PRadException)
{
    if(!dst_in.is_open())
        throw PRadException("READ DST", "input file is not opened!");

    data.clear();

    dst_in.read((char*) &data.event_number, sizeof(data.event_number));

    uint32_t value_size;
    dst_in.read((char*) &value_size, sizeof(value_size));

    for(uint32_t i = 0; i < value_size; ++i)
    {
        float value;
        dst_in.read((char*) &value, sizeof(value));
        data.values.push_back(value);
    }
}

void PRadDSTParser::WriteRunInfo() throw(PRadException)
{
    if(!dst_out.is_open())
        throw PRadException("WRITE DST", "output file is not opened!");

    // write header
    uint32_t event_info = (PRad_DST_EvHeader << 8) | PRad_DST_Run_Info;
    dst_out.write((char*) &event_info, sizeof(event_info));

    auto runInfo = handler->GetRunInfo();
    dst_out.write((char*) &runInfo, sizeof(runInfo));
}

void PRadDSTParser::readRunInfo(bool update) throw(PRadException)
{
    if(!dst_in.is_open())
        throw PRadException("READ DST", "input file is not opened!");

    RunInfo runInfo;

    dst_in.read((char*) &runInfo, sizeof(runInfo));

    if(update)
        handler->UpdateRunInfo(runInfo);
}

void PRadDSTParser::WriteEPICSMap() throw(PRadException)
{
    if(!dst_out.is_open())
        throw PRadException("WRITE DST", "output file is not opened!");

    // write header
    uint32_t event_info = (PRad_DST_EvHeader << 8) | PRad_DST_Epics_Map;
    dst_out.write((char*) &event_info, sizeof(event_info));

    vector<epics_ch> epics_channels = handler->GetSortedEPICSList();

    uint32_t ch_size = epics_channels.size();
    dst_out.write((char*) &ch_size, sizeof(ch_size));

    for(auto &ch : epics_channels)
    {
        uint32_t str_size = ch.name.size();
        dst_out.write((char*) &str_size, sizeof(str_size));
        for(auto &c : ch.name)
            dst_out.write((char*) &c, sizeof(c));
        dst_out.write((char*) &ch.id, sizeof(ch.id));
        dst_out.write((char*) &ch.value, sizeof(ch.value));
    }
}

void PRadDSTParser::readEPICSMap(bool update) throw(PRadException)
{
    if(!dst_in.is_open())
        throw PRadException("READ DST", "input file is not opened!");

    uint32_t ch_size, str_size, id;
    string str;
    float value;

    dst_in.read((char*) &ch_size, sizeof(ch_size));
    for(uint32_t i = 0; i < ch_size; ++i)
    {
        str = "";
        dst_in.read((char*) &str_size, sizeof(str_size));
        for(uint32_t j = 0; j < str_size; ++j)
        {
            char c;
            dst_in.read(&c, sizeof(c));
            str.push_back(c);
        }
        dst_in.read((char*) &id, sizeof(id));
        dst_in.read((char*) &value, sizeof(value));

        if(update)
            handler->RegisterEPICS(str, id, value);
    }
}

void PRadDSTParser::WriteHyCalInfo() throw(PRadException)
{
    if(!dst_out.is_open())
        throw PRadException("WRITE DST", "output file is not opened!");

    // write header
    uint32_t event_info = (PRad_DST_EvHeader << 8) | PRad_DST_HyCal_Info;
    dst_out.write((char*) &event_info, sizeof(event_info));

    uint32_t ch_size = handler->GetChannelList().size();
    dst_out.write((char*) &ch_size, sizeof(ch_size));

    for(auto channel : handler->GetChannelList())
    {
        PRadDAQUnit::Pedestal ped = channel->GetPedestal();
        dst_out.write((char*) &ped, sizeof(ped));

        PRadDAQUnit::CalibrationConstant cal = channel->GetCalibrationConstant();
        uint32_t gain_size = cal.base_gain.size();
        dst_out.write((char*) &cal.factor, sizeof(cal.factor));
        dst_out.write((char*) &cal.base_factor, sizeof(cal.base_factor));
        dst_out.write((char*) &gain_size, sizeof(gain_size));
        for(auto &gain : cal.base_gain)
            dst_out.write((char*) &gain, sizeof(gain));
    }
}

void PRadDSTParser::readHyCalInfo(bool update) throw(PRadException)
{
    if(!dst_in.is_open())
        throw PRadException("READ DST", "input file is not opened!");

    uint32_t ch_size;
    dst_in.read((char*) &ch_size, sizeof(ch_size));

    auto channelList = handler->GetChannelList();

    for(uint32_t i = 0; i < ch_size; ++i)
    {
        PRadDAQUnit::Pedestal ped;
        dst_in.read((char*) &ped, sizeof(ped));

        PRadDAQUnit::CalibrationConstant cal;
        dst_in.read((char*) &cal.factor, sizeof(cal.factor));
        dst_in.read((char*) &cal.base_factor, sizeof(cal.base_factor));

        double gain;
        uint32_t gain_size;
        dst_in.read((char*) &gain_size, sizeof(gain_size));

        for(uint32_t j = 0; j < gain_size; ++j)
        {
            dst_in.read((char*) &gain, sizeof(gain));
            cal.base_gain.push_back(gain);
        }

        if((i < channelList.size()) && update) {
            channelList.at(i)->UpdatePedestal(ped);
            channelList.at(i)->UpdateCalibrationConstant(cal);
        }
    }
}

void PRadDSTParser::WriteGEMInfo() throw(PRadException)
{
    if(!dst_out.is_open())
        throw PRadException("WRITE DST", "output file is not opened!");

    // write header
    uint32_t event_info = (PRad_DST_EvHeader << 8) | PRad_DST_GEM_Info;
    dst_out.write((char*) &event_info, sizeof(event_info));

    vector<PRadGEMAPV *> apv_list = handler->GetSRS()->GetAPVList();

    uint32_t apv_size = apv_list.size();
    dst_out.write((char*) &apv_size, sizeof(apv_size));

    for(auto apv : apv_list)
    {
        GEMChannelAddress addr(apv->fec_id, apv->adc_ch);
        dst_out.write((char*) &addr, sizeof(addr));

        vector<PRadGEMAPV::Pedestal> ped_list = apv->GetPedestalList();
        uint32_t ped_size = ped_list.size();
        dst_out.write((char*) &ped_size, sizeof(ped_size));

        for(auto &ped : ped_list)
            dst_out.write((char*) &ped, sizeof(ped));
    }
}

void PRadDSTParser::readGEMInfo(bool update) throw(PRadException)
{
    if(!dst_in.is_open())
        throw PRadException("READ DST", "input file is not opened!");

    uint32_t apv_size, ped_size;
    dst_in.read((char*) &apv_size, sizeof(apv_size));

    for(uint32_t i = 0; i < apv_size; ++i)
    {
        GEMChannelAddress addr;
        dst_in.read((char*) &addr, sizeof(addr));

        auto apv = handler->GetSRS()->GetAPV(addr);

        dst_in.read((char*) &ped_size, sizeof(ped_size));

        for(uint32_t j = 0; j < ped_size; ++j)
        {
            PRadGEMAPV::Pedestal ped;
            dst_in.read((char*) &ped, sizeof(ped));
            if(apv && update)
                apv->UpdatePedestal(ped, j);
        }
    }
}

//============================================================================//
// Return type:  false. file end or error                                     //
//               true. successfully read                                      //
//============================================================================//
bool PRadDSTParser::Read(bool update)
{
    try {
        if(dst_in.tellg() < input_length && dst_in.tellg() != -1)
        {
            uint32_t event_info;
            dst_in.read((char*) &event_info, sizeof(event_info));

            if((event_info >> 8) != PRad_DST_EvHeader) {
                cerr << "DST Parser: Unrecognized event header "
                     << hex << setw(8) << setfill('0') << event_info
                     <<" in PRad dst file, probably corrupted file!"
                     << dec << endl;
                return false;
            }
            // read by event type
            type = (PRadDSTInfo)(event_info&0xff);

            switch(type)
            {
            case PRad_DST_Event:
                readEvent(event);
                break;
            case PRad_DST_Epics:
                readEPICS(epics_event);
                break;
            case PRad_DST_Epics_Map:
                readEPICSMap(update);
                break;
            case PRad_DST_Run_Info:
                readRunInfo(update);
                break;
            case PRad_DST_HyCal_Info:
                readHyCalInfo(update);
                break;
            case PRad_DST_GEM_Info:
                readGEMInfo(update);
                break;
            default:
                return false;
            }

            return true;
        } else {
            // file end
            return false;
        }

    } catch(PRadException &e) {
        cerr << e.FailureType() << ": "
             << e.FailureDesc() << endl
             << "Read from DST Aborted!" << endl;
        return false;
    } catch(exception &e) {
        cerr << e.what() << endl
             << "Read from DST Aborted!" << endl;
        return false;
    }
}
