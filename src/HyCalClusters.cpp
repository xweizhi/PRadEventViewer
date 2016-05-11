#include <cmath>
#include "HyCalClusters.h"

using namespace std;

HyCalClusters::HyCalClusters(const double &thres)
: threshold(thres)
{
    baseR = 20.5 * 3.;
}

void HyCalClusters::AddModule(const double &x, const double &y, const double &E)
{
    modules.push_back(HyCal_Module(x, y, E));
}

vector<HyCalClusters::HyCal_Hits> HyCalClusters::ReconstructHits()
{
    vector<HyCal_Module> pos_centers, centers;
    for(auto &module : modules)
    {
        if(module.E > threshold/2.) {
            pos_centers.push_back(module);
        }
    }

    for(size_t i = 0; i < pos_centers.size(); ++i)
    {
        bool rule_out = false;
        for(size_t j = i + 1; j < pos_centers.size(); ++j)
        {
            if( distance(pos_centers[i], pos_centers[j]) < baseR ) {
                rule_out = true;
                if(pos_centers[i].E > pos_centers[j].E)
                    pos_centers[j] = pos_centers[i];
            }
        }
        if(!rule_out)
            centers.push_back(pos_centers[i]);

    }
    return centers;
}

double HyCalClusters::distance(const HyCal_Module &m1, const HyCal_Module &m2)
{
    return sqrt((m1.x - m2.x)*(m1.x - m2.x) + (m1.y - m2.y)*(m1.y - m2.y));
}
