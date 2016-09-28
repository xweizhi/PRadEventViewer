//============================================================================//
// PRad Cluster Reconstruction Class, different reconstruction methods can be //
// implemented  according to pratical needs                                   //
//                                                                            //
// Weizhi Xiong, Chao Peng                                                    //
// 06/10/2016                                                                 //
//============================================================================//

#include <cmath>
#include <iostream>
#include <iomanip>
#include "PRadReconstructor.h"
#include "PRadDataHandler.h"
#include "PRadDAQUnit.h"
#include "PRadTDCGroup.h"
#include "PRadIslandWrapper.h"

using namespace std;

//____________________________________________________________
PRadReconstructor::PRadReconstructor(PRadDataHandler *h)
: fHandler(h), fIsland(new PRadIslandWrapper(h))
{
    fMoliereCrystal = 20.5;
    fMoliereLeadGlass = 38.2;
    fMoliereRatio = fMoliereLeadGlass/fMoliereCrystal;
    fBaseR = 60.;
    fCorrectFactor = 1.;
    fMaxNCluster = 10;
    fMinClusterCenterE = 10.;
    fMinClusterE = 50.;
    fHighestModuleID = 0;
}

//____________________________________________________________
PRadReconstructor::~PRadReconstructor()
{
    delete fIsland;
}

//____________________________________________________________
void PRadReconstructor::Clear()
{
    fHyCalHit.clear();
    fClusterCenterID.clear();
}

//____________________________________________________________
void PRadReconstructor::SetHandler(PRadDataHandler *h)
{
    fHandler = h;
    fIsland->SetHandler(h);
}

//____________________________________________________________
void PRadReconstructor::InitConfig(const string &path)
{
    ConfigParser c_parser(": ,\t"); // self-defined splitters

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

    fMaxNCluster = GetConfigValue("MAX_N_CLUSTER").Int();
    fMinClusterCenterE = GetConfigValue("MIN_CLUSTER_CENTER_E").Double();
    fMinClusterE = GetConfigValue("MIN_CLUSTER_E").Double();
}

ConfigValue PRadReconstructor::GetConfigValue(const string &name)
{
    auto it = fConfigMap.find(name);
    if(it == fConfigMap.end())
        return ConfigValue();
    return it->second;
}
//________________________________________________________________
vector<HyCalHit> &PRadReconstructor::CoarseHyCalReconstruct(const int &event_index)
{
    EventData &event = fHandler->GetEvent(event_index);
    return CoarseHyCalReconstruct(event);
}

vector<HyCalHit> &PRadReconstructor::CoarseHyCalReconstruct(EventData &event)
{
    Clear(); // clear all the saved buffer before analyzing the next event

    // do no reconstruction for non-physics event
    // otherwise there will be some mess from LMS events
    if(!event.is_physics_event())
        return fHyCalHit;

    fHandler->ChooseEvent(event);
    return Reconstruct_fivebyfive();
}

vector<HyCalHit> &PRadReconstructor::IslandReconstruct(const int &event_index)
{
    EventData &event = fHandler->GetEvent(event_index);
    return IslandReconstruct(event);
}

vector<HyCalHit> &PRadReconstructor::IslandReconstruct(EventData &event)
{
    Clear();

    hycalcluster_t *hclusters = fIsland->GetHyCalCluster(event);
    int NC = fIsland->GetNHyCalClusters();

    for(int i = 0; i < NC; ++i)
    {
        fHyCalHit.emplace_back(hclusters[i].x1, hclusters[i].y1, hclusters[i].E);
    }

    return fHyCalHit;
}

