//============================================================================//
// Start Qt and root applications                                             //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include "PRadDataHandler.h"
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
    handler->ReadFromDST("prad_1498.dst");

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

