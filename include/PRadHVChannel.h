#ifndef PRAD_HV_CHANNEL_H
#define PRAD_HV_CHANNEL_H

#include "CAENHVWrapper.h"
#include <vector>
#include <string>
#include <pthread.h>
#include "datastruct.h"

using namespace std;

class PRadDataHandler;

class PRadHVChannel
{
public:
    struct CAEN_Board
    {
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

    struct CAEN_Crate
    {
        unsigned char id;
        string name;
        string ip;
        CAENHV_SYSTEM_TYPE_t sysType;
        int linkType;
        string username;
        string password;
        int handle;
        vector<CAEN_Board> boardList;
        bool mapped;

        // constructor
        CAEN_Crate() {};
        CAEN_Crate(unsigned char i, string n, string p, CAENHV_SYSTEM_TYPE_t type,
                   int link, string user, string pwd)
        : id(i), name(n), ip(p), sysType(type), linkType(link),
          username(user), password(pwd), handle(-1), mapped(false) {};
    };

    enum ShowErrorType
    {
        ShowError,
        ShowAnything,
    };

    PRadHVChannel(PRadDataHandler *h);
    virtual ~PRadHVChannel();
    void AddCrate(const string &name,
                  const string &ip,
                  const unsigned char &id,
                  const CAENHV_SYSTEM_TYPE_t &type = SY1527,
                  const int &linkType = LINKTYPE_TCPIP,
                  const string &username = "admin",
                  const string &password = "admin");
    void Initialize();
    void StartMonitor();
    void StopMonitor() {alive = false;};
    void SetPowerOn(bool &val);
    void SetPowerOn(CrateConfig &config, bool &val);
    void SetVoltage(const char *name, CrateConfig &config, float &val);
    void ReadVoltage();
    void PrintOut();

    static void *loop_helper(void *ptr)
    {
        ((PRadHVChannel *)ptr)->queryLoop();
        return NULL;
    }

private:
    PRadDataHandler *myHandler;
    vector<CAEN_Crate> crateList;
    volatile bool alive;
    void getCrateMap(CAEN_Crate &crate);
    void heartBeat();
    void queryLoop();
    void showError(const string &prefix, const int &err, ShowErrorType type = ShowError);
    pthread_t loopThread;
    pthread_mutex_t locker;
};

#endif
