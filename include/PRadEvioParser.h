#ifndef PRAD_EVIO_PARSER_H
#define PRAD_EVIO_PARSER_H

#include <stdint.h>
#include <datastruct.h>

class PRadDataHandler;
class ConfigParser;

class PRadEvioParser
{
public:
    PRadEvioParser(PRadDataHandler* handler);
    virtual ~PRadEvioParser();
    void parseEventByHeader(PRadEventHeader *evtHeader);
    void parseADC1881M(const uint32_t *data);
    void parseGEMData(const uint32_t *data, const size_t &size, const int &fec_id);
    void parseTDCV767(const uint32_t *data, const size_t &size, const int &roc_id);
    void parseTDCV1190(const uint32_t *data, const size_t &size, const int &roc_id);
    void parseDSCData(const uint32_t *data);
    void parseTIData(const uint32_t *data, const size_t &size, const int &roc_id);
    void parseEPICS(const uint32_t *data);
    unsigned int eventNb;

private:
    PRadDataHandler *myHandler;
    ConfigParser *c_parser;
};

#endif
