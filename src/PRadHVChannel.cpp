//============================================================================//
// A C++ wrapper class to use CAENHVWrapper library                           //
// It is not finished yet                                                     //
//                                                                            //
// Chao Peng                                                                  //
// 02/17/2016                                                                 //
//============================================================================//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include "PRadHVChannel.h"
#include "PRadDataHandler.h"

ostream &operator<<(ostream &os, PRadHVChannel::CAEN_Board const &b)
{
    return os << b.model << ", " << b.desc << ", "
              << b.slot << ", " << b.nChan << ", "
              << b.serNum << ", "
              << (int)b.fmwMSB << "." << (int)b.fmwLSB;
}

ostream &operator<<(ostream &os, CAENHVData const &data)
{
    return os << "Module: " << data.name << ", "
              << "VMon: " << data.Vmon << " V, "
              << "VSet:" << data.Vset << " V."
              << endl;
}

PRadHVChannel::PRadHVChannel(PRadDataHandler *h)
: myHandler(h), alive(false)
{
    pthread_mutex_init(&locker, 0);
}

PRadHVChannel::~PRadHVChannel()
{
    StopMonitor();
    pthread_join(loopThread, 0);
}

void PRadHVChannel::AddCrate(const string &name,
                             const string &ip,
                             const unsigned char &id,
                             const CAENHV_SYSTEM_TYPE_t &type,
                             const int &linkType,
                             const string &username,
                             const string &password)
{
    if(alive)
        cerr << "HV Channel AddCrate Error: HV Channel is alive, cannot add crate" << endl;
    crateList.push_back(CAEN_Crate(id, name, ip, type, linkType, username, password));
}

void PRadHVChannel::Initialize()
{
    for(size_t i = 0; i < crateList.size(); ++i)
    {
        int err;
        char arg[32];
        strcpy(arg, crateList[i].ip.c_str());
        err = CAENHV_InitSystem(crateList[i].sysType,
                                crateList[i].linkType,
                                arg,
                                crateList[i].username.c_str(),
                                crateList[i].password.c_str(),
                                &crateList[i].handle);
        if(err == CAENHV_OK) {
            cout << "Connected to high voltage system "
                 << crateList[i].name << "@" << crateList[i].ip
                 << endl;
            if(!crateList[i].mapped) // does not have crate map yet
                getCrateMap(crateList[i]);
        }
        else {
            cerr << "Cannot connect to "
                 << crateList[i].name << "@" << crateList[i].ip
                 << endl;
            showError("HV Initialize", err);
        }
    }
}

void PRadHVChannel::getCrateMap(CAEN_Crate &crate)
{
    if(crate.handle < 0) {
        cerr << "Uninitialized crate " << crate.name
             << ", cannot get crate map!" << endl;
        return;
    }
    unsigned short NbofSlot;
    unsigned short *NbofChList;
    char *modelList, *descList;
    unsigned short *serNumList;
    unsigned char *fmwMinList, *fmwMaxList;

    // looks like this function only return the first model name rather than a list
    int err = CAENHV_GetCrateMap(crate.handle, &NbofSlot, &NbofChList, &modelList, &descList, &serNumList, &fmwMinList, &fmwMaxList);

    char *m = modelList, *d = descList;
    if(err == CAENHV_OK) {
        for(int slot = 0; slot < NbofSlot; ++slot, m += strlen(m) + 1, d += strlen(d) + 1) {
            if(NbofChList[slot]) {
               CAEN_Board newBoard(m, d, slot, NbofChList[slot], serNumList[slot], fmwMinList[slot], fmwMaxList[slot]);
                crate.boardList.push_back(newBoard);
            }
        }
    }
    free(NbofChList);
    free(modelList);
    free(descList);
    free(serNumList);
    free(fmwMinList);
    free(fmwMaxList);

    crate.mapped = true;
}

void PRadHVChannel::StartMonitor()
{
    alive = true;
    pthread_create(&loopThread, NULL, &PRadHVChannel::loop_helper, this);
}

