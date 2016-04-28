#ifndef HYCAL_CLUSTERS_H
#define HYCAL_CLUSTERS_H

#include <vector>

class HyCalClusters
{
public:
    typedef struct Hits_Info
    {
        double x;
        double y;
        double E;
        Hits_Info(const double &xi, const double &yi, const double &Ei)
        : x(xi), y(yi), E(Ei) {};
        Hits_Info() {};
    } HyCal_Hits, HyCal_Module;

public:
    HyCalClusters();
    virtual ~HyCalClusters() {};
    void AddModule(const double &x, const double &y, const double &E);
    std::vector<HyCal_Hits> ReconstructHits();

private:
    std::vector<HyCal_Module> modules;
};

#endif
