//============================================================================//
// A C++ wrapper class for C based ET                                         //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include "PRadETChannel.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>


PRadETChannel::PRadETChannel(size_t size)
: etID(nullptr), bufferSize(size)
{
    buffer = new uint32_t[bufferSize];
}

PRadETChannel::~PRadETChannel()
{
    if(buffer != nullptr)
        delete[](buffer), buffer=nullptr;

    // force close ET
    ForceClose();
}

void PRadETChannel::ForceClose()
{
    if(etID != nullptr && et_alive(etID))
    {
        et_forcedclose(etID);
        etID = nullptr;
    }
}

void PRadETChannel::Open(const char* ipAddr, int tcpPort, const char* etFile) throw(PRadException)
{
    // ET is on 129.57.167.225 (prad.jlab.org)
    et_open_config_sethost(openConf.config, ipAddr);
    et_open_config_setserverport(openConf.config, tcpPort);
    // Use a direct connection to the ET system
    et_open_config_setcast(openConf.config, ET_DIRECT);

    int charSize = strlen(etFile)+1;
    char *fileName = new char[charSize];
    strncpy(fileName, etFile, charSize);

    // Open et client
    int status = et_open(&etID, fileName, openConf.config);
    delete fileName;

    if(status != ET_OK) {
        throw(PRadException(PRadException::ET_CONNECT_ERROR, "et_client: cannot open et client!"));
    }
}


void PRadETChannel::CreateStation(std::string stName, int mode) throw(PRadException)
{
    if (etID == nullptr) {
        throw(PRadException(PRadException::ET_STATION_CREATE_ERROR, "et_client: cannot create station without opening a ET client!"));
    }

    // Generic settings
    et_statconfig sconfig;
    et_station_config_init(&sconfig);
    et_station_config_setuser(sconfig, ET_STATION_USER_MULTI);
    et_station_config_setrestore(sconfig, ET_STATION_RESTORE_OUT);
    et_station_config_setprescale(sconfig, 1);
    et_station_config_setcue(sconfig, ET_CHUNK_SIZE);

    // TODO, change to meaningful settings
    int selections[] = {17,15,-1,-1};
    char fName[] = "et_my_function";
    char libName[] = "libet_user.so";

    // some pre-defined settings
    // TODO, make these settings selectable
    switch(mode)
    {
    case 1:
        et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
        et_station_config_setblock(sconfig, ET_STATION_BLOCKING);
        break;
    case 2:
        et_station_config_setselect(sconfig, ET_STATION_SELECT_ALL);
        et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
        break;
    case 3:
        et_station_config_setselect(sconfig, ET_STATION_SELECT_MATCH);
        et_station_config_setblock(sconfig, ET_STATION_BLOCKING);
        et_station_config_setselectwords(sconfig, selections);
        break;
    case 4:
        et_station_config_setselect(sconfig, ET_STATION_SELECT_MATCH);
        et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
        et_station_config_setselectwords(sconfig, selections);
        break;
    case 5:
        et_station_config_setselect(sconfig, ET_STATION_SELECT_USER);
        et_station_config_setblock(sconfig, ET_STATION_BLOCKING);
        et_station_config_setselectwords(sconfig, selections);
        if (et_station_config_setfunction(sconfig, fName) == ET_ERROR) {
            throw(PRadException(PRadException::ET_STATION_CONFIG_ERROR, "et_client: cannot set function!"));
        }
        if (et_station_config_setlib(sconfig, libName) == ET_ERROR) {
            throw(PRadException(PRadException::ET_STATION_CONFIG_ERROR, "et_client: cannot set library!"));
        }
        break;
    case 6:
        et_station_config_setselect(sconfig, ET_STATION_SELECT_USER);
        et_station_config_setblock(sconfig, ET_STATION_NONBLOCKING);
        et_station_config_setselectwords(sconfig, selections);
        if (et_station_config_setfunction(sconfig, fName) == ET_ERROR) {
            throw(PRadException(PRadException::ET_STATION_CONFIG_ERROR, "et_client: cannot set function!"));
        }
        if (et_station_config_setlib(sconfig, libName) == ET_ERROR) {
            throw(PRadException(PRadException::ET_STATION_CONFIG_ERROR, "et_client: cannot set library!"));
        }
        break;
    }

    /* set level of debug output */
    et_system_setdebug(etID, ET_DEBUG_INFO);

    int status;
    char *stationName = new char[stName.size()+1];
    strcpy(stationName,stName.c_str());

    /* create the station */
    status = et_station_create(etID, &stationID, stationName,sconfig);

    delete stationName;
    et_station_config_destroy(sconfig);

    if (status < ET_OK) {
        if (status == ET_ERROR_EXISTS) {
            /* stationID contains pointer to existing station */;
            throw(PRadException(PRadException::ET_STATION_CREATE_ERROR, "et_client: station already exists!"));
        } else if (status == ET_ERROR_TOOMANY) {
            throw(PRadException(PRadException::ET_STATION_CREATE_ERROR, "et_client: too many stations created!"));
        } else {
            throw(PRadException(PRadException::ET_STATION_CREATE_ERROR, "et_client: error in station creation!"));
        }
    }
}

