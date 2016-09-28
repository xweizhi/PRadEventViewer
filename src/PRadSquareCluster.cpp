//============================================================================//
// PRad Cluster Reconstruction Method                                         //
// Reconstruct the cluster within a square 5x5 area                           //
//                                                                            //
// Weizhi Xiong, Chao Peng                                                    //
// 06/10/2016                                                                 //
//============================================================================//

#include <cmath>
#include <iostream>
#include <iomanip>
#include "PRadSquareCluster.h"
#include "PRadDataHandler.h"
#include "PRadDAQUnit.h"
#include "PRadTDCGroup.h"
#include "PRadIslandWrapper.h"

using namespace std;

PRadSquareCluster::PRadSquareCluster(PRadDataHandler *h)
: PRadReconstructor(h)
{
    Configurate();
}

PRadSquareCluster::~PRadSquareCluster()
{
}

PRadSquareCluster::Configurate(const string &c_path)
{
    ReadConfigFile(c_path);

    fMoliereCrystal = GetConfigValue("MOLIERE_CRYSTAL", "20.5").Double();
    fMoliereLeadGlass = GetConfigValue("MOLIERE_LEADGLASS", "38.2").Double();
    fMoliereRatio = fMoliereLeadGlass/fMoliereCrystal;

    fBaseR = GetConfigValue("CLUSTER_SEP_DISTANCE", "60.0").Double();
    fMaxNCluster = GetConfigValue("MAX_N_CLUSTER", "10").Int();
    fMinClusterCenterE = GetConfigValue("MIN_CLUSTER_CENTER_E", "10.0").Double();
    fMinClusterE = GetConfigValue("MIN_CLUSTER_TOTAL_E", "50.0").Double();
}

//________________________________________________________________
vector<HyCalHit> &PRadSquareCluster::CoarseHyCalReconstruct(const int &event_index)
{
    EventData &event = fHandler->GetEvent(event_index);
    return CoarseHyCalReconstruct(event);
}

vector<HyCalHit> &PRadSquareCluster::CoarseHyCalReconstruct(EventData &event)
{
    Clear(); // clear all the saved buffer before analyzing the next event

    // do no reconstruction for non-physics event
    // otherwise there will be some mess from LMS events
    if(!event.is_physics_event())
        return fHyCalHit;

    fHandler->ChooseEvent(event);
    return Reconstruct_fivebyfive();
}

vector<HyCalHit> &PRadSquareCluster::IslandReconstruct(const int &event_index)
{
    EventData &event = fHandler->GetEvent(event_index);
    return IslandReconstruct(event);
}

vector<HyCalHit> &PRadSquareCluster::IslandReconstruct(EventData &event)
{
    Clear();

    HyCalHit* Hits = fIsland->GetHyCalCluster(event);
    int NHits = fIsland->GetNHyCalClusters();

    for(int i = 0; i < NHits; ++i)
    {
        Hits[i].E *= 1e3; // back to MeV
        Hits[i].x *= -1;    // coordinate change to be consistent
        Hits[i].x_log *= -1; // coordinate change to be consistent
        fHyCalHit.push_back(Hits[i]);
    }

    return fHyCalHit;
}

vector<HyCalHit> &PRadSquareCluster::Reconstruct_fivebyfive()
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
unsigned short PRadSquareCluster::getMaxEChannel()
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
inline bool PRadSquareCluster::useLogWeight(double /*x*/, double /*y*/)
{
    return true; //for now
}
//___________________________________________________________________________________________
vector<unsigned short> PRadSquareCluster::findCluster(unsigned short centerID, double &clusterEnergy)
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
double PRadSquareCluster::Distance(PRadDAQUnit *u1, PRadDAQUnit *u2)
{
    double x_dis = u1->GetX() - u2->GetX();
    double y_dis = u1->GetY() - u2->GetY();
    return sqrt( x_dis*x_dis + y_dis*y_dis);
}
//___________________________________________________________________________________________
double PRadSquareCluster::Distance(const double &x1, const double &y1, const double &x2, const double &y2)
{
    return sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) );
}
//___________________________________________________________________________________________
double PRadSquareCluster::Distance(const double &x1, const double &y1, const double &x2, const double &y2, const double &z1, const double &z2)
{
    return sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2) );
}
//___________________________________________________________________________________________
double PRadSquareCluster::Distance(const vector<double> &p1, const vector<double> &p2)
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
vector<unsigned short> & PRadSquareCluster::GetTimeForCluster(unsigned short channelID)
{
  PRadTDCGroup* thisGroup = fHandler->GetTDCGroup(fHandler->GetChannel(channelID)
                                                  ->GetTDCName());
  return thisGroup->GetTimeMeasure();
}
//________________________________________________________________________________________________
PRadDAQUnit *PRadSquareCluster::LocateModule(const double &x, const double &y)
{
    auto fModuleList = fHandler->GetChannelList();

    for(auto &module : fModuleList)
    {
        PRadDAQUnit::Geometry thisGeometry = module->GetGeometry();
        if ( fabs(x - thisGeometry.x) < thisGeometry.size_x/2. &&
             fabs(y - thisGeometry.y) < thisGeometry.size_y/2. )
        {
            return module;
        }
    }

    return nullptr;
}

