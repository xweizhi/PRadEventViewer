//============================================================================//
// An example showing how to read and analyze simulation output from PRadSim  //
//                                                                            //
// Chao Peng                                                                  //
// 10/12/2016                                                                 //
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
    // reading pedestal file can be set in config.txt
    // but other example either reading ped from dst file
    // or fit ped from data, so the reading config is commented out
    handler->ReadPedestalFile("config/pedestal.dat");

    PRadBenchMark timer;

    //    handler->ReadFromEvio("/home/chao/Desktop/prad_001287.evio.0");
    // read simulation output
    handler->ReadFromEvio("/home/chao/geant4/PRadSim/output/simrun_46.evio");

    for(size_t i = 0; i < handler->GetEventCount(); ++i)
    {
        handler->HyCalReconstruct(i);
        int Nhits;
        HyCalHit *hit = handler->GetHyCalCluster(Nhits);

        cout << "event " << i << endl;
        for(int i = 0; i < Nhits; ++i)
        {
            cout << hit[i].E << "  " << hit[i].x << "  " << hit[i].y << endl;
        }
    }

    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;
    cout << "Read " << handler->GetEventCount() << " events and "
         << handler->GetEPICSEventCount() << " EPICS events from file."
         << endl;
    cout << handler->GetBeamCharge() << endl;
    cout << handler->GetLiveTime() << endl;

//    handler->WriteToDST("prad_001323_0-10.dst");
    //handler->PrintOutEPICS();
    return 0;
}