void PRadETChannel::AttachStation() throw(PRadException)
{
    /* attach to the newly created station */
    if (et_station_attach(etID, stationID, &attachID) < 0) {
        throw(PRadException(PRadException::ET_STATION_ATTACH_ERROR, "et_client: error in station attach!"));
    }
    std::cout << "Successfully attached to ET!" << std::endl;
}

bool PRadETChannel::Read() throw(PRadException)
{
    // read the event in ET

    // check if et is opened or alive
    if (etID == nullptr || !et_alive(etID))
        throw(PRadException(PRadException::ET_READ_ERROR,"et_client: et is not opened or dead!"));

    // get the event
    int status = et_event_get(etID, attachID, &etEvent, ET_ASYNC, nullptr);

    switch(status)
    {
    case ET_OK:
        break;
    case ET_ERROR_EMPTY:
        return false;
    case ET_ERROR_DEAD:
        throw(PRadException(PRadException::ET_READ_ERROR,"et_client: et is dead!"));
     case ET_ERROR_TIMEOUT:
        throw(PRadException(PRadException::ET_READ_ERROR,"et_client: got timeout!!"));
     case ET_ERROR_BUSY:
        throw(PRadException(PRadException::ET_READ_ERROR,"et_client: station is busy!"));
     case ET_ERROR_WAKEUP:
        throw(PRadException(PRadException::ET_READ_ERROR,"et_client: someone told me to wake up."));
     default:
        throw(PRadException(PRadException::ET_READ_ERROR,"et_client: unkown error!"));
     }

    // copy the data buffer
    void *data;
    et_event_getdata(etEvent,&data);
    et_event_getlength(etEvent, &bufferSize);

    memcpy(buffer,(uint32_t*)data,bufferSize);

    // put back the event
    status = et_event_put(etID, attachID, etEvent);

    switch(status)
    {
    case ET_OK:
        break;
    case ET_ERROR_DEAD:
        throw(PRadException(PRadException::ET_READ_ERROR,"et_client: et is dead!"));
    default:
        throw(PRadException(PRadException::ET_READ_ERROR,"et_client: unkown error!"));
    }

    return true;
}


// nested config classes
PRadETChannel::OpenConfig::OpenConfig()
{
    et_open_config_init(&config);
}

PRadETChannel::OpenConfig::~OpenConfig()
{
    et_open_config_destroy(config);
}


PRadETChannel::StationConfig::StationConfig()
{
    et_station_config_init(&config);
}

PRadETChannel::StationConfig::~StationConfig()
{
    et_station_config_destroy(config);
}

