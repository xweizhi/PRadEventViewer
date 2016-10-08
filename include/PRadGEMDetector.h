#ifndef PRAD_GEM_DETECTOR_H
#define PRAD_GEM_DETECTOR_H

#include <vector>
#include <string>

class PRadGEMAPV;

class PRadGEMDetector
{
public:
    enum PlaneType
    {
        Plane_X,
        Plane_Y,
        MaxType,
    };

    struct Plane
    {
        std::string name;
        float size;
        int connector;
        int orientation;
        std::vector<PRadGEMAPV*> apv_list;

        Plane()
        : name("Undefined"), size(0.), connector(-1), orientation(0)
        {};

        Plane(const std::string &n, const float &s, const int &c, const int &o)
        : name(n), size(s), connector(c), orientation(o)
        {};
    };

public:
    PRadGEMDetector(const std::string &readoutBoard,
                    const std::string &detectorType,
                    const std::string &detector);
    virtual ~PRadGEMDetector();

    void AddPlane(const PlaneType &type, const Plane &plane);
    void AddPlane(const PlaneType &type, Plane &&plane);
    void AssignID(const int &i);
    void ConnectAPV(const PlaneType &plane, PRadGEMAPV *apv);
    Plane &GetPlane(const PlaneType &type);
    Plane &GetPlaneX();
    Plane &GetPlaneY();
    std::vector<PRadGEMAPV*> &GetAPVList(const PlaneType &type);

    int id;
    std::string name;
    std::string type;
    std::string readout_board;
    Plane planes[MaxType];
};

#endif
