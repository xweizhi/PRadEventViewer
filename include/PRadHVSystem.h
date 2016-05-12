#ifndef PRAD_HV_SYSTEM_H
#define PRAD_HV_SYSTEM_H

#include "CAENHVWrapper.h"
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include "PRadException.h"
#include "datastruct.h"

using namespace std;

class PRadDataHandler;

class PRadHVSystem
{
public:
    class CAEN_Board
    {
    public:
        string model;
        string desc;
        unsigned short slot;
        unsigned short nChan;
        unsigned short serNum;
        unsigned char fmwLSB;
        unsigned char fmwMSB;

        // constructor
        CAEN_Board() {};
        CAEN_Board(string m, string d, unsigned short s, unsigned short n,
                   unsigned short ser, unsigned char lsb, unsigned char msb)
        : model(m), desc(d), slot(s), nChan(n), serNum(ser), fmwLSB(lsb), fmwMSB(msb)
        {};
        CAEN_Board(char* m, char* d, unsigned short s, unsigned short n,
                   unsigned short ser, unsigned char lsb, unsigned char msb)
        : model(m), desc(d), slot(s), nChan(n), serNum(ser), fmwLSB(lsb), fmwMSB(msb)
        {};
     };

    class CAEN_Crate
    {
    public:
        unsigned char id;
        string name;
        string ip;
        CAENHV_SYSTEM_TYPE_t sysType;
        int linkType;
        string username;
        string password;
        int handle;
        bool mapped;
        vector<CAEN_Board> boardList;

        // constructor
        CAEN_Crate() {};
        CAEN_Crate(unsigned char i, string n, string p, CAENHV_SYSTEM_TYPE_t type,
                   int link, string user, string pwd)
        : id(i), name(n), ip(p), sysType(type), linkType(link),
          username(user), password(pwd), handle(-1), mapped(false) {};
//        void Initialize() throw(PRadException);
//        void DeInitialize() throw(PRadException);
        void clear() {handle = -1; mapped = false; boardList.clear();};
    };

    enum ShowErrorType
    {
        ShowError,
        ShowAll,
    };

    PRadHVSystem(PRadDataHandler *h);
    virtual ~PRadHVSystem();
    void AddCrate(const string &name,
                  const string &ip,
                  const unsigned char &id,
                  const CAENHV_SYSTEM_TYPE_t &type = SY1527,
                  const int &linkType = LINKTYPE_TCPIP,
                  const string &username = "admin",
                  const string &password = "admin");
    void Initialize();
    void DeInitialize();
    void Connect();
    void Disconnect();
    void StartMonitor();
    void StopMonitor();
    void SetPowerOn(bool &val);
    void SetPowerOn(ChannelAddress &config, bool &val);
    void SetVoltage(const char *name, ChannelAddress &config, float &val);
    void ReadVoltage();
    void PrintOut();

private:
    PRadDataHandler *myHandler;
    vector<CAEN_Crate> crateList;
    volatile bool alive;
    thread queryThread;
    mutex locker;
    void getCrateMap(CAEN_Crate &crate);
    void heartBeat();
    void queryLoop();
    void checkVoltage(const CAENHVData &hvData);
    void showError(const string &prefix, const int &err, ShowErrorType type = ShowError);
    float getLimit(const char *name = "");
};

#endif
