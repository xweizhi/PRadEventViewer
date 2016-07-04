#ifndef PRAD_DST_PARSER_H
#define PRAD_DST_PARSER_H

#include <fstream>
#include <string>
#include "PRadException.h"
#include "PRadEventStruct.h"

class PRadDataHandler;

enum PRadDSTInfo
{
    // event types
    PRad_DST_Event = 0,
    PRad_DST_Epics,
    PRad_DST_Epics_Map,
    PRad_DST_Run_Info,
    PRad_DST_HyCal_Info,
    PRad_DST_GEM_Info,
    PRad_DST_Undefined,
};

enum PRadDSTHeader
{
    // headers
    PRad_DST_Header = 0xc0c0c0,
    PRad_DST_EvHeader = 0xe0e0e0,
};

class PRadDSTParser
{
public:
    PRadDSTParser(PRadDataHandler *h);
    virtual ~PRadDSTParser();

    void OpenOutput(const std::string &path, std::ios::openmode mode = std::ios::out | std::ios::binary);
    void OpenInput(const std::string &path, std::ios::openmode mode = std::ios::out | std::ios::binary);
    void WriteToDST(const std::string &path, std::ios::openmode mode = std::ios::out | std::ios::binary);
    void ReadFromDST(const std::string &path, std::ios::openmode mode = std::ios::in | std::ios::binary);
    void CloseOutput();
    void CloseInput();
    bool Read(bool update = true);
    PRadDSTInfo EventType() {return type;};
    EventData &GetEvent() {return event;};
    EPICSData &GetEPICSEvent() {return epics_event;};


    void WriteEvent(const EventData &data) throw(PRadException);
    void WriteEPICS(const EPICSData &data) throw(PRadException);
    void WriteEPICSMap() throw(PRadException);
    void WriteRunInfo() throw(PRadException);
    void WriteHyCalInfo() throw(PRadException);
    void WriteGEMInfo() throw(PRadException);

private:
    void readEvent(EventData &data) throw(PRadException);
    void readEPICS(EPICSData &data) throw(PRadException);
    void readEPICSMap(bool update = true) throw(PRadException);
    void readRunInfo(bool update = true) throw(PRadException);
    void readHyCalInfo(bool update = true) throw(PRadException);
    void readGEMInfo(bool update = true) throw(PRadException);

private:
    PRadDataHandler *handler;
    std::ofstream dst_out;
    std::ifstream dst_in;
    int64_t input_length;
    EventData event;
    EPICSData epics_event;
    PRadDSTInfo type;
};

#endif
