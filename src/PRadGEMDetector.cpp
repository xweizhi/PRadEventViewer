//============================================================================//
// GEM detector class                                                         //
// A detector has X and Y planes, each of the plane is connected to several   //
// APVs                                                                       //
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
    for(int i = 0; i < (int)Plane_Max; ++i)
        planes[i] = nullptr;
}

PRadGEMDetector::~PRadGEMDetector()
{
    for(int i = 0; i < (int)Plane_Max; ++i)
        if(planes[i] != nullptr)
            delete planes[i], planes[i] = nullptr;
}

void PRadGEMDetector::AddPlane(const PRadGEMDetector::PlaneType &type,
                               const std::string &name,
                               const float &size,
                               const int &conn,
                               const int &ori)
{
    planes[(int)type] = new Plane(this, name, size, conn, ori);
}

void PRadGEMDetector::AddPlane(const PRadGEMDetector::PlaneType &type,
                               PRadGEMDetector::Plane *plane)
{
    if(plane->detector != nullptr) {
        std::cerr << "PRad GEM Detector Error: "
                  << "Trying to add plane " << plane->name
                  << " to detector " << name
                  << ", but the plane is belong to " << plane->detector->name
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

    plane->detector = this;
    planes[(int)type] = plane;
}

void PRadGEMDetector::AssignID(const int &i)
{
    id = i;
}

std::vector<PRadGEMDetector::Plane*> PRadGEMDetector::GetPlaneList()
{
    std::vector<Plane*> result;
    for(int i = 0; i < (int)Plane_Max; ++i)
        result.push_back(planes[i]);
    return result;
}

PRadGEMDetector::Plane *PRadGEMDetector::GetPlane(const PRadGEMDetector::PlaneType &type)
{
    return planes[(int)type];
}

std::vector<PRadGEMAPV*> PRadGEMDetector::GetAPVList(const PRadGEMDetector::PlaneType &type)
{
    if(planes[(int)type] == nullptr)
        return std::vector<PRadGEMAPV*>();

    return planes[(int)type]->apv_list;
}

void PRadGEMDetector::ConnectAPV(const PRadGEMDetector::PlaneType &type, PRadGEMAPV *apv)
{
    if(planes[(int)type] == nullptr)
        return;

    planes[(int)type]->ConnectAPV(apv);
}

//============================================================================//
// Detector Plane Functions                                                   //
//============================================================================//
void PRadGEMDetector::Plane::ConnectAPV(PRadGEMAPV *apv)
{
    auto list_it = find(apv_list.begin(), apv_list.end(), apv);

    if(list_it == apv_list.end()) {
        apv_list.push_back(apv);
    } else {
        std::cout << "PRad GEM Detector Warning: Failed to connect plane " << name
                  << " with APV at FEC " << apv->fec_id << ", Channel " << apv->adc_ch
                  << ", APV is already connected."
                  << std::endl;
    }
}
