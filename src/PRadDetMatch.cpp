#include <list>
#include <cassert>
#include <cmath>

#include "PRadDetMatch.h"
#include "PRadDetCoor.h"

PRadDetMatch::PRadDetMatch()
:fGEMXYMatchMode(0), fProjectToGEM2(0), fHyCalGEMMatchMode(0), fGEMPeakCharge(0),
  fDoMatchProcessing(0), fMatchCut(0.)
{
    for (int i=0; i<NGEM; i++){
        vector<GEMDetCluster> gemHits;
        gemHits.clear();
        fGEM2DClusters[i] = gemHits;
    }
}
//__________________________________________________________________________________
PRadDetMatch::~PRadDetMatch()
{

}
//___________________________________________________________________________________
void PRadDetMatch::Clear()
{
    fNHyCalClusters = 0;
    fHyCalClusters = nullptr;
    fGEM1DClusters.clear();
    map<int, vector<GEMDetCluster> >::iterator it;
    for (it = fGEM2DClusters.begin(); it != fGEM2DClusters.end(); it++) { (it->second).clear(); }
}
//___________________________________________________________________________________
void PRadDetMatch::ReadConfigFile(const string &path)
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
//________________________________________________________________________
ConfigValue PRadDetMatch::GetConfigValue(const string &name,
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
//________________________________________________________________________
void PRadDetMatch::Configurate(const string & path)
{
    ReadConfigFile(path);

    fGEMXYMatchMode    = GetConfigValue("GEM_XY_MATCH_MODE", "0").Int();
    fProjectToGEM2     = GetConfigValue("PROJECT_TO_GEM2", "0").Int();
    fHyCalGEMMatchMode = GetConfigValue("HYCAL_GEM_MATCH_MODE", "0").Int();
    fGEMPeakCharge     = GetConfigValue("GEM_PEAK_CHARGE", "0").Int();
    fDoMatchProcessing = GetConfigValue("MATCH_PROCESSING", "1").Int();

    fMatchCut          = GetConfigValue("MATCH_CUT", "60.").Float();
    fGEMResolution     = GetConfigValue("GEM_RESOLUTION", "0.1").Float();

    fGEMZ[0]           = GetConfigValue("GEM1_Z", "5304.").Float();
    fGEMZ[1]           = GetConfigValue("GEM2_Z", "5264.").Float();
    fHyCalZ            = GetConfigValue("HYCAL_Z", "5817.").Float();
    fZLGToPWO          = GetConfigValue("Z_LG_TO_PWO", "-101.2").Float();
}
//_________________________________________________________________________
void PRadDetMatch::LoadHyCalClusters(int &nclusters, HyCalHit* clusters)
{
    fNHyCalClusters = nclusters;
    fHyCalClusters  = clusters;
}
//________________________________________________________________________
void PRadDetMatch::LoadGEMClusters(int type, list<GEMPlaneCluster>& clusters)
{
    map< int, list<GEMPlaneCluster>* >::iterator it;
    it = fGEM1DClusters.find(type);
    if (it != fGEM1DClusters.end()){
        cout<<"cluster already loaded? make sure you call the Clear function of PRadDetMatch"<<endl;
    }else{
        fGEM1DClusters[type] = &clusters;
    }
}
//_________________________________________________________________________
void PRadDetMatch::DetectorMatch()
{
    //first match the X-Y coordinate of each GEM detector
    for (int i=0; i<NGEM; i++){
         assert(fGEM2DClusters[i].empty()); //must be cleared before filled in
        if (fGEMXYMatchMode == 0){
            GEMXYMatchPlusMode(i);
        }else{
            GEMXYMatchNormalMode(i);
        }
    }

    //for each HyCal cluster, find GEM cluster that may be able to match the coordinate
    //The matching is probably better to be done on a common plane, say coincide with
    //the second GEM, because the cut may be different depending how far the plane is
    //away from the target center

    for (int i=0; i<fNHyCalClusters; i++){
        fHyCalClusters[i].clear_gem();
        float hycalX = fHyCalClusters[i].x_log;
        float hycalY = fHyCalClusters[i].y_log;
        //TODO: check if z position of LG clusters and PWO clusters are the same

        for (int j=0; j<NGEM; j++){
            int countHit = 0;
            //Project HyCal cluster to the GEM plane where matching happens
            ProjectToZ(hycalX, hycalY, fHyCalZ, fGEMZ[j]);
            vector<GEMDetCluster> & cl = fGEM2DClusters[j];
            float rSave = fMatchCut;
            unsigned int idSave = cl.size();

            for (unsigned int k = 0; k<cl.size(); k++){
                float thisR = Distance2Points(hycalX, hycalY, cl.at(k).x, cl.at(k).y);
                if (thisR < fMatchCut) countHit++;

                if (thisR < rSave){
                    //found a potential matching GEM cluster, save the id to HyCal cluster

                    //if match mode is 0, save all hits within range
                    //if match mode is 1, save the cloest one

                    if (fHyCalGEMMatchMode == 0) { fHyCalClusters[i].add_gem_clusterID(j, k); }
                    else{
                        rSave = thisR;
                        idSave = k;
                    }
                }
            }
            if (fHyCalGEMMatchMode !=0 && idSave < cl.size()){
                assert(fHyCalClusters[i].gemNClusters[j] == 0); //should be empty before
                fHyCalClusters[i].add_gem_clusterID(j, idSave);
                fHyCalClusters[i].x_gem = fGEM2DClusters[j].at(idSave).x;
                fHyCalClusters[i].y_gem = fGEM2DClusters[j].at(idSave).y;
                fHyCalClusters[i].z_gem = fGEMZ[j];
            }
            if (countHit > 1) SETBIT(fHyCalClusters[i].flag, kMultiGEMHits);
        }

        if (fHyCalClusters[i].gemNClusters[0] != 0) SETBIT(fHyCalClusters[i].flag, kGEM1Match);
        if (fHyCalClusters[i].gemNClusters[1] != 0) SETBIT(fHyCalClusters[i].flag, kGEM2Match);
    }

    //at the end of this function, each HyCalHit will have two vectors that collect
    //the IDs of GEM clusters that are matchable, the ID coresponds to the array index
    //in the fGEM2DClusters
    if (fDoMatchProcessing) MatchProcessing();
}
//_________________________________________________________________________
void PRadDetMatch::MatchProcessing()
{
    if (fHyCalGEMMatchMode){
        //we have chosen the GEM hit that is closest to HyCal Hit in this mode
        //we will just do additional selection for hits appear in the overlap region
        for (int i=0; i<fNHyCalClusters; i++){
            //if the HyCal Cluster has hits from both GEM

            if (fHyCalClusters[i].gemNClusters[0] == 1 && fHyCalClusters[i].gemNClusters[1] == 1){
                //project every thing to the second GEM for analysis
                int   index[NGEM] = { fHyCalClusters[i].gemClusterID[0][0], fHyCalClusters[i].gemClusterID[1][0]};
                float hycalX = fHyCalClusters[i].x_log;
                float hycalY = fHyCalClusters[i].y_log;
                float GEM1X  =  fGEM2DClusters[0].at(index[0]).x;
                float GEM1Y  =  fGEM2DClusters[0].at(index[0]).y;
                float GEM2X  =  fGEM2DClusters[1].at(index[1]).x;
                float GEM2Y  =  fGEM2DClusters[1].at(index[1]).y;

                ProjectToZ(hycalX, hycalY, fHyCalZ, fGEMZ[1]);
                ProjectToZ(GEM1X, GEM1Y, fGEMZ[0], fGEMZ[1]);

                float r1 = Distance2Points(hycalX, hycalY, GEM1X, GEM1Y);
                float r2 = Distance2Points(hycalX, hycalY, GEM2X, GEM2Y);
                float r3 = Distance2Points(GEM1X, GEM1Y, GEM2X, GEM2Y);
                if (r3 < 10.*fGEMResolution){
                    //likely the two this from the two GEMs are from the same track
                    fHyCalClusters[i].x_gem = (GEM1X + GEM2X)/2.;
                    fHyCalClusters[i].y_gem = (GEM1Y + GEM2Y)/2.;
                    fHyCalClusters[i].z_gem = fGEMZ[1];
                    SETBIT(fHyCalClusters[i].flag, kOverlapMatch);
                }else{
                    //will keep only on of the two hits
                    int isave = r1 < r2 ? 0 : 1;
                    int ikill = (int)(!isave);
                    fHyCalClusters[i].gemNClusters[ikill] = 0;
                    fHyCalClusters[i].x_gem = fGEM2DClusters[isave].at(index[isave]).x;
                    fHyCalClusters[i].y_gem = fGEM2DClusters[isave].at(index[isave]).y;
                    fHyCalClusters[i].z_gem = fGEMZ[isave];
                    if (ikill == 0) { CLRBIT(fHyCalClusters[i].flag, kGEM1Match); }
                    else { CLRBIT(fHyCalClusters[i].flag, kGEM2Match); }
                }

            }
             if (fHyCalClusters[i].gemNClusters[0] > 0 || fHyCalClusters[i].gemNClusters[1] >0 )
             SETBIT(fHyCalClusters[i].flag, kGEMMatch);
        }
    }else{
        //TODO for more advanced GEM and HyCal match processing method
    }
}
//_________________________________________________________________________
void PRadDetMatch::ProjectGEMToHyCal()
{
    //project all the GEM 2D clusters to HyCal surface, used for the purpose
    //of reconstructed event display and and various calibration study for HyCal

    for (map<int, vector<GEMDetCluster> >::iterator it = fGEM2DClusters.begin();
         it != fGEM2DClusters.end(); it++){
        vector<GEMDetCluster> & thisHit = (it->second);
        for (unsigned int i=0; i<thisHit.size(); i++){
            ProjectToZ(thisHit[i].x, thisHit[i].y, thisHit[i].z, fHyCalZ);
        }
    }

    for (int i=0; i<fNHyCalClusters; i++){
        
        ProjectToZ(fHyCalClusters[i].x_gem, fHyCalClusters[i].y_gem,
                   fHyCalClusters[i].z_gem, fHyCalZ);
    }
}
//________________________________________________________________________
void  PRadDetMatch::GEMXYMatchNormalMode(int igem)
{
    //IMPORTANCE: In the normal mode cluster matching, x and y clusters MUST be
    //sorted according to cluster ADC. It is better for it to be done here since
    //if we don't use the Normal mode, it is not necessary to sort the array

    int type = igem*NGEM;
    list<GEMPlaneCluster> & xlist = *fGEM1DClusters[type];
    list<GEMPlaneCluster> & ylist = *fGEM1DClusters[type + 1];

    unsigned int nbCluster = (xlist.size() < ylist.size()) ?
                              xlist.size() : ylist.size();

    if (nbCluster == 0) return;

    list<GEMPlaneCluster>::iterator itx = xlist.begin();
    list<GEMPlaneCluster>::iterator ity = ylist.begin();

    for(unsigned int i = 0;i<nbCluster;i++){
        float x = (*itx).position;
        float y = (*ity).position;
        //project x y to GEM 2 z is requested to do so
        if (fProjectToGEM2) ProjectToZ(x, y, fGEMZ[igem], fGEMZ[1]);
        float c_x = 0.;
        float c_y = 0.;
        if (fGEMPeakCharge){
            c_x = (*itx).total_charge;
            c_y = (*ity).total_charge;
        }else{
            c_x = (*itx).peak_charge;
            c_y = (*ity).peak_charge;
        }

        fGEM2DClusters[igem].push_back(GEMDetCluster(
                                       x, y, fGEMZ[igem],
                                       c_x, c_y,
                                       ((*itx).hits).size(),
                                       ((*ity).hits).size(),
                                       igem
                                       )
                                       );
        *itx++;
        *ity++;
    }
}
//________________________________________________________________________
void  PRadDetMatch::GEMXYMatchPlusMode(int igem)
{
    //Plus mode exhausts all the possible combination of x-y
    int type = igem*NGEM;
    list<GEMPlaneCluster> & xlist = *fGEM1DClusters[type];
    list<GEMPlaneCluster> & ylist = *fGEM1DClusters[type + 1];

    list<GEMPlaneCluster>::iterator itx = xlist.begin();
    list<GEMPlaneCluster>::iterator ity = ylist.begin();

    for (itx = xlist.begin(); itx != xlist.end(); itx++){
        for (ity = ylist.begin(); ity != ylist.end(); ity++){
            float x = (*itx).position;
            float y = (*ity).position;
            //project x y to GEM 2 z is requested to do so
            if (fProjectToGEM2) ProjectToZ(x, y, fGEMZ[igem], fGEMZ[1]);

            float c_x = 0.;
            float c_y = 0.;
            if (fGEMPeakCharge){
                c_x = (*itx).total_charge;
                c_y = (*ity).total_charge;
            }else{
                c_x = (*itx).peak_charge;
                c_y = (*ity).peak_charge;
            }
            fGEM2DClusters[igem].push_back(GEMDetCluster(
                                       x, y, fGEMZ[igem],
                                       c_x, c_y,
                                       ((*itx).hits).size(),
                                       ((*ity).hits).size(),
                                       igem
                                       )
                                       );
        }
    }
}
//_________________________________________________________________________
void PRadDetMatch::ProjectToZ(float& x, float& y, float& z, float&zproj)
{
    x *= zproj/z;
    y *= zproj/z;
}
//_________________________________________________________________________
float PRadDetMatch::Distance2Points(float& x1, float &y1, float &x2, float &y2)
{
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}















