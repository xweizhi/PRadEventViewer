#ifndef PRAD_SQUARE_CLUSTER_H
#define PRAD_SQUARE_CLUSTER_H

#include "PRadReconstructor.h"

class PRadSquareCluster : public PRadReconstructor
{
public:
    PRadSquareCluster(PRadDataHandler *h = nullptr);
    virtual ~PRadSquareCluster();

    void Configurate(const std::string &path);
    void Clear();
    void Reconstruct(EventData &event);
    PRadDAQUnit *LocateModule(const double &x, const double &y);

protected:
    unsigned short getMaxEChannel();
    //void GEMCoorToLab(float* x, float *y, int type);
    //void HyCalCoorToLab(float* x, float *y);
    bool useLogWeight(double x, double y);
    std::vector<unsigned short> findCluster(unsigned short cneterID, double &clusterEnergy);
    std::vector<unsigned short> &GetTimeForCluster(PRadDAQUnit *module);

    //for parameter from reconstruction data base
    int fMaxNCluster;
    double fMinClusterCenterE;
    double fMinClusterE;
    double fMoliereCrystal;
    double fMoliereLeadGlass;
    double fMoliereRatio;
    double fBaseR;
    std::vector<unsigned short> fClusterCenterID;

public:
    static double Distance(PRadDAQUnit *u1, PRadDAQUnit *u2);
    static double Distance(const double &x1, const double &y1, const double &x2, const double &y2);
    static double Distance(const double &x1, const double &y1, const double &x2, const double &y2, const double &z1, const double &z2);
    static double Distance(const std::vector<double> &p1, const std::vector<double> &p2);
};

#endif
