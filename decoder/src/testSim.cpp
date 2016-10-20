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
#include "TFile.h"
#include "TTree.h"
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
    handler->ReadFromEvio("/home/chao/geant4/PRadSim/output/simrun_47.evio");

    TFile *f = new TFile("testSim.root", "RECREATE");
    TTree *t = new TTree("T", "T");

    int N;
    double E[100], x[100], y[100]; // maximum number of clusters, 100 is enough
    // retrieve part of the cluster information
    t->Branch("NClusters", &N, "NClusters/I");
    t->Branch("ClusterE", &E, "ClusterE/D");
    t->Branch("ClusterX", &x[0], "ClusterX[NClusters]/D");
    t->Branch("ClusterY", &y[0], "ClusterY[NClusters]/D");


    for(size_t i = 0; i < handler->GetEventCount(); ++i)
    {
        handler->HyCalReconstruct(i);
        HyCalHit *hit = handler->GetHyCalCluster(N);

        for(int i = 0; i < N; ++i)
        {
            E[i] = hit[i].E;
            x[i] = hit[i].x_log;
            y[i] = hit[i].y_log;
        }
        t->Fill();
    }

    cout << "TIMER: Finished, took " << timer.GetElapsedTime() << " ms" << endl;
    cout << "Read " << handler->GetEventCount() << " events and "
         << handler->GetEPICSEventCount() << " EPICS events from file."
         << endl;
    cout << handler->GetBeamCharge() << endl;
    cout << handler->GetLiveTime() << endl;

    f->cd();
    t->Write();
    f->Close();

//    handler->WriteToDST("prad_001323_0-10.dst");
    //handler->PrintOutEPICS();
    return 0;
}
