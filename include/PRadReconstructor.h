#ifndef PRAD_RECONSTRUCTOR_H
#define PRAD_RECONSTRUCTOR_H

#include <string>
#include <vector>
#include "datastruct.h"
#include "PRadEventStruct.h"

class PRadDataHandler;
class PRadDAQUnit;

class PRadReconstructor
{
public:
  PRadReconstructor();
  ~PRadReconstructor() {};
  
  void InitConfig(const std::string &path);
  void Clear();
  void SetModuleList(std::vector<PRadDAQUnit*> * theList) { fModuleList = theList; }

  std::vector<HyCalHit> * CoarseHyCalReconstruct();
  
protected:

  unsigned short GetMaxEChannel();
  //void GEMCoorToLab(float* x, float *y, int type);
  //void HyCalCoorToLab(float* x, float *y);
  bool UseLogWeight(double x, double y);
  std::vector<unsigned short> FindCluster(unsigned short cneterID, double* clusterEnergy);

  std::vector<PRadDAQUnit*> * fModuleList;
  std::vector<unsigned short> fClusterCenterID;
  std::vector<HyCalHit> fHyCalHit;
  std::vector<float>* fGEM1XHit;
  std::vector<float>* fGEM1YHit;
  std::vector<float>* fGEM2XHit;
  std::vector<float>* fGEM2YHit;

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

public:
  static double Distance(const double &x1, const double &y1, const double &x2, const double &y2);
  static double Distance(const double &x1, const double &y1, const double &x2, const double &y2, const double &z1, const double &z2);
  static double Distance(const std::vector<double> &p1, const std::vector<double> &p2);
};

#endif
