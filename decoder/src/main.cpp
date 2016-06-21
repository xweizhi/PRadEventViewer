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
#include "evioUtil.hxx"
#include "evioFileChannel.hxx"
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
//    handler->ReadFromEvio("/work/prad/xbai/1323/prad_001323.evio.1");
//    handler->ReadFromSplitEvio("/work/prad/xbai/1323/prad_001323.evio", 10);
//    handler->Replay("/data/totape/prad_001498.evio", 100);
//    handler->GetSRS()->SavePedestal("gem_ped.txt");


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

