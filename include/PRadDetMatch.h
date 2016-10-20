//========================================================//
//match the HyCal Clusters and GEM clusters, both of them //
//need to be transformed to the Lab frame in advance      //
//========================================================//

#ifndef PRAD_DET_MATCH_H
#define PRAD_DET_MATCH_H
#include <map>
#include <unordered_map>
#include <list>

#include "PRadEventStruct.h"
#include "PRadGEMPlane.h"
#include "ConfigParser.h"

using namespace std;

class PRadDetMatch
{
public:
    PRadDetMatch();
    ~PRadDetMatch();;

    void ReadConfigFile(const std::string &path);
    ConfigValue GetConfigValue(const std::string &var_name,
                               const std::string &def_value,
                               bool verbose = true);
    void Configurate(const std::string &path);
    void Clear();
    void LoadHyCalClusters(int &nclusters, HyCalHit* clusters);
    void LoadGEMClusters(int type, list<GEMPlaneCluster>& clusters);

    void DetectorMatch();
    void MatchProcessing();
    void ProjectGEMToHyCal();

    //getter for HyCal cluster
    int GetNHyCalClusters() {return fNHyCalClusters;}
    HyCalHit *GetHyCalClusters() {return fHyCalClusters;}

    //getter for GEM 1d clusters
    map<int, list<GEMPlaneCluster>* > & GetGEM1DClusters() { return fGEM1DClusters; }

    //getter for GEM 2D clusters
    map<int, vector<GEMDetCluster> > & GetGEM2DClusters() { return fGEM2DClusters; }

    static void  ProjectToZ(float& x, float& y, float&z, float& zproj);
    static float Distance2Points(float& x1, float& y1, float& x2, float& y2);

protected:
    void  GEMXYMatchNormalMode(int igem);
    void  GEMXYMatchPlusMode(int igem);

    // configuration map
    unordered_map<string, ConfigValue> fConfigMap;

    //analysis flag
    int                  fGEMXYMatchMode;    //0 for plus mode, 1 for normal mode
    int                  fProjectToGEM2;
    int                  fHyCalGEMMatchMode; //0 save all hits within range, 1 save cloest
    int                  fGEMPeakCharge;     //1 use total charge for GEM cluster, 0 peak charge
    int                  fDoMatchProcessing; //1 will do additional processing for matching hits

    //geometric parameters
    float                fGEMZ[NGEM];
    float                fHyCalZ;
    float                fGEMResolution;
    float                fZLGToPWO;

    //analysis parameters
    float                fMatchCut;

    //input and output
    int                           fNHyCalClusters;
    HyCalHit*                     fHyCalClusters;
    map<int,
        list<GEMPlaneCluster>* >  fGEM1DClusters;

    map<int,
        vector<GEMDetCluster> >   fGEM2DClusters;


};

#endif
