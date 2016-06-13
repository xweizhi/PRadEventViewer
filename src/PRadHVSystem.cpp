//============================================================================//
// High Voltage control class                                                 //
//                                                                            //
// Chao Peng                                                                  //
// 02/17/2016                                                                 //
//============================================================================//

#include "PRadHVSystem.h"
#include "PRadEventViewer.h"
#include "PRadDataHandler.h"
#include "ConfigParser.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>

#include "HyCalModule.h"

using namespace std;
using namespace CAENHV;

PRadHVSystem::PRadHVSystem(PRadEventViewer *p)
: console(p), alive(false)
{}

PRadHVSystem::~PRadHVSystem()
{
    Disconnect();
    for(auto &crate : crateList)
        delete crate;
}

void PRadHVSystem::AddCrate(const string &name,
                             const string &ip,
                             const unsigned char &id,
                             const CAENHV_SYSTEM_TYPE_t &type,
                             const int &linkType,
                             const string &username,
                             const string &password)
{
    if(alive) {
        cerr << "HV Channel AddCrate Error: HV Channel is alive, cannot add crate" << endl;
        return;
     }

    if(GetCrate(id) || GetCrate(name)) {
        cerr << "Crate " << name 
             << ", id "<< id 
             << " exists in HV system, skip adding it." << endl;
        return;
    }

    CAEN_Crate *newCrate = new CAEN_Crate(id, name, ip, type, linkType, username, password);

    crate_id_map[id] = newCrate;
    crate_name_map[name] = newCrate;

    crateList.push_back(newCrate);
}

void PRadHVSystem::Connect()
{
    locker.lock();
    int try_cnt = 0, fail_cnt = 0;

    cout << "Trying to initialize all HV crates" << endl;

    for(auto &crate : crateList)
    {
        ++ try_cnt;
        if(crate->Initialize()) {
            cout << "Connected to high voltage system "
                 << crate->GetName() << "@" << crate->GetIP()
                 << endl;
        } else {
            ++fail_cnt;
            cerr << "Cannot connect to "
                 << crate->GetName() << "@" << crate->GetIP()
                 << endl;
        }
    }

    cout << "HV crates initialize DONE, tried "
         << try_cnt << " crates,  failed on "
         << fail_cnt << " crates" << endl;

    locker.unlock();
}

void PRadHVSystem::Disconnect()
{
    StopMonitor();

    for(auto &crate : crateList)
    {
        if(crate->DeInitialize()) {
            cout << "Disconnected from high voltage system "
                 << crate->GetName() << "@" << crate->GetIP()
                 << endl;
        } else {
            cerr << "Failed to disconnect from "
                 << crate->GetName() << "@" << crate->GetIP()
                 << endl;
        }
    }
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
            CheckStatus();

        ++loopCount;
        this_thread::sleep_for(chrono::seconds(1));
    }
}

void PRadHVSystem::ReadVoltage()
{
    locker.lock();

    for(auto &crate : crateList)
    {
        crate->ReadVoltage();
    }
    locker.unlock();

    console->Refresh();
}

void PRadHVSystem::CheckStatus()
{
    locker.lock();

    for(auto &crate : crateList)
    {
        crate->CheckStatus();
    }

    locker.unlock();
}

void PRadHVSystem::SaveCurrentSetting(const string &path)
{
    ofstream hv_out(path);
    hv_out << "#" << setw(11) << "crate"
           << setw(8) << "slot"
           << setw(8) << "channel"
           << setw(16) << "name"
           << setw(10) << "VMon"
           << setw(10) << "VSet"
           << endl;

    ReadVoltage(); // update its value

    for(auto &crate : crateList)
    {
        for(auto &board : crate->GetBoardList())
        {
            for(auto &channel : board->GetChannelList())
            {
                hv_out << setw(12) << crate->GetName()
                       << setw(8) << board->GetSlot()
                       << setw(8) << channel->GetChannel()
                       << setw(16) << channel->GetName()
                       << setw(10) << channel->GetVMon()
                       << setw(10) << channel->GetVSet()
                       << endl;
            }
        }
    }
    cout << "Saved the High Voltage Setting to " << path << endl;
    hv_out.close();
}