vector<HyCalHit> &PRadReconstructor::Reconstruct_fivebyfive()
{
    double weightX = 0.;
    double weightY = 0.;
    double totalWeight = 0.;
    auto fModuleList = fHandler->GetChannelList();

    for (unsigned short i = 0; i < fMaxNCluster; ++i)
    {
        unsigned short theMaxModuleID = getMaxEChannel();
        if (theMaxModuleID == 0xffff)
            break;//this happens if no module has large enough energy

        double clusterEnergy = 0.;
        PRadDAQUnit::ChannelType thisType = fModuleList.at(theMaxModuleID)->GetType();
        vector<unsigned short> collection = findCluster(theMaxModuleID, clusterEnergy);

        if ((clusterEnergy <= fMinClusterE) ||
            (thisType == PRadDAQUnit::LeadTungstate && collection.size() <= 3) ||
            (thisType == PRadDAQUnit::LeadGlass && collection.size() <= 1))
        {
            --i;
            continue;
        }

        vector<unsigned short> clusterTime = GetTimeForCluster(theMaxModuleID);

        weightX = 0.;
        weightY = 0.;
        totalWeight = 0.;

        for (unsigned short j=0; j<collection.size(); ++j)
        {
            unsigned short thisID = collection.at(j);
            PRadDAQUnit* thisModule = fModuleList.at(thisID);
            if (!thisModule->IsHyCalModule())
                continue;

            double thisX = thisModule->GetX();
            double thisY = thisModule->GetY();

            double weight = thisModule->GetEnergy()/clusterEnergy;
            if (useLogWeight(thisX, thisY))
                weight = 4.6 + log(weight);

            if (weight > 0.) {
                weightX += weight*thisX;
                weightY += weight*thisY;
                totalWeight += weight;
            }
        }

        double hitX = weightX/totalWeight;
        double hitY = weightY/totalWeight;
        fHyCalHit.emplace_back(hitX, hitY, clusterEnergy, clusterTime);
    }

    return fHyCalHit;
}
//____________________________________________________________
unsigned short PRadReconstructor::getMaxEChannel()
{
    double theMaxValue = 0;
    bool foundNewCenter = false;
    unsigned short theMaxChannelID = 0xffff;
    auto fModuleList = fHandler->GetChannelList();

    for (unsigned int i = 0; i < fModuleList.size(); ++i)
    {
        // if have not found the module with maxmimum energy then find it
        // otherwise check if the next maximum is too close to the existing center
        PRadDAQUnit* thisModule = fModuleList.at(i);
        if (!thisModule->IsHyCalModule())
            continue;

        if (thisModule->GetEnergy() < fMinClusterCenterE)
            continue;

        double theClusterRadius = fBaseR;
        if (thisModule->GetType() == PRadDAQUnit::LeadGlass)
            theClusterRadius = fMoliereRatio*fBaseR;

        double distance = 1e4;
        bool merged = false;
        for (unsigned int j = 0; j < fClusterCenterID.size(); ++j){
            PRadDAQUnit* lastCenterModule = fModuleList.at( fClusterCenterID.at(j) );
            distance = Distance( thisModule, lastCenterModule ) ;
            if (distance < 2*theClusterRadius) {
                merged = true;
                continue;
            }
        }

        if (merged)
            continue;

        if (thisModule->GetEnergy() > theMaxValue) {
            foundNewCenter = true;
            theMaxValue = thisModule->GetEnergy();
            theMaxChannelID = i;
        }
    }

    if (foundNewCenter)
    fClusterCenterID.push_back(theMaxChannelID);

    return theMaxChannelID;
}
//__________________________________________________________________________________________
inline bool PRadReconstructor::useLogWeight(double /*x*/, double /*y*/)
{
    return true; //for now
}
//___________________________________________________________________________________________
vector<unsigned short> PRadReconstructor::findCluster(unsigned short centerID, double &clusterEnergy)
{
    auto fModuleList = fHandler->GetChannelList();
    double clusterRadius = 0.;
    double centerX = fModuleList.at(centerID)->GetX();
    double centerY = fModuleList.at(centerID)->GetY();
    vector<unsigned short> collection;

    for (unsigned int i = 0; i < fModuleList.size(); ++i)
    {
        PRadDAQUnit* thisModule = fModuleList.at(i);
        if (!thisModule->IsHyCalModule())
            continue;
        if (thisModule->GetType() == 1) {
            clusterRadius = fBaseR;
        } else {
            clusterRadius = fBaseR*fMoliereRatio;
        }

        if ( thisModule->GetEnergy() > 0. &&
             Distance( thisModule->GetX(), thisModule->GetY(), centerX, centerY ) <= clusterRadius )
        {
            clusterEnergy += thisModule->GetEnergy();
            collection.push_back(i);
        }
    }

    return collection;
}
//___________________________________________________________________________________________
double PRadReconstructor::Distance(PRadDAQUnit *u1, PRadDAQUnit *u2)
{
    double x_dis = u1->GetX() - u2->GetX();
    double y_dis = u1->GetY() - u2->GetY();
    return sqrt( x_dis*x_dis + y_dis*y_dis);
}
//___________________________________________________________________________________________
double PRadReconstructor::Distance(const double &x1, const double &y1, const double &x2, const double &y2)
{
    return sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) );
}
//___________________________________________________________________________________________
double PRadReconstructor::Distance(const double &x1, const double &y1, const double &x2, const double &y2, const double &z1, const double &z2)
{
    return sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2) );
}
//___________________________________________________________________________________________
double PRadReconstructor::Distance(const vector<double> &p1, const vector<double> &p2)
{
    if(p1.size() != p2.size()) {
        cerr << "Dimension is different, failed to calculate distance between two points!" << endl;
        return 0.;
    }

    double quadratic_sum = 0.;
    for(size_t i = 0; i < p1.size(); ++i)
    {
        double quadratic = p1.at(i) - p2.at(i);
        quadratic *= quadratic;
        quadratic_sum += quadratic;
    }

    return sqrt(quadratic_sum);
}
//________________________________________________________________________________________________
vector<unsigned short> & PRadReconstructor::GetTimeForCluster(unsigned short channelID)
{
  PRadTDCGroup* thisGroup = fHandler->GetTDCGroup(fHandler->GetChannel(channelID)
                                                  ->GetTDCName());
  return thisGroup->GetTimeMeasure();
}
//________________________________________________________________________________________________
unsigned short PRadReconstructor::FindClusterCenterModule(double x, double y)
{
    auto fModuleList = fHandler->GetChannelList();
    for (unsigned short i=0; i<fModuleList.size(); i++){
       PRadDAQUnit::Geometry thisGeometry = fModuleList.at(i)->GetGeometry();
       if ( fabs(x - thisGeometry.x) < thisGeometry.size_x/2. &&
            fabs(y - thisGeometry.y) < thisGeometry.size_y/2. ) { return i; }
    }
    return 0xffff;

}





























