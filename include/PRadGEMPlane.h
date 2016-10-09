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

public:
    PRadGEMPlane();
    PRadGEMPlane(const std::string &n, const PlaneType &t, const double &s,
                 const int &c, const int &o);
    PRadGEMPlane(PRadGEMDetector *d, const std::string &n, const PlaneType &t,
                 const double &s, const int &c, const int &o);
    virtual ~PRadGEMPlane();

    void ConnectAPV(PRadGEMAPV *apv);
    std::vector<PRadGEMAPV*> &GetAPVList() {return apv_list;};
    double GetStripPosition(const int &plane_strip);

public:
    PRadGEMDetector *detector;
    std::string name;
    PlaneType type;
    double size;
    int connector;
    int orientation;
    std::vector<PRadGEMAPV*> apv_list;
};

#endif
