#include "HyCalClusters.h"

using namespace std;

HyCalClusters::HyCalClusters()
{
}

void HyCalClusters::AddModule(const double &x, const double &y, const double &E)
{
    modules.push_back(HyCal_Module(x, y, E));
}

vector<HyCalClusters::HyCal_Hits> HyCalClusters::ReconstructHits()
{
    return {};
}
