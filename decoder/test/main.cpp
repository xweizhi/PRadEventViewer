//============================================================================//
// Start Qt and root applications                                             //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include "PRadDecoder.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(int /*argc*/, char * /*argv*/ [])
{

    PRadDataHandler *handler = new PRadDataHandler();

    // read configuration files
    handler->ReadConfig("config.txt");
    //handler->GetSRS()->SetUnivTimeSample(5);
    //handler->GetSRS()->SetUnivZeroSupThresLevel(5);

    PRadBenchMark timer;
    //handler->ReadFromEvio("../prad_001197.evio.0");
    //handler->ReadFromEvio("../prad_001197.evio.1");
    handler->ReadFromDST("gem.dst");

    size_t size = handler->GetEventCount();
    cout << size << endl;
    for(size_t i = 0; i < size; ++i)
    {
        cout << handler->GetEventData(i).gem_data.size() << endl;
    }
    //handler->ReadFromDST("gem.dst");
    cout << "TIMER: Second method, took " << timer.GetElapsedTime() << " ms" << endl;
    cout << "Read " << handler->GetEventCount() << " events and "
         << handler->GetEPICSEventCount() << " EPICS events from file."
         << endl;
//    handler->WriteToDST("gem.dst");
    //handler->PrintOutEPICS();
    return 0;
}

