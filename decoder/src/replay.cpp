//============================================================================//
// An application of replay raw data file and save the replayed data into DST //
// file. This is the 1st-level replay, it only discards the pedestal data     //
//                                                                            //
// Chao Peng                                                                  //
// 10/04/2016                                                                 //
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

int main(int argc, char * argv[])
{
    char *ptr;
    string output, input;

    // -i input_file -o output_file
    for(int i = 1; i < argc; ++i)
    {
        ptr = argv[i];
        if(*(ptr++) == '-') {
            switch(*(ptr++))
            {
            case 'o':
                output = argv[++i];
                break;
            case 'i':
                input = argv[++i];
                break;
            default:
                printf("Unkown option!\n");
                exit(1);
            }
        }
    }

    PRadDataHandler *handler = new PRadDataHandler();

    // read configuration files
    handler->ReadConfig("config.txt");

    PRadBenchMark timer;
//    handler->ReadFromDST("/work/hallb/prad/replay/prad_001292.dst");
//    handler->ReadFromDST("prad_1292.dst");
//    handler->ReadFromEvio("/work/prad/xbai/1323/prad_001323.evio.1");
//    handler->ReadFromSplitEvio("/work/prad/xbai/1323/prad_001323.evio", 10);
    handler->InitializeByData(input+".0");
    handler->Replay(input, 1500, output);
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