void PRadHVChannel::queryLoop()
{
    int loopCount = 0;
    while(alive)
    {
        if(!(loopCount%60))
            ReadVoltage();
        else if(!(loopCount%15))
            heartBeat();
        ++loopCount;
        sleep(1);
    }
}

void PRadHVChannel::heartBeat()
{
    pthread_mutex_lock(&locker);
    for(size_t i = 0; i < crateList.size(); ++i)
    {
        if(crateList[i].handle < 0)
            continue;

        char sw[30];
        int err = CAENHV_GetSysProp(crateList[i].handle, "SwRelease", sw);
        showError("HV Heartbeat", err);
    }
    pthread_mutex_unlock(&locker);
}

void PRadHVChannel::SetPowerOn(bool &val)
{
    for(size_t i = 0; i < crateList.size(); ++i)
    {
        for(size_t j = 0; j < crateList[i].boardList.size(); ++j)
        {
            int size = crateList[i].boardList[j].nChan;
            unsigned short list[size];
            unsigned int valList[size];
            for(int k = 0; k < size; ++k)
            {
                valList[k] = (val)?1:0;
                list[k] = k;
            }

            int err = CAENHV_SetChParam(crateList[i].handle,
                                        crateList[i].boardList[j].slot,
                                        "Pw",
                                        size,
                                        list,
                                        valList);
            showError("HV Power On", err);
        }
    }
}

void PRadHVChannel::SetPowerOn(CrateConfig &config, bool &val)
{
    for(size_t i = 0; i < crateList.size(); ++i)
    {
        if(crateList[i].id == config.crate) {
            unsigned int value = (val)?1:0;
            unsigned short slot = (unsigned short) config.slot;
            unsigned short channel = (unsigned short) config.channel;
            int err = CAENHV_SetChParam(crateList[i].handle, slot, "Pw", 1, &channel, &value);
            showError("HV Power On", err);
            return;
        }
    }
}

void PRadHVChannel::SetVoltage(const char *name, CrateConfig &config, float &val)
{
    // set voltage limit
    float limit = (name[0] == 'G')?1900:1300;
    if(val > limit) {
        cerr << "Exceeds safe value in setting the voltage, ignore the action!" << endl;
        return;
    }

    // TODO, check the primary channel voltage
    for(size_t i = 0; i < crateList.size(); ++i)
    {
        if(crateList[i].id == config.crate) {
            float value = val;
            unsigned short slot = (unsigned short) config.slot;
            unsigned short channel = (unsigned short) config.channel;
            int err = CAENHV_SetChParam(crateList[i].handle, slot, "V0Set", 1, &channel, &value);
            showError("HV Set Voltage", err);
            return;
        }
    }
}

