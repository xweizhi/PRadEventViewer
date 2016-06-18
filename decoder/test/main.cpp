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
    handler->ReadEPICSChannels("config/epics_channels.txt");
    handler->ReadTDCList("config/tdc_group_list.txt");
    handler->ReadChannelList("config/module_list.txt");

    // build map for channels
    handler->BuildChannelMap();

    // read basic pedestal, this step is not necessary
    // you can get pedestal from the first data file
    handler->ReadPedestalFile("config/pedestal.dat");

    // read basic calibration constants, this step is required to get correct energy
    handler->ReadCalibrationFile("config/calibration.txt");

    handler->ReadGEMConfiguration("config/gem_map.cfg");
    handler->ReadGEMPedestalFile("config/gem_ped.dat");
    //handler->GetSRS()->SetUnivTimeSample(5);
    //handler->GetSRS()->SetUnivZeroSupThresLevel(5);

    PRadBenchMark timer;
    //handler->ReadFromEvio("../prad_001197.evio.0");
    //handler->ReadFromEvio("../prad_001197.evio.1");
    handler->ReadFromDST("gem.dst");

    size_t size = handler->GetEventCount();
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

