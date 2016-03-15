#ifndef PRAD_ET_CHANNEL_H
#define PRAD_ET_CHANNEL_H

#include <stdint.h>
#include "et.h"
#include "PRadException.h"

#define ET_CHUNK_SIZE 10

class PRadETChannel
{
public:
    PRadETChannel(const char* ipAddr, int tcpPort, const char* etFile, size_t size = 1048576) throw(PRadException);
    virtual ~PRadETChannel();
    void CreateStation(std::string stName, int mode) throw(PRadException);
    void AttachStation() throw(PRadException);
    void DetachStation();
    bool Read() throw(PRadException);
    void *GetBuffer() {return (void*) buffer;};
    size_t GetBufferLength() {return bufferSize;};

private:
    et_att_id attachID;
    et_stat_id stationID;
    et_sys_id etID;
    et_event *etEvent;
    uint32_t *buffer;
    size_t bufferSize;
};

#endif
