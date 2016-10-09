#ifndef PRAD_GEM_PLANE_H
#define PRAD_GEM_PLANE_H

#include <vector>
#include <string>

class PRadGEMDetector;
class PRadGEMAPV;

class PRadGEMPlane
{
public:
    enum PlaneType
    {
        Plane_X,
        Plane_Y,
        Plane_Max
    };

    struct PlaneHit
    {
        int strip;
        double charge;

        PlaneHit() : strip(0), charge(0.) {};
        PlaneHit(const int &s, const double &c) : strip(s), charge(c) {};
    };

    struct PlaneCluster
    {
        std::vector<PlaneHit> hits;

        PlaneCluster() {};
        PlaneCluster(const std::vector<PlaneHit> &p) : hits(p) {};
        PlaneCluster(std::vector<PlaneHit> &&p) : hits(std::move(p)) {};
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
    std::vector<PlaneHit> &GetPlaneHits();
    std::vector<PlaneCluster> &GetPlaneCluster();

private:
    PRadGEMDetector *detector;
    std::string name;
    PlaneType type;
    double size;
    int connector;
    int orientation;
    std::vector<PRadGEMAPV*> apv_list;
    std::vector<PlaneHit> hit_list;
    std::vector<PlaneCluster> cluster_list;
};

#endif
