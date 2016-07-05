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

    PRadBenchMark timer;
//    handler->ReadFromDST("test.dst");
//    handler->WriteToDST("test.dst");

    // here shows an example how to read DST file while not saving all the events
    // in memory
    dst_parser->OpenInput("test.dst");
    int count = 0;
    while(dst_parser->Read() && count < 100000)
    {
        switch(dst_parser->EventType())
        {
        case PRad_DST_Event: {
            ++count;
            auto event = dst_parser->GetEvent();
            cout << event.event_number << "  ";
            cout << event.gem_data.size() << "  ";
            cout << handler->GetEPICSValue("MBSY2C_energy", event) << endl;
          } break;
        case PRad_DST_Epics:
            handler->GetEPICSData().push_back(dst_parser->GetEPICSEvent());
            break;
        default:
            break;
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

