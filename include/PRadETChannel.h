#ifndef PRAD_ET_CHANNEL_H
#define PRAD_ET_CHANNEL_H

#include <stdint.h>
#include "et.h"
#include "PRadException.h"

#define ET_CHUNK_SIZE 10

class PRadETChannel
{

public:
    // nested classes for configurations
    class OpenConfig
    {
    public:
        OpenConfig();
        virtual ~OpenConfig();

        // wrapper functions
        void Initialize();
        void SetWait(int val);
        void SetTimeOut(struct timespec val);
        void SetHost(const char *val);
        void SetCast(int val);
        void SetTTL(int val);
        void SetPort(unsigned short val);
        void SetMultiPort(unsigned short val);
        void SetServerPort(unsigned short val);
        void AddBroadCast(const char *val);
        void RemoveBroadCast(const char *val);
        void AddMultiCast(const char *val);
        void RemoveMultiCast(const char *val);
        void SetPolicy(int val);
        void SetMode(int val);
        void SetDebugDefault(int val);
        void SetInterface(const char *val);
        void SetTCP(int rBufSize, int sBufSize, int noDelay);

    public:
        et_openconfig config;
    };

    class StationConfig
    {
    public:
        StationConfig();
        virtual ~StationConfig();

        //wrapper functions
        void Initialize();

    public:
        et_statconfig config;
    };

public:
    PRadETChannel(size_t size = 1048576);
    virtual ~PRadETChannel();
    void Open(const char* ipAddr, int tcpPort, const char* etFile) throw(PRadException);
    void CreateStation(std::string stName, int mode) throw(PRadException);
    void AttachStation() throw(PRadException);
    void DetachStation();
    void ForceClose();
    bool Read() throw(PRadException);
    void *GetBuffer() {return (void*) buffer;};
    size_t GetBufferLength() {return bufferSize;};
    OpenConfig &GetOpenConfig() {return openConf;};
    StationConfig &GetStationConfig() {return stationConf;};

private:
    OpenConfig openConf;
    StationConfig stationConf;
    et_att_id attachID;
    et_stat_id stationID;
    et_sys_id etID;
    et_event *etEvent;
    uint32_t *buffer;
    size_t bufferSize;
};

#endif
