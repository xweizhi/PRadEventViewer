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
        Plane_Max
    };

    struct Plane
    {
        PRadGEMDetector *detector;
        std::string name;
        float size;
        int connector;
        int orientation;
        std::vector<PRadGEMAPV*> apv_list;

        Plane()
        : detector(nullptr), name("Undefined"), size(0.), connector(-1), orientation(0)
        {};

        Plane(const std::string &n, const float &s, const int &c, const int &o)
        : detector(nullptr), name(n), size(s), connector(c), orientation(o)
        {};

        Plane(PRadGEMDetector *d, const std::string &n, const float &s, const int &c, const int &o)
        : detector(d), name(n), size(s), connector(c), orientation(o)
        {};

        void ConnectAPV(PRadGEMAPV *apv);
        std::vector<PRadGEMAPV*> &GetAPVList() {return apv_list;};
    };

public:
    PRadGEMDetector(const std::string &readoutBoard,
                    const std::string &detectorType,
                    const std::string &detector);
    virtual ~PRadGEMDetector();

    void AddPlane(const PlaneType &type, Plane *plane);
    void AddPlane(const PlaneType &type,
                  const std::string &name, const float &size, const int &conn, const int &ori);
    void AssignID(const int &i);
    std::vector<Plane*> GetPlaneList();
    Plane *GetPlane(const PlaneType &type);
    void ConnectAPV(const PlaneType &plane, PRadGEMAPV *apv);
    std::vector<PRadGEMAPV*> GetAPVList(const PlaneType &type);

    int id;
    std::string name;
    std::string type;
    std::string readout_board;
    Plane *planes[Plane_Max];
};

#endif
