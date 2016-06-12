#ifndef PRAD_RECONSTRUCTOR_H
#define PRAD_RECONSTRUCTOR_H
//ROOT
#include "TFile.h"
#include "TTree.h"
//c++
#include <cassert>
#include <iostream>
//PRad
#include "PRadDataHandler.h"
#include "ConfigParser.h"
#include "PRadDAQUnit.h"
#include "datastruct.h"
using namespace std;

struct HyCalHit
{
    double x;
    double y;
    double E;

    HyCalHit() : x(0), y(0), E(0)
    {};
    HyCalHit(const double &cx, const double &cy, const double &cE)
    : x(cx), y(cy), E(cE)
    {};
};

class PRadReconstructor
{
  public:
  PRadReconstructor();
  ~PRadReconstructor() {};
  
  void InitConfig(const string &path);
  void Clear();
  void SetModuleList(vector<PRadDAQUnit*> * theList) { fModuleList = theList; }

  double Distance2Points(double x1, double y1, double x2, double y2);
  vector<HyCalHit> * CoarseHyCalReconstruct();
  
  protected:

  unsigned short GetMaxEChannel();
  //void GEMCoorToLab(float* x, float *y, int type);
  //void HyCalCoorToLab(float* x, float *y);
  bool UseLogWeight(double x, double y);
  vector<unsigned short> FindCluster(unsigned short cneterID, double* clusterEnergy);

  vector<PRadDAQUnit*> * fModuleList;
  vector<unsigned short> fClusterCenterID;
  vector<HyCalHit> fHyCalHit;
  vector<float>* fGEM1XHit;
  vector<float>* fGEM2XHit;
  vector<float>* fGEM1YHit;
  vector<float>* fGEM2YHit;

  //for parameter from reconstruction data base
  int fMaxNCluster;
  double fMinClusterCenterE;
  double fMinClusterE;

  //some universal constants
  double fMoliereCrystal;
  double fMoliereLeadGlass;
  double fMoliereRatio;
  double fBaseR;
  double fCorrectFactor;
};

#endif
