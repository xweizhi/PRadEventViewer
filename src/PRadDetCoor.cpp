#include "PRadDetCoor.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <list>
#include <cassert>

PRadDetCoor::PRadDetCoor()
: fGEMOriginShift(0.), fBeamOffsetX(0.), fBeamOffsetY(0.),
  fGEMOffsetX(0.), fGEMOffsetY(0.), fHyCalOffsetX(0.),fHyCalOffsetY(0.)
{

}
//_______________________________________________________________________
PRadDetCoor::~PRadDetCoor()
{
}
//_______________________________________________________________________
void PRadDetCoor::ReadConfigFile(const string &path)
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
ConfigValue PRadDetCoor::GetConfigValue(const string &name,
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
void PRadDetCoor::Configurate(const string & path)
{
    ReadConfigFile(path);

    fGEMZ[0]           = GetConfigValue("GEM1_Z", "5304.").Float();
    fGEMZ[1]           = GetConfigValue("GEM2_Z", "5264.").Float();
    fHyCalZ            = GetConfigValue("HYCAL_Z", "5817.").Float();
    fGEMOriginShift    = GetConfigValue("GEM_ORIGIN_SHIFT", "253.2").Float();
    fZLGToPWO          = GetConfigValue("Z_LG_TO_PWO", "-101.2").Float();

    //the following should probably go to a seperated function because
    //each run gets a different offset value
    fBeamOffsetX       = GetConfigValue("BEAM_OFFSET_X", "0.").Float();
    fBeamOffsetY       = GetConfigValue("BEAM_OFFSET_Y", "0.").Float();
    fGEMOffsetX        = GetConfigValue("GEM_OFFSET_X", "0.").Float();
    fGEMOffsetY        = GetConfigValue("GEM_OFFSET_Y", "0.").Float();
    fHyCalOffsetX      = GetConfigValue("HYCAL_OFFSET_X", "0.").Float();
    fHyCalOffsetY      = GetConfigValue("HYCAL_OFFSET_Y", "0.").Float();
}
//_________________________________________________________________________
void PRadDetCoor::HyCalClustersToLab(int &nclusters, HyCalHit* clusters)
{
    //simply transform HyCal hits from internal coordinate to lab

    for (int i=0; i<nclusters; i++){
        clusters[i].x     = CoordinateTransform(kHyCalX, clusters[i].x);
        clusters[i].x_log = CoordinateTransform(kHyCalX, clusters[i].x_log);
        clusters[i].y     = CoordinateTransform(kHyCalY, clusters[i].y);
        clusters[i].y_log = CoordinateTransform(kHyCalY, clusters[i].y_log);
    }
}
//__________________________________________________________________________
template<class T> void PRadDetCoor::HyCalClustersToLab(int &nclusters, T* x, T*y)
{
    //template function for transfroming HyCal hit in array format

    for (int i=0; i<nclusters; i++){
        x[i] = CoordinateTransform(kHyCalX, x[i]);
        y[i] = CoordinateTransform(kHyCalY, y[i]);
    }
}
//__________________________________________________________________________
void PRadDetCoor::GEMClustersToLab(int type, list<GEMPlaneCluster>& clusters)
{
    for (list<GEMPlaneCluster>::iterator i = clusters.begin();
         i != clusters.end(); i++)
        (*i).position = CoordinateTransform((CoordinateType)type, (*i).position);
}
//________________________________________________________________________
template<class T> void PRadDetCoor::GEMClustersToLab(int type, int &nclusters, T* pos)
{
    //template function for transforming GEM hits in array format

    for (int i=0; i<nclusters; i++)
    pos[i] = CoordinateTransform((CoordinateType)type, pos[i]);
}
//________________________________________________________________________
template <class T> inline T PRadDetCoor::CoordinateTransform(CoordinateType type, T& coor)
{
    switch (type) {
        case kHyCalX:
        return coor + fHyCalOffsetX - fBeamOffsetX;
        break;

        case kHyCalY:
        return coor + fHyCalOffsetY - fBeamOffsetY;
        break;

        case kGEM1X:
        return coor - fGEMOriginShift + fGEMOffsetX   - fBeamOffsetX;
        break;

        case kGEM1Y:
        return -1*coor + fGEMOffsetY   - fBeamOffsetY;
        break;

        case kGEM2X:
        return -1*coor + fGEMOriginShift - fBeamOffsetX;
        break;

        case kGEM2Y:
        return coor - fBeamOffsetY;
        break;

        default:
        cout<<"should never happen, call expert"<<endl;
        break;
    }
    return 0.;
}
//____________________________________________________________________________
template<class T> void PRadDetCoor::LinesIntersect(const T* xa, const T* ya, const T* xb,
                                                   const T* yb, const T* x,  const T* y,
                                                   int ndim)
{
    //finding the intersection point (x, y) of two straight line in 2D space
    //can be used to determine the beam spot position
    assert(ndim == 2);

    T m[2];
    T b[2];

    for (int i=0; i<ndim; i++){
      m[i] = (yb[i] - ya[i]) / (xb[i] - xa[i]);
      b[i] = m[i]*xa[i] - ya[i];
    }

    x = (b[1] - b[0])/(m[0] - m[1]);
    y = (m[1]*b[0] - m[0]*b[1])/(m[1] - m[0]);
}















