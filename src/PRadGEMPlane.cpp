//============================================================================//
// GEM plane class                                                            //
// A GEM plane is a component of GEM detector                                 //
// It is connected to several APV units                                       //
//                                                                            //
// Chao Peng                                                                  //
// 10/07/2016                                                                 //
//============================================================================//

#include "PRadGEMPlane.h"
#include "PRadGEMAPV.h"
#include <iostream>

PRadGEMPlane::PRadGEMPlane()
: detector(nullptr), name("Undefined"), type(Plane_X),
  size(0.), connector(-1), orientation(0)
{
}

PRadGEMPlane::PRadGEMPlane(const std::string &n, const PlaneType &t, const double &s,
                           const int &c, const int &o)
: detector(nullptr), name(n), type(t), size(s), connector(c), orientation(o)
{
}

PRadGEMPlane::PRadGEMPlane(PRadGEMDetector *d, const std::string &n, const PlaneType &t,
                           const double &s, const int &c, const int &o)
: detector(d), name(n), type(t), size(s), connector(c), orientation(o)
{
}

PRadGEMPlane::~PRadGEMPlane()
{
    for(auto &apv : apv_list)
    {
        apv->SetDetectorPlane(nullptr);
    }
}

void PRadGEMPlane::ConnectAPV(PRadGEMAPV *apv)
{
    for(auto &plane_apv : apv_list)
    {
        if(plane_apv->plane_index == apv->plane_index) {
            std::cout << "PRad GEM Detector Warning: Failed to connect plane " << name
                      << " with APV at FEC " << apv->fec_id << ", Channel " << apv->adc_ch
                      << ". APV index on this plane is duplicated."
                      << std::endl;
            return;
        }
        if(apv->plane_index >= connector) {
            std::cout << "PRad GEM Detector Warning: Failed to connect plane " << name
                      << " with APV at FEC " << apv->fec_id << ", Channel " << apv->adc_ch
                      << ". Plane connectors are not enough, have " << connector
                      << ", this APV is at " << apv->plane_index
                      << std::endl;
            return;
        }
    }

    apv_list.push_back(apv);
    apv->SetDetectorPlane(this);
}

// unit is mm
double PRadGEMPlane::GetStripPosition(const int &plane_strip)
{
    double pitch = 0.4;
    if(type == Plane_X)
        return -0.5*(size + 31*pitch) + pitch*plane_strip;
    else
        return -0.5*(size - pitch) + pitch*plane_strip;
}

