#ifndef PRAD_DST_PARSER_H
#define PRAD_DST_PARSER_H

#include <fstream>
#include <string>
#include "PRadException.h"
#include "PRadEventStruct.h"

class PRadDataHandler;

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
    void SetMode(const uint32_t &bit) {update_mode = bit;};
    bool Read();
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
    void readEPICSMap() throw(PRadException);
    void readRunInfo() throw(PRadException);
    void readHyCalInfo() throw(PRadException);
    void readGEMInfo() throw(PRadException);

private:
    PRadDataHandler *handler;
    std::ofstream dst_out;
    std::ifstream dst_in;
    int64_t input_length;
    EventData event;
    EPICSData epics_event;
    PRadDSTInfo type;
    uint32_t update_mode;
};

#endif
