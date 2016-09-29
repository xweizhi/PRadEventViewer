#include "PRadDataHandler.h"
#include "PRadDSTParser.h"
#include "PRadEvioParser.h"
#include "PRadBenchMark.h"
#include "PRadDAQUnit.h"
#include "PRadGEMSystem.h"
#include "PRadIslandCluster.h"
#include "PRadSquareCluster.h"
#include "TFile.h"
#include "TTree.h"
#include <iostream>
#include <string>
#include <vector>

#define MAXHIT 50

using namespace std;

int main(int /*argc*/, char * /*argv*/ [])
{
  PRadDataHandler *handler = new PRadDataHandler();
  PRadDSTParser *dst_parser = new PRadDSTParser(handler);

  handler->ReadConfig("config.txt");

  //handler->UpdateHyCalGain("../config/gainfunc_round7.dat");

  dst_parser->OpenInput("/work/hallb/prad/replay/prad_001291.dst");

  //----------init root file-------------//
  TFile *f = new TFile("test5.root", "RECREATE");
  TTree *t = new TTree("T", "T");

  Int_t clusterN, eventNumber;
  Double_t Ebeam;
  Double_t totalE;
  Double_t clusterE[MAXHIT], clusterX[MAXHIT], clusterY[MAXHIT], clusterChi2[MAXHIT];
  Int_t clusterNHit[MAXHIT];
  Int_t clusterStatus[MAXHIT];
  Int_t clusterType[MAXHIT];

  t->Branch("EventNum", &eventNumber, "EventNum/I");
  t->Branch("nHyCalHit", &clusterN, "nHyCalHit/I");
  t->Branch("EBeam", &Ebeam, "EBeam/D");
  t->Branch("TotalE", &totalE, "TotalE/D");
  t->Branch("HyCalHitX", &clusterX[0], "HyCalHitX[nHyCalHit]/D");
  t->Branch("HyCalHitY", &clusterY[0], "HyCalHitY[nHyCalHit]/D");
  t->Branch("HyCalHitE", &clusterE[0], "HyCalHitE[nHyCalHit]/D");
  t->Branch("HyCalHitChi2", &clusterChi2[0], "HyCalHitChi2[nHyCalHit]/D");
  t->Branch("HyCalHitNModule", &clusterNHit[0], "HyCalHitNModule[nHyCalHit]/I");
  t->Branch("HyCalHitStatus", &clusterStatus[0], "HyCalHitStatus[nHyCalHit]/I");
  t->Branch("HyCalHitType", &clusterType[0], "HyCalHitType[nHyCalHit]/I");


  //-------------------------------------//

  int count = 0;

  while(dst_parser->Read()){
  if(dst_parser->EventType() == PRad_DST_Event) {
    auto event = dst_parser->GetEvent();
    if (! (event.trigger == PHYS_LeadGlassSum || event.trigger == PHYS_TotalSum)) continue;
    if (count%10000 == 0) cout<<"----------event "<<count<<"-------------"<<endl;
    if (count > 100000) break;
    count++;

    handler->HyCalReconstruct(event);
    HyCalHit *clusterArray = handler->GetHyCalCluster(clusterN);

    Ebeam = handler->GetEPICSValue("MBSY2C_energy", eventNumber);
    totalE = 0.;
    eventNumber = handler->GetCurrentEventNb();

    for (int i=0; i<clusterN ; i++){
      HyCalHit *thisCluster = &clusterArray[i];
      clusterX[i] = thisCluster->x_log;
      clusterY[i] = thisCluster->y_log;
      clusterE[i] = thisCluster->E;
      totalE += clusterE[i];
      clusterNHit[i] = thisCluster->nblocks;
      clusterChi2[i] = thisCluster->chi2;
      clusterStatus[i] = thisCluster->status;
      clusterType[i] = thisCluster->type;
    }

    t->Fill();
    if (count == -1) break;

  } else if(dst_parser->EventType() == PRad_DST_Epics){
      // save epics into handler, otherwise get epicsvalue won't work
      handler->GetEPICSData().push_back(dst_parser->GetEPICSEvent());
  }

  }

  dst_parser->CloseInput();

  f->cd();
  t->Write();
  f->Close();

  return 0;
}
