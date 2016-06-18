#ifndef PRAD_EVIO_PARSER_H
#define PRAD_EVIO_PARSER_H

#include <fstream>
#include <cstdint>
#include "datastruct.h"
#include "PRadException.h"

class PRadDataHandler;
class ConfigParser;

class PRadEvioParser
{
public:
    PRadEvioParser(PRadDataHandler* handler);
    virtual ~PRadEvioParser();
    unsigned int GetEventNumber() {return event_number;};
    void SetEventNumber(const unsigned int &ev) {event_number = ev;};
    void ReadEvioFile(const char *filepath, const bool &verbose = false);
    void ParseEventByHeader(PRadEventHeader *evtHeader);

    static PRadTriggerType bit_to_trigger(const unsigned int &bit);
    static unsigned int trigger_to_bit(const PRadTriggerType &trg);

private:
    size_t getEvioBlock(std::ifstream &s, uint32_t *buf) throw(PRadException);
    size_t getAPVDataSize(const uint32_t *data);
    void parseADC1881M(const uint32_t *data);
    void parseGEMData(const uint32_t *data, const size_t &size, const int &fec_id);
    void parseTDCV767(const uint32_t *data, const size_t &size, const int &roc_id);
    void parseTDCV1190(const uint32_t *data, const size_t &size, const int &roc_id);
    void parseDSCData(const uint32_t *data, const size_t &size);
    void parseTIData(const uint32_t *data, const size_t &size, const int &roc_id);
    void parseEPICS(const uint32_t *data);

private:
    PRadDataHandler *myHandler;
    ConfigParser *c_parser;
    unsigned int event_number;
};

#endif
