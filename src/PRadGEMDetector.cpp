//============================================================================//
// GEM detector class                                                         //
// A detector has several planes (X, Y)                                       //
//                                                                            //
// Chao Peng                                                                  //
// 10/07/2016                                                                 //
//============================================================================//

#include "PRadGEMDetector.h"
#include "PRadGEMAPV.h"
#include <algorithm>
#include <iostream>

PRadGEMDetector::PRadGEMDetector(const std::string &readoutBoard,
                                 const std::string &detectorType,
                                 const std::string &detector)
: name (detector), type(detectorType), readout_board(readoutBoard)
{
    planes.resize(PRadGEMPlane::Plane_Max, nullptr);
}

PRadGEMDetector::~PRadGEMDetector()
{
    for(auto &plane : planes)
    {
        if(plane != nullptr)
            delete plane, plane = nullptr;
    }
}

void PRadGEMDetector::AddPlane(const PRadGEMPlane::PlaneType &type,
                               const std::string &name,
                               const double &size,
                               const int &conn,
                               const int &ori)
{
    planes[(int)type] = new PRadGEMPlane(this, name, type, size, conn, ori);
}

void PRadGEMDetector::AddPlane(const PRadGEMPlane::PlaneType &type,
                               PRadGEMPlane *plane)
{
    if(plane->GetDetector() != nullptr) {
        std::cerr << "PRad GEM Detector Error: "
                  << "Trying to add plane " << plane->GetName()
                  << " to detector " << name
                  << ", but the plane is belong to " << plane->GetDetector()->name
                  << std::endl;
        return;
    }

    if(planes[(int)type] != nullptr) {
        std::cout << "PRad GEM Detector Warning: "
                  << "Trying to add multiple planes with the same type "
                  << "to detector " << name
                  << ", there will be potential memory leakage if the original "
                  << "plane is not released properly."
                  << std::endl;
    }

    plane->SetDetector(this);
    planes[(int)type] = plane;
}

void PRadGEMDetector::ReconstructHits()
{
    for(auto &plane : planes)
    {
        if(plane == nullptr)
            continue;

        plane->ReconstructHits();
    }
}

void PRadGEMDetector::ClearHits()
{
    for(auto &plane : planes)
    {
        if(plane != nullptr)
            plane->ClearPlaneHits();
    }
}

void PRadGEMDetector::AssignID(const int &i)
{
    id = i;
}

std::vector<PRadGEMPlane*> PRadGEMDetector::GetPlaneList()
{
    // since it allows nullptr in planes
    // for safety issue, only pack existing planes and return
    std::vector<PRadGEMPlane*> result;

    for(auto &plane : planes)
    {
        if(plane != nullptr)
            result.push_back(plane);
    }

    return result;
}

PRadGEMPlane *PRadGEMDetector::GetPlane(const PRadGEMPlane::PlaneType &type)
{
    return planes[(int)type];
}

std::vector<PRadGEMAPV*> PRadGEMDetector::GetAPVList(const PRadGEMPlane::PlaneType &type)
{
    if(planes[(int)type] == nullptr)
        return std::vector<PRadGEMAPV*>();

    return planes[(int)type]->GetAPVList();
}

void PRadGEMDetector::ConnectAPV(const PRadGEMPlane::PlaneType &type, PRadGEMAPV *apv)
{
    if(planes[(int)type] == nullptr)
        return;

    planes[(int)type]->ConnectAPV(apv);
}
