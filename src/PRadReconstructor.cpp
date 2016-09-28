//============================================================================//
// Basic PRad Cluster Reconstruction Class For HyCal                          //
// Different reconstruction methods can be implemented accordingly            //
//                                                                            //
// Chao Peng                                                                  //
// 09/28/2016                                                                 //
//============================================================================//

#include <cmath>
#include <iostream>
#include <iomanip>
#include "PRadReconstructor.h"

using namespace std;

PRadReconstructor::PRadReconstructor(PRadDataHandler *h)
: fHandler(h), fNHyCalClusters(0)
{
}

PRadReconstructor::~PRadReconstructor()
{
}

void PRadReconstructor::Clear()
{
    fNHyCalClusters = 0;
}

void PRadReconstructor::SetHandler(PRadDataHandler *h)
{
    fHandler = h;
}

void PRadReconstructor::ReadConfigFile(const string &path)
{
    ConfigParser c_parser(": ,\t="); // self-defined splitters

    if(!c_parser.OpenFile(path)) {
         cout << "Cannot open file " << path << endl;
     }

    unordered_map<string, float> variable_map;

    while(c_parser.ParseLine())
    {
        if (c_parser.NbofElements() != 2)
            continue;

        string var_name = c_parser.TakeFirst();
        ConfigValue var_value = c_parser.TakeFirst();
        fConfigMap[var_name] = var_value;
    }
}

ConfigValue PRadReconstructor::GetConfigValue(const string &name)
{
    auto it = fConfigMap.find(name);
    if(it == fConfigMap.end())
        return ConfigValue();
    return it->second;
}

void PRadReconstructor::Configurate(const string & /*path*/)
{
    // to be implemented by methods
}

void PRadReconstructor::Reconstruct(EventData & /*event*/)
{
    // to be implemented by methods
}
