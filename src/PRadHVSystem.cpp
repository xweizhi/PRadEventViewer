//============================================================================//
// A C++ wrapper class to use CAENHVWrapper library                           //
// It is not finished yet                                                     //
//                                                                            //
// Chao Peng                                                                  //
// 02/17/2016                                                                 //
//============================================================================//

#include "PRadHVSystem.h"
#include "PRadEventViewer.h"
#include "PRadDataHandler.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>

using namespace std;
using namespace CAENHV;

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

string trim(const string& str,
            const string& whitespace = " \t")
{
    const auto strBegin = str.find_first_not_of(whitespace);
    if (strBegin == string::npos)
        return ""; // no content

    const auto strEnd = str.find_last_not_of(whitespace);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

int PRadHVSystem::CAEN_PrimaryChannel::SetPower(bool&& on)
{
    if(!initialized) return 0;
    unsigned int val = on ? 1 : 0;
    return CAENHV_SetChParam(handle, slot, "Pw", 1, &channel, &val);
}

int PRadHVSystem::CAEN_PrimaryChannel::SetVoltage(float&& v)
{
    if(!initialized) return 0;
    float val = v;
    return CAENHV_SetChParam(handle, slot, "V0Set", 1, &channel, &val);
}

PRadHVSystem::PRadHVSystem(PRadEventViewer *p)
: console(p), alive(false)
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

int PRadHVSystem::GetCrateHandle(const string &name)
{
    for(auto &crate : crateList)
    {
        if(crate.name == name)
            return crate.handle;
    }
    return -1;
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
            //TODO, change this hard coded exception
            if(crate.id == 5 && slot == 14)
                continue;
            CAEN_Board newBoard(m, d, slot, NbofChList[slot], serNumList[slot], fmwMinList[slot], fmwMaxList[slot]);
            if(newBoard.model.find("1832") != string::npos)
                newBoard.primaryCh = CAEN_PrimaryChannel(crate.handle, slot);
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
        if(!(loopCount%5))
            ReadVoltage();

        if(!(loopCount%30))
            checkStatus();

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

void PRadHVSystem::checkStatus()
{
    locker.lock();
    int err;
    for(auto &crate : crateList)
    {
        if(crate.handle < 0)
            continue;

        for(auto &board : crate.boardList)
        {
            int size = board.nChan;
            unsigned int status[size];
            unsigned short list[size];
            char nameList[size][MAX_CH_NAME];

            for(int k = 0; k < size; ++k)
            {
                list[k] = k;
                status[k] = 0;
            }
            err = CAENHV_GetChName(crate.handle, board.slot, size, list, nameList);
            showError("HV Read Voltage", err);

            err = CAENHV_GetChParam(crate.handle, board.slot, "Status", size, list, status);
            showError("HV Status Monitor", err);

            for(int i = 0; i < size; ++i)
            {
                showChError(nameList[i], status[i]);
            }
        }
    }
    locker.unlock();
}

void PRadHVSystem::SetPower(const bool &val)
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

void PRadHVSystem::SetPower(const ChannelAddress &config, const bool &val)
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

void PRadHVSystem::SetVoltage(const char *name, const ChannelAddress &config, const float &val)
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
            unsigned short list[size];
            char nameList[size][MAX_CH_NAME];

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

                    console->GetHandler()->FeedData(hvData);
            }

        }
    }
    locker.unlock();
    console->Refresh();
}


void PRadHVSystem::SaveCurrentSetting(const string &path)
{
    ofstream hv_out(path);
    hv_out << "#" << setw(11) << "crate"
           << setw(8) << "slot"
           << setw(8) << "channel"
           << setw(16) << "name"
           << setw(10) << "VSet"
           << endl;
    int err;

    for(auto &crate : crateList)
    {
        if(crate.handle < 0)
            continue;

        for(auto &board : crate.boardList)
        {
            int size = board.nChan;
            float setVals[size];
            unsigned short list[size];
            char nameList[size][MAX_CH_NAME];

            for(int k = 0; k < size; ++k)
                list[k] = k;

            err = CAENHV_GetChName(crate.handle, board.slot, size, list, nameList);
            if(err != CAENHV_OK) {
                showError("HV Read Voltage", err);
                continue;
            }

            err = CAENHV_GetChParam(crate.handle, board.slot, "V0Set", size, list, setVals);
            if(err != CAENHV_OK) {
                showError("HV Read Voltage", err);
                continue;
            }

            for(int k = 0; k < size; ++k)
            {
                hv_out << setw(12) << crate.name
                       << setw(8) << board.slot
                       << setw(8) << k
                       << setw(16) << nameList[k]
                       << setw(10) << setVals[k]
                       << endl;
            }
        }
    }
    cout << "Saved the High Voltage Setting to " << path << endl;
    hv_out.close();
}

