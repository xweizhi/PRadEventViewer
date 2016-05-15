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
#include <chrono>
#include "PRadHVSystem.h"
#include "PRadDataHandler.h"

ostream &operator<<(ostream &os, PRadHVSystem::CAEN_Board const &b)
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

PRadHVSystem::PRadHVSystem(PRadDataHandler *h)
: myHandler(h), alive(false)
{}

PRadHVSystem::~PRadHVSystem()
{
    Disconnect();
}

void PRadHVSystem::AddCrate(const string &name,
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

void PRadHVSystem::Initialize()
{
    locker.lock();
    int try_cnt = 0, fail_cnt = 0;

    cout << "Trying to initialize all HV crates" << endl;

    for(auto &crate : crateList)
    {
        ++ try_cnt;
        int err;
        char arg[32];
        strcpy(arg, crate.ip.c_str());
        err = CAENHV_InitSystem(crate.sysType,
                                crate.linkType,
                                arg,
                                crate.username.c_str(),
                                crate.password.c_str(),
                                &crate.handle);
        if(err == CAENHV_OK) {
            cout << "Connected to high voltage system "
                 << crate.name << "@" << crate.ip
                 << endl;
            if(!crate.mapped) // fist time initialize, does not have crate map
                getCrateMap(crate);
        }
        else {
            ++fail_cnt;
            cerr << "Cannot connect to "
                 << crate.name << "@" << crate.ip
                 << endl;
            showError("HV Initialize", err);
        }
    }

    cout << "HV crates initialize DONE, tried "
         << try_cnt << " crates, failed for "
         << fail_cnt << " crates" << endl;

    locker.unlock();
}

void PRadHVSystem::DeInitialize()
{
    for(auto &crate : crateList)
    {
        int err = CAENHV_DeinitSystem(crate.handle);
        crate.clear();

        if(err == CAENHV_OK) {
            cout << "Disconnected from high voltage system "
                 << crate.name << "@" << crate.ip
                 << endl;
        }
        else {
            cerr << "Failed to disconnect from "
                 << crate.name << "@" << crate.ip
                 << endl;
            showError("HV DeInitialize", err);
        }
    }
}

void PRadHVSystem::Connect()
{
    Initialize();
}

void PRadHVSystem::Disconnect()
{
    StopMonitor();

    DeInitialize();
}

void PRadHVSystem::getCrateMap(CAEN_Crate &crate)
{
    if(crate.handle < 0) {
        cerr << "HV Channel Get Crate Map Error: crate "
             << crate.name << " is not initialized!"
             << endl;
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
            if(!NbofChList[slot])
                continue;
            CAEN_Board newBoard(m, d, slot, NbofChList[slot], serNumList[slot], fmwMinList[slot], fmwMaxList[slot]);
            crate.boardList.push_back(newBoard);
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

void PRadHVSystem::StartMonitor()
{
    alive = true;
    queryThread = thread(&PRadHVSystem::queryLoop, this);
}

void PRadHVSystem::StopMonitor()
{
    alive = false;
    if(queryThread.joinable())
        queryThread.join();
}

void PRadHVSystem::queryLoop()
{
    unsigned int loopCount = 0;
    while(alive)
    {
        if(!(loopCount%60))
            ReadVoltage();
        else if(!(loopCount%15))
            heartBeat();
        ++loopCount;
        this_thread::sleep_for(chrono::seconds(1));
    }
}

void PRadHVSystem::heartBeat()
{
    locker.lock();
    for(auto &crate : crateList)
    {
        if(crate.handle < 0)
            continue;

        char sw[30];
        int err = CAENHV_GetSysProp(crate.handle, "SwRelease", sw);
        showError("HV Heartbeat", err);
    }
    locker.unlock();
}

void PRadHVSystem::SetPowerOn(bool &val)
{
    int err;
    for(auto &crate : crateList)
    {
        for(auto &board : crate.boardList)
        {
            int size = board.nChan;
            unsigned short list[size];
            unsigned int valList[size];
            for(int k = 0; k < size; ++k)
            {
                valList[k] = (val)?1:0;
                list[k] = k;
            }

            err = CAENHV_SetChParam(crate.handle, board.slot, "Pw", size, list, valList);
            showError("HV Power On", err);
        }
    }
}

void PRadHVSystem::SetPowerOn(ChannelAddress &config, bool &val)
{
    for(auto &crate : crateList)
    {
        if(crate.id == config.crate) {
            unsigned int value = (val)?1:0;
            unsigned short slot = (unsigned short) config.slot;
            unsigned short channel = (unsigned short) config.channel;
            int err = CAENHV_SetChParam(crate.handle, slot, "Pw", 1, &channel, &value);
            showError("HV Power On", err);
            return;
        }
    }
}

void PRadHVSystem::SetVoltage(const char *name, ChannelAddress &config, float &val)
{
    // set voltage limit
    if(val > getLimit(name)) {
        cerr << "Exceeds safe value in setting the voltage, ignore the action!" << endl;
        return;
    }

    // TODO, check the primary channel voltage
    for(auto &crate : crateList)
    {
        if(crate.id == config.crate) {
            float value = val;
            unsigned short slot = (unsigned short) config.slot;
            unsigned short channel = (unsigned short) config.channel;
            int err = CAENHV_SetChParam(crate.handle, slot, "V0Set", 1, &channel, &value);
            showError("HV Set Voltage", err);
            return;
        }
    }
}

void PRadHVSystem::ReadVoltage()
{
    locker.lock();
    int err;
    CAENHVData hvData;
    for(auto &crate : crateList)
    {
        if(crate.handle < 0)
            continue;

        hvData.config.crate = crate.id;
        for(auto &board : crate.boardList)
        {
            int size = board.nChan;
            float monVals[size], setVals[size];
            unsigned int pwON[size];
            char nameList[size][MAX_CH_NAME];
            unsigned short list[size];

            for(int k = 0; k < size; ++k)
                list[k] = k;

            err = CAENHV_GetChName(crate.handle, board.slot, size, list, nameList);
            showError("HV Read Voltage", err);

            err = CAENHV_GetChParam(crate.handle, board.slot, "Pw", size, list, pwON);
            showError("HV Read Voltage", err);

            err = CAENHV_GetChParam(crate.handle, board.slot, "VMon", size, list, monVals);
            showError("HV Read Voltage", err);

            err = CAENHV_GetChParam(crate.handle, board.slot, "V0Set", size, list, setVals);
            showError("HV Read Voltage", err);

            hvData.config.slot = (unsigned char)board.slot;
            for(int k = 0; k < size; ++k)
            {
                    hvData.config.channel = (unsigned char)k;
                    hvData.name = nameList[k];
                    hvData.ON = (bool)pwON[k];
                    hvData.Vmon = monVals[k];
                    hvData.Vset = setVals[k];

                    checkVoltage(hvData);

                    myHandler->FeedData(hvData);
            }

        }
    }
    locker.unlock();
}

void PRadHVSystem::PrintOut()
{
    cout << crateList.size() << " high voltage crates connected." << endl;
    cout << "==========================================" << endl;
    for(auto &crate : crateList)
    {
        cout << crate.name << "@" << crate.ip
             << " - " << crate.boardList.size()
             << " boards detected."
             << endl;
        for(auto &board : crate.boardList)
        {
            cout << board << endl;
        }
        cout << "======================================" << endl;
    }
}

void PRadHVSystem::checkVoltage(const CAENHVData &hvData)
{
    if(hvData.name.at(0) == 'G' || hvData.name.at(0) == 'W' || hvData.name.at(0) == 'L') {
        float diff = (hvData.ON)?(hvData.Vmon - hvData.Vset):0;
        if(diff > 15 || diff < -15) {
            cerr << "WARNING: voltage deviates from set value!" << endl
                 << hvData << endl;
        }
    }

    float limit = getLimit(hvData.name.c_str());
    if(hvData.Vmon > limit) {
        cerr << "WARNING: voltage exceeds safe limit!" << endl
             << hvData << endl;
    }
}

float PRadHVSystem::getLimit(const char *name)
{
    if(name[0] == 'G') return 1700;
    if(name[0] == 'W') return 1200;
    if(name[0] == 'L') return 2000;
    if(name[0] == 'S') return 2000;
    if(name[0] == 'P') return 3000;
    return 1200;
}

void PRadHVSystem::showError(const string &prefix, const int &err, ShowErrorType type)
{
    if(err == CAENHV_OK && type != ShowAll)
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
