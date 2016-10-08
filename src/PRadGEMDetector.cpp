#include "PRadGEMDetector.h"
#include "PRadGEMAPV.h"
#include <algorithm>
#include <iostream>

PRadGEMDetector::PRadGEMDetector(const std::string &readoutBoard,
                                 const std::string &detectorType,
                                 const std::string &detector)
: name (detector), type(detectorType), readout_board(readoutBoard)
{

}

PRadGEMDetector::~PRadGEMDetector()
{

}

void PRadGEMDetector::AddPlane(const PRadGEMDetector::PlaneType &type, const Plane &plane)
{
    planes[(int)type] = plane;
}

void PRadGEMDetector::AddPlane(const PRadGEMDetector::PlaneType &type, Plane &&plane)
{
    planes[(int)type] = plane;
}

void PRadGEMDetector::AssignID(const int &i)
{
    id = i;
}

PRadGEMDetector::Plane &PRadGEMDetector::GetPlane(const PRadGEMDetector::PlaneType &type)
{
    return planes[(int)type];
}

PRadGEMDetector::Plane &PRadGEMDetector::GetPlaneX()
{
    return planes[(int)Plane_X];
}

PRadGEMDetector::Plane &PRadGEMDetector::GetPlaneY()
{
    return planes[(int)Plane_Y];
}

std::vector<PRadGEMAPV*> &PRadGEMDetector::GetAPVList(const PRadGEMDetector::PlaneType &type)
{
    return planes[(int)type].apv_list;
}

void PRadGEMDetector::ConnectAPV(const PRadGEMDetector::PlaneType &type, PRadGEMAPV *apv)
{
    auto &mylist = GetAPVList(type);
    auto list_it = find(mylist.begin(), mylist.end(), apv);
    if(list_it != mylist.end())
        mylist.push_back(apv);
    else {
        std::cout << "PRad GEM Detector Warning: Failed to connect "
                  << "Detector " << name << ", plane " << planes[(int)type].name
                  << " with APV at FEC " << apv->fec_id << ", Channel " << apv->adc_ch
                  << ", APV is already connected."
                  << std::endl;
    }
}
