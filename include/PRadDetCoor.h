//============================================================================//
//this class serve purpose of coordinate tranformation between the internal   //
//coordinate system and the lab system (beam spot at the origin)              //
//============================================================================//

#ifndef PRAD_DET_COOR_H
#define PRAD_DET_COOR_H
#include <iostream>
#include <string>
#include <unordered_map>
#include <map>
#include <list>
#include "PRadEventStruct.h"
#include "ConfigParser.h"
#include "PRadGEMPlane.h"

using namespace std;

class PRadDetCoor
{
public:
    enum CoordinateType{
        kGEM1X = 0,
        kGEM1Y,
        kGEM2X,
        kGEM2Y,
        kHyCalX,
        kHyCalY
    };


    PRadDetCoor();
    ~PRadDetCoor();

    void ReadConfigFile(const std::string &path);
    ConfigValue GetConfigValue(const std::string &var_name,
                               const std::string &def_value,
                               bool verbose = true);
    void Configurate(const std::string &path);
    void HyCalClustersToLab(int &nclusters, HyCalHit* clusters);

    float & GetHyCalZ() { return fHyCalZ; }
    float & GetGEMZ(int i) { return fGEMZ[i]; }

    template<class T> void HyCalClustersToLab(int &nclusters, T* x, T*y);

    void GEMClustersToLab(int type, list<GEMPlaneCluster>& clusters);

    template<class T> void GEMClustersToLab(int type, int &nclusters, T* pos);

    template<class T> void LinesIntersect(const T* xa, const T* ya, const T* xb,
                                          const T* yb, const T* x, const T* y, int ndim);

protected:

    template<class T> T CoordinateTransform(CoordinateType type, T & coor);  //transform the coordinate according to type

    // configuration map
    unordered_map<string, ConfigValue> fConfigMap;

    //geometric parameters
    float                fGEMZ[NGEM];
    float                fHyCalZ;
    float                fZLGToPWO;
    //if there is not relative rotation between GEM plane and
    //HyCal, then we only need four parameters to align everything
    //in beam coordinate
    float                fGEMOriginShift;
    float                fBeamOffsetX;
    float                fBeamOffsetY;
    float                fGEMOffsetX;
    float                fGEMOffsetY;
    float                fHyCalOffsetX;
    float                fHyCalOffsetY;
};


#endif
