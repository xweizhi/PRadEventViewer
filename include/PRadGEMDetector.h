#ifndef PRAD_GEM_DETECTOR_H
#define PRAD_GEM_DETECTOR_H

#include <vector>
#include <string>
#include "PRadGEMPlane.h"

class PRadGEMAPV;

class PRadGEMDetector
{
public:
    PRadGEMDetector(const std::string &readoutBoard,
                    const std::string &detectorType,
                    const std::string &detector);
    virtual ~PRadGEMDetector();

    void AddPlane(const PRadGEMPlane::PlaneType &type, PRadGEMPlane *plane);
    void AddPlane(const PRadGEMPlane::PlaneType &type, const std::string &name,
                  const double &size, const int &conn, const int &ori);
    void AssignID(const int &i);
    std::vector<PRadGEMPlane*> GetPlaneList();
    PRadGEMPlane *GetPlane(const PRadGEMPlane::PlaneType &type);
    void ConnectAPV(const PRadGEMPlane::PlaneType &plane, PRadGEMAPV *apv);
    std::vector<PRadGEMAPV*> GetAPVList(const PRadGEMPlane::PlaneType &type);

    int id;
    std::string name;
    std::string type;
    std::string readout_board;
    std::vector<PRadGEMPlane*> planes;
};

#endif