void PRadHVSystem::RestoreSetting(const string &path)
{
    SetPower(false);

    ifstream hv_in(path);
    string line, trim_line;

    while(getline(hv_in, line))
    {
        trim_line = trim(line);
        if(trim_line.at(0) == '#' || trim_line.empty())
            continue;
        istringstream iss(trim_line);
        string crate_name, channel_name;
        int handle, slot;
        unsigned short channel;
        float VSet, limit;
        iss >> crate_name >> slot >> channel >> channel_name >> VSet;
        handle = GetCrateHandle(crate_name);
        limit = getLimit(channel_name.c_str());
        if(VSet > limit) {
            cout << "Crate: " << crate_name
                 << ", Slot: "  << slot
                 << ", Channel: "  << channel
                 << ", Name: " << channel_name
                 << ", VSet " << VSet
                 << "V exceeds the limit " << limit
                 << "V, set to limit value." << endl;
            VSet = limit;
        }
        CAENHV_SetChName(handle, slot, 1, &channel, channel_name.c_str());
        CAENHV_SetChParam(handle, slot, "V0Set", 1, &channel, (void*)&VSet);
    }
    cout << "Restore the High Voltage Setting from " << path << endl;
    hv_in.close(); 
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
/* status will tell if it is over voltage or under voltage
    if(hvData.name.at(0) == 'G' || hvData.name.at(0) == 'W' || hvData.name.at(0) == 'L') {
        float diff = (hvData.ON)?(hvData.Vmon - hvData.Vset):0;
        if(diff > 15 || diff < -15) {
            cerr << "WARNING: voltage deviates from set value!" << endl
                 << hvData << endl;
        }
    }
*/
    float limit = getLimit(hvData.name.c_str());
    if(hvData.Vmon > limit) {
        cerr << "WARNING: voltage exceeds safe limit!" << endl
             << hvData << endl;
    }
}

float PRadHVSystem::getLimit(const char *name)
{
    if(name[0] == 'G') return 1700;
    if(name[0] == 'W') return 1300;
    if(name[0] == 'L') return 2000;
    if(name[0] == 'S') return 2000;
    if(name[0] == 'P') return 3000;
    if(name[0] == 'H') return 2000;
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

void PRadHVSystem::showChError(const char *n, const unsigned int &err_bit)
{
    if(err_bit&(1 << 3)) cerr << "Channel " << n << " is in overcurrent!" << endl;
    if(err_bit&(1 << 4)) cerr << "Channel " << n << " is in overvoltage!" << endl;
    if(err_bit&(1 << 5)) cerr << "Channel " << n << " is in undervoltage!" << endl;
    if(err_bit&(1 << 6)) cerr << "Channel " << n << " is in external trip!" << endl;
    if(err_bit&(1 << 7)) cerr << "Channel " << n << " is in max voltage!" << endl;
    if(err_bit&(1 << 8)) cerr << "Channel " << n << " is in external disable!" << endl;
    if(err_bit&(1 << 9)) cerr << "Channel " << n << " is in internal trip!" << endl;
    if(err_bit&(1 << 10)) cerr << "Channel " << n << " is in calibration error!" << endl;
    if(err_bit&(1 << 11)) cerr << "Channel " << n << " is unplugged!" << endl;
    if(err_bit&(1 << 13)) cerr << "Channel " << n << " is in overvoltage protection!" << endl;
    if(err_bit&(1 << 14)) cerr << "Channel " << n << " is in power fail!" << endl;
    if(err_bit&(1 << 15)) cerr << "Channel " << n << " is in temperature error!" << endl;
}
