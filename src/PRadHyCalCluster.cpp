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
#include "PRadHyCalCluster.h"

using namespace std;

PRadHyCalCluster::PRadHyCalCluster(PRadDataHandler *h)
: fHandler(h), fNHyCalClusters(0)
{
}

PRadHyCalCluster::~PRadHyCalCluster()
{
}

void PRadHyCalCluster::Clear()
{
    fNHyCalClusters = 0;
}

void PRadHyCalCluster::SetHandler(PRadDataHandler *h)
{
    fHandler = h;
}

void PRadHyCalCluster::ReadConfigFile(const string &path)
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

        string var_name;
        ConfigValue var_value;
        c_parser >> var_name >> var_value;
        fConfigMap[var_name] = var_value;
    }
}

ConfigValue PRadHyCalCluster::GetConfigValue(const string &name,
                                              const string &def_value,
                                              bool verbose)
{
    auto it = fConfigMap.find(name);
    if(it == fConfigMap.end())
    {
        if(verbose)
            cout << name
                 << " not defined in configuration file, set to default value "
                 << def_value
                 << endl;
        return ConfigValue(def_value);
    }
    return it->second;
}

void PRadHyCalCluster::Configure(const string & /*path*/)
{
    // to be implemented by methods
}

void PRadHyCalCluster::Reconstruct(EventData & /*event*/)
{
    // to be implemented by methods
}
