//============================================================================//
// An example showing how to use GEM clustering method to reconstruct gem data//
//                                                                            //
// Chao Peng                                                                  //
// 10/07/2016                                                                 //
//============================================================================//

#include "PRadDataHandler.h"
#include "PRadDSTParser.h"
#include "PRadEvioParser.h"
#include "PRadBenchMark.h"
#include "PRadDAQUnit.h"
#include "PRadGEMSystem.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(int /*argc*/, char * /*argv*/ [])
{

    PRadDataHandler *handler = new PRadDataHandler();

    // read configuration files
    handler->ReadConfig("config.txt");

    PRadBenchMark timer;

    // get GEM system
    PRadGEMSystem *gem_srs = handler->GetSRS();

    // show the plane list
    auto det_list = gem_srs->GetDetectorList();

    for(auto &detector: det_list)
    {
        cout << "Detector: " << detector->GetName() << endl;
        for(auto &plane : detector->GetPlaneList())
        {
            cout << "    " << "Plane: " << plane->GetName() << endl;
            for(auto &apv : plane->GetAPVList())
            {
                cout << "    " << "    "
                     << "APV: " << apv->GetPlaneIndex()
                     << ", " << apv->GetAddress();

                int min = apv->GetPlaneStripNb(0);
                int max = apv->GetPlaneStripNb(0);
                for(size_t i = 1; i < apv->GetTimeSampleSize(); ++i)
                {
                    int strip = apv->GetPlaneStripNb(i);
                    if(min > strip) min = strip;
                    if(max < strip) max = strip;
                }

                cout << ", " << min
                     << " ~ " << max
                     << endl;
            }
        }
    }

    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;
    cout << "Read " << handler->GetEventCount() << " events and "
         << handler->GetEPICSEventCount() << " EPICS events from file."
         << endl;
    cout << handler->GetBeamCharge() << endl;
    cout << handler->GetLiveTime() << endl;

    return 0;
}
