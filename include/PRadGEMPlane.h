#ifndef PRAD_GEM_PLANE_H
#define PRAD_GEM_PLANE_H

#include <list>
#include <vector>
#include <string>

class PRadGEMDetector;
class PRadGEMAPV;

struct GEMPlaneHit
{
    int strip;
    double charge;

    GEMPlaneHit() : strip(0), charge(0.) {};
    GEMPlaneHit(const int &s, const double &c) : strip(s), charge(c) {};
};

struct GEMPlaneCluster
{
    double position;
    double charge;
    std::vector<GEMPlaneHit> hits;

    GEMPlaneCluster(): position(0.), charge(0.) {};

    GEMPlaneCluster(const std::vector<GEMPlaneHit> &p)
    : position(0.), charge(0.), hits(p)
    {};

    GEMPlaneCluster(std::vector<GEMPlaneHit> &&p)
    : position(0.), charge(0.), hits(std::move(p))
    {};
};

class PRadGEMPlane
{
public:
    enum PlaneType
    {
        Plane_X,
        Plane_Y,
        Plane_Max
    };

public:
    PRadGEMPlane();
    PRadGEMPlane(const std::string &n, const PlaneType &t, const double &s,
                 const int &c, const int &o);
    PRadGEMPlane(PRadGEMDetector *d, const std::string &n, const PlaneType &t,
                 const double &s, const int &c, const int &o);
    virtual ~PRadGEMPlane();

    void ConnectAPV(PRadGEMAPV *apv);
    void DisconnectAPV(const size_t &plane_index);
    double GetStripPosition(const int &plane_strip);
    double GetMaxCharge(const std::vector<float> &charges);
    double GetIntegratedCharge(const std::vector<float> &charges);
    void AddPlaneHit(const int &plane_strip, const std::vector<float> &charges);
    void ClearPlaneHits();
    void CollectAPVHits();
    void ClusterHits();

    // set parameter
    void SetDetector(PRadGEMDetector *det) {detector = det;};
    void SetName(const std::string &n) {name = n;};
    void SetType(const PlaneType &t) {type = t;};
    void SetSize(const double &s) {size = s;};
    void SetCapacity(const int &c);
    void SetOrientation(const int &o) {orientation = o;};

    // get parameter
    PRadGEMDetector *GetDetector() {return detector;};
    std::string &GetName() {return name;};
    PlaneType &GetType() {return type;};
    double &GetSize() {return size;};
    int &GetCapacity() {return connector;};
    int &GetOrientation() {return orientation;};
    std::vector<PRadGEMAPV*> GetAPVList();
    std::vector<GEMPlaneHit> &GetPlaneHits();
    std::list<GEMPlaneCluster> &GetPlaneCluster() {return cluster_list;};

private:
    void filterClusters();
    void splitClusters();
    bool filterCluster(const GEMPlaneCluster &c);
    bool splitCluster(GEMPlaneCluster &c, GEMPlaneCluster &c1);

private:
    PRadGEMDetector *detector;
    std::string name;
    PlaneType type;
    double size;
    int connector;
    int orientation;
    std::vector<PRadGEMAPV*> apv_list;
    std::vector<GEMPlaneHit> hit_list;
    // there will be requent remove, split operations for clusters in the middle
    // thus use list instead of vector
    std::list<GEMPlaneCluster> cluster_list;
};

#endif
