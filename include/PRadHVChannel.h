#ifndef PRAD_HV_CHANNEL_H
#define PRAD_HV_CHANNEL_H

#include "CAENHVWrapper.h"
#include <vector>
#include <string>
#include <map>
#include "datastruct.h"

using namespace std;

class PRadDataHandler;

class PRadHVChannel
{
public:
    typedef struct
    {
        string model;
        string desc;
        unsigned short slot;
        unsigned short nChan;
        unsigned short serNum;
        unsigned char fmwLSB;
        unsigned char fmwMSB;
    } HVBoardInfo;

    typedef struct
    {
        string name;
        string ip;
        CAENHV_SYSTEM_TYPE_t sysType;
        int linkType;
        string username;
        string password;
        vector<HVBoardInfo> boardList;
        int handle;
        unsigned char id;
    } HVCrateInfo;

    enum ShowErrorType
    {
        ShowError,
        ShowAnything,
    };

    PRadHVChannel(PRadDataHandler *h);
    virtual ~PRadHVChannel();
    void AddCrate(const string name,
                  const string &ip,
                  const unsigned char &id, 
                  const CAENHV_SYSTEM_TYPE_t &type = SY1527, 
                  const int &linkType = LINKTYPE_TCPIP,
                  const string &username = "admin",
                  const string &password = "admin");
    void Initialize();
    void HeartBeat();
    void SetPowerOn(bool &val);
    void SetPowerOn(CrateConfig &config, bool &val);
    void SetVoltage(const char *name, CrateConfig &config, float &val);
    void ReadVoltage();
    void PrintOut();

private:
    PRadDataHandler *myHandler;
    vector<HVCrateInfo> crateList;
    void showError(const string &prefix, const int &err, ShowErrorType type = ShowError);
};

#endif
