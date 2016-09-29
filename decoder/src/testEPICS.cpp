//============================================================================//
// Start Qt and root applications                                             //
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
    handler->SetHyCalClusterMethod("Square");

    PRadBenchMark timer;
//    handler->ReadFromDST("test.dst");
//    handler->WriteToDST("test.dst");

    // here shows an example how to read DST file while not saving all the events
    // in memory
    dst_parser->OpenInput("/work/hallb/prad/replay/prad_001287.dst");

    int count = 0;

    // uncomment next line, it will not update calibration factor from dst file
    //dst_parser->SetMode(NO_HYCAL_CAL_UPDATE);

    while(dst_parser->Read() && count < 30000)
    {
        if(dst_parser->EventType() == PRad_DST_Event) {
            ++count;
            // you can push this event into data handler
            // handler->GetEventData().push_back(dst_parser->GetEvent()
            // or you can just do something with this event and discard it
            auto event = dst_parser->GetEvent();
            cout << event.event_number << "  ";
            cout << event.gem_data.size() << "  ";
            cout << handler->GetEPICSValue("MBSY2C_energy", event) << endl;
            handler->HyCalReconstruct(event);
            int Nhits;
            HyCalHit *hit = handler->GetHyCalCluster(Nhits);

            for(int i = 0; i < Nhits; ++i)
            {
                cout << hit[i].E << "  " << hit[i].x << "  " << hit[i].y << endl;
            }
        } else if(dst_parser->EventType() == PRad_DST_Epics) {
            // save epics into handler, otherwise get epicsvalue won't work
            handler->GetEPICSData().push_back(dst_parser->GetEPICSEvent());
        }
    }

    dst_parser->CloseInput();

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