void PRadHVSystem::RestoreSetting(const string &path)
{
    ConfigParser c_parser;

    c_parser.OpenFile(path);

    while(c_parser.ParseLine())
    {
        if(c_parser.NbofElements() != 6)
            continue;

        string crate_name, channel_name;
        int slot;
        unsigned short channel;
        float VSet;

        crate_name = c_parser.TakeFirst();
        slot = c_parser.TakeFirst().Int();
        channel = c_parser.TakeFirst().UShort();
        channel_name = c_parser.TakeFirst();
        c_parser.TakeFirst(); // VMon, not used
        VSet = c_parser.TakeFirst().Float();

        CAEN_Channel *ch = GetChannel(crate_name, slot, channel);

        if(ch != nullptr) {
            ch->SetName(channel_name);
            ch->SetVoltage(VSet);
        } else {
            cout << "Crate: " << crate_name
                 << "Slot: " << slot
                 << "Channel: " << channel
                 << " is not found!" << endl;
        }
    }

    cout << "Restore the High Voltage Setting from " << path << endl;
    c_parser.CloseFile();
}

void PRadHVSystem::SetVoltage(const ChannelAddress &addr, const float &Vset)
{
    CAEN_Channel *channel = GetChannel(addr.crate, addr.slot, addr.channel);
    if(channel == nullptr)
    {
        cout << "HV System Error: Did not find Channel at "
             << "crate " << addr.crate
             << ", slot " << addr.slot
             << ", channel " << addr.channel
             << endl;
        return;
    }

    channel->SetVoltage(Vset);
}

void PRadHVSystem::SetPower(const bool &on_off)
{
    for(auto crate : crateList)
    {
        crate->SetPower(on_off);
    }
}

void PRadHVSystem::SetPower(const ChannelAddress &addr, const bool &on_off)
{
    CAEN_Channel *channel = GetChannel(addr.crate, addr.slot, addr.channel);
    if(channel == nullptr)
    {
        cout << "HV System Error: Did not find Channel at "
             << "crate " << addr.crate
             << ", slot " << addr.slot
             << ", channel " << addr.channel
             << endl;
        return;
    }

    channel->SetPower(on_off);
}

CAEN_Crate *PRadHVSystem::GetCrate(const string &n)
{
    auto it = crate_name_map.find(n);
    if(it != crate_name_map.end())
        return it->second;
    return nullptr;
}

CAEN_Crate *PRadHVSystem::GetCrate(const int &id)
{
    auto it = crate_id_map.find(id);
    if(it != crate_id_map.end())
        return it->second;
    return nullptr;
}

CAEN_Board *PRadHVSystem::GetBoard(const string &n, const unsigned short &slot)
{
    CAEN_Crate *crate = GetCrate(n);
    if(crate)
        return crate->GetBoard(slot);
    return nullptr;
}

CAEN_Board *PRadHVSystem::GetBoard(const int &id, const unsigned short &slot)
{
    CAEN_Crate *crate = GetCrate(id);
    if(crate)
        return crate->GetBoard(slot);
    return nullptr;
}

CAEN_Channel *PRadHVSystem::GetChannel(const string &n, const unsigned short &slot, const unsigned short &channel)
{
    CAEN_Board *board = GetBoard(n, slot);
    if(board)
        return board->GetChannel(channel);
    return nullptr;
}

CAEN_Channel *PRadHVSystem::GetChannel(const int &id, const unsigned short &slot, const unsigned short &channel)
{
    CAEN_Board *board = GetBoard(id, slot);
    if(board)
        return board->GetChannel(channel);
    return nullptr;
}

PRadHVSystem::Voltage PRadHVSystem::GetVoltage(const string &n, const unsigned short &slot, const unsigned short &channel)
{
    Voltage volt;

    CAEN_Channel *ch = GetChannel(n, slot, channel);

    if(ch) {
        volt.ON = ch->IsTurnedOn();
        volt.Vmon = ch->GetVMon();
        volt.Vset = ch->GetVSet();
    }

    return volt;
}

PRadHVSystem::Voltage PRadHVSystem::GetVoltage(const int &id, const unsigned short &slot, const unsigned short &channel)
{
    Voltage volt;

    CAEN_Channel *ch = GetChannel(id, slot, channel);

    if(ch) {
        volt.ON = ch->IsTurnedOn();
        volt.Vmon = ch->GetVMon();
        volt.Vset = ch->GetVSet();
    }

    return volt;
}