void PRadHVChannel::ReadVoltage()
{
    pthread_mutex_lock(&locker);
    int err;
    CAENHVData hvData;
    for(size_t i = 0; i < crateList.size(); ++i)
    {
        if(crateList[i].handle < 0)
            continue;

        hvData.config.crate = crateList[i].id;
        for(size_t j = 0; j < crateList[i].boardList.size(); ++j)
        {
            int size = crateList[i].boardList[j].nChan;
            float monVals[size], setVals[size];
            unsigned int pwON[size];
            char nameList[size][MAX_CH_NAME];
            unsigned short list[size];

            for(int k = 0; k < size; ++k)
                list[k] = k;

            err = CAENHV_GetChName(crateList[i].handle,
                                   crateList[i].boardList[j].slot,
                                   size,
                                   list,
                                   nameList);
            showError("HV Read Voltage", err);

            err = CAENHV_GetChParam(crateList[i].handle,
                                    crateList[i].boardList[j].slot,
                                    "Pw",
                                    size,
                                    list,
                                    pwON);
            showError("HV Read Voltage", err);

            err = CAENHV_GetChParam(crateList[i].handle,
                                    crateList[i].boardList[j].slot,
                                    "VMon",
                                    size,
                                    list,
                                    monVals);
            showError("HV Read Voltage", err);

            err = CAENHV_GetChParam(crateList[i].handle,
                                    crateList[i].boardList[j].slot,
                                    "V0Set",
                                    size,
                                    list,
                                    setVals);
            showError("HV Read Voltage", err);

            hvData.config.slot = (unsigned char)crateList[i].boardList[j].slot;
            for(int k = 0; k < size; ++k)
            {
                    hvData.config.channel = (unsigned char)k;
                    hvData.name = nameList[k];
                    hvData.ON = (bool)pwON[k];
                    hvData.Vmon = monVals[k];
                    hvData.Vset = setVals[k];

                    float diff = (hvData.ON)?(hvData.Vmon - hvData.Vset):0;
                    if(diff > 10 || diff < -10) {
                        cerr << "WARNING: voltage deviates from set value!" << endl
                             << hvData << endl;
                    }

                    float limit = (nameList[k][0] == 'G')?1900:1300;
                    if(hvData.Vmon > limit) {
                        cerr << "WARNING: voltage exceeds safe limit!" << endl
                             << hvData << endl;
                    }

                    myHandler->FeedData(hvData);
            }

        }
    }
    pthread_mutex_unlock(&locker);
}

void PRadHVChannel::PrintOut()
{
    cout << crateList.size() << " high voltage crates connected." << endl;
    cout << "========================================================" << endl;
    for(size_t i = 0; i < crateList.size(); ++i)
    {
        cout << crateList[i].name << "@" << crateList[i].ip
             << " - " << crateList[i].boardList.size()
             << " boards detected." 
             << endl;
        for(size_t j = 0; j < crateList[i].boardList.size(); ++j)
        {
            cout << crateList[i].boardList[j] << endl;
        }
        cout << "========================================================" << endl;
    }
}

void PRadHVChannel::showError(const string &prefix, const int &err, ShowErrorType type)
{
    if(err == CAENHV_OK && type != ShowAnything)
        return;

    string result = prefix + " ERROR: ";

    switch(err)
    {
    case 0: cout << prefix << ": Command is successfully executed," << endl; return;
    case 1: result += "Error of operatived system"; break;
    case 2: result += "Write error in communication channel"; break;
    case 3: result += "Read error in communication channel"; break;
    case 4: result += "Time out in server communication"; break;
    case 5: result += "Command Front End application is down"; break;
    case 6: result += "Communication with system not yet connected by a Login command"; break;
    case 7: result += "Communication with a not present board/slot"; break;
    case 8: result += "Communication with RS232 not yet implemented"; break;
    case 9: result += "User memory not sufficient"; break;
    case 10: result += "Value out of range"; break;
    case 11: result += "Execute command not yet implemented"; break;
    case 12: result += "Get Property not yet implemented"; break;
    case 13: result += "Set Property not yet implemented"; break;
    case 14: result += "Property not found"; break;
    case 15: result += "Execute command not found"; break;
    case 16: result += "No System property"; break;
    case 17: result += "No get property"; break;
    case 18: result += "No set property"; break;
    case 19: result += "No execute command"; break;
    case 20: result += "Device configuration changed"; break;
    case 21: result += "Property of param not found"; break;
    case 22: result += "Param not found"; break;
    case 23: result += "No data present"; break;
    case 24: result += "Device already open"; break;
    case 25: result += "To Many devices opened"; break;
    case 26: result += "Function Parameter not valid"; break;
    case 27: result += "Function not available for the connected device"; break;
    case 0x1001: result += "Device already connected"; break;
    case 0x1002: result += "Device not connected"; break;
    case 0x1003: result += "Operating system error"; break;
    case 0x1004: result += "Login failed"; break;
    case 0x1005: result += "Logout failed"; break;
    case 0x1006: result += "Link type not supported"; break;
    case 0x1007: result += "Incorrect username/password"; break;
    default: result += "Unknown error code"; break;
    }

    cerr << result << endl;
}
