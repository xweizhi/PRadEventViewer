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
    handler->ReadTDCList("config/tdc_group_list.txt");
    handler->ReadChannelList("config/module_list.txt");

    // build map for channels
    handler->BuildChannelMap();

    // read basic pedestal, this step is not necessary
    // you can get pedestal from the first data file
    handler->ReadPedestalFile("config/pedestal.dat");

    // read basic calibration constants, this step is required to get correct energy
    handler->ReadCalibrationFile("config/calibration.txt");

     // read dst files and refill histograms
    handler->ReadFromDST("prad_001197_test1.dst");
    cout << "Read " << handler->GetEventCount() << " events." << endl;
    handler->RefillChannelHists();
    handler->SaveHistograms("dst_1.root");
    handler->Clear();
    handler->ReadFromDST("prad_001197_test2.dst");
    cout << "Read " << handler->GetEventCount() << " events." << endl;
    handler->RefillChannelHists();
    handler->SaveHistograms("dst_2.root");
    handler->Clear();

    // input list
    vector<string> fileList;
    string first_file = "../prad_001197.evio.0";
    fileList.push_back("../prad_001197.evio.0");
    fileList.push_back("../prad_001197.evio.1");
    fileList.push_back("../prad_001197.evio.2");

    // timer
    PRadBenchMark timer;

    // first method, use evio library
    //
    // read first data file and then correct pedestal and gain
    try {
        evio::evioFileChannel *chan = new evio::evioFileChannel(first_file.c_str(), "r");
        chan->open();
        while(chan->read())
        {
             handler->Decode(chan->getBuffer());
        }
        handler->FitPedestal();
        // run number is important since we had some reference shift during some runs
        handler->CorrectGainFactor(1197);
        handler->Clear();
    } catch (evio::evioException e) {
        std::cerr << e.toString() << endl;
    } catch (...) {
        std::cerr << "?unknown exception" << endl;
    }

   // reading data file with the corrected pedestal and gain
   try {
        for(const string &file : fileList)
        {
            evio::evioFileChannel *chan = new evio::evioFileChannel(file.c_str(),"r");
            chan->open();
            while(chan->read())
            {
                 handler->Decode(chan->getBuffer());
            }
        }
    } catch (evio::evioException e) {
        std::cerr << e.toString() << endl;
    } catch (...) {
        std::cerr << "?unknown exception" << endl;
    }
    // finished reading
    cout << "TIMER: First method, took " << timer.GetElapsedTime() << " ms" << endl;

    // save data info
    handler->WriteToDST("prad_001197_test1.dst");
    handler->SaveHistograms("prad_001197_test1.root");
    // clear saved data
    handler->Clear();

    // reset timer
    timer.Reset();

    // second method, use self-written function to read evio file

    // correct gain and pedestal from the first data file
    // run number can be auto determined by file name
    handler->InitializeByData(first_file);

    // reading data files
    for(const string &file : fileList)
    {
        handler->ReadFromEvio(file);
    }
    cout << "TIMER: Second method, took " << timer.GetElapsedTime() << " ms" << endl;

    // save data info
    handler->WriteToDST("prad_001197_test2.dst");
    handler->SaveHistograms("prad_001197_test2.root");
    // clear saved data
    handler->Clear();

    return 0;
}

