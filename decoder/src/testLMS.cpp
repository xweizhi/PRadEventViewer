//============================================================================//
// An example showing how to get the LMS data from the replayed DST file      //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
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
    PRadDSTParser *dst_parser = new PRadDSTParser(handler);

    // read configuration files
    handler->ReadConfig("config.txt");

    PRadBenchMark timer;

    // here shows an example how to read DST file while not saving all the events
    // in memory
    dst_parser->OpenInput("/work/hallb/prad/replay/prad_001287.dst");

    int count = 0;

    // uncomment next line, it will not update calibration factor from dst file
    //dst_parser->SetMode(NO_HYCAL_CAL_UPDATE);

    // typically, first 10000 are pedestal data
    // and next 10000 are LMS data
    // so 30000 is more than enough for pedestal and LMS data
    while(dst_parser->Read() && count < 30000)
    {
        if(dst_parser->EventType() == PRad_DST_Event) {
            ++count;
            // you can push this event into data handler
            // stored event enables you to check it one by one
            handler->GetEventData().push_back(dst_parser->GetEvent());
            // fill the event into histograms
            // if you only want to deal with histograms, you can comment
            // the above lines to save memory
            handler->FillHistograms(dst_parser->GetEvent());
        } else if(dst_parser->EventType() == PRad_DST_Epics) {
            // save epics into handler, otherwise get epicsvalue won't work
            handler->GetEPICSData().push_back(dst_parser->GetEPICSEvent());
        }
    }

    dst_parser->CloseInput();

    // channel name, histogram name, fit function, min range, max range, verbose
    auto pars1 = handler->FitHistogram("W1115", "PED", "gaus", 0, 8200, true);
    auto pars2 = handler->FitHistogram("W1115", "LMS", "gaus", 0, 8200, true);

    // for Gaussian, par0 is amplitude, par1 is mean, par2 is sigma
    cout << "Pedestal for W1115: mean " << pars1.at(1)
         << ", sigma " << pars1.at(2) << endl;
    cout << "LMS for W1115: mean " << pars1.at(1)
         << ", sigma " << pars1.at(2) << endl;

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
