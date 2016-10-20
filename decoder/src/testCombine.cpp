//============================================================================//
// example showing how to do GEM and HyCal Reconstruction and then            //
// save the info into root file, coordinate are in detector's internal frame  //
// Weizhi Xiong                                                               //
// 10/11/2016                                                                 //
//============================================================================//

#include "PRadDataHandler.h"
#include "PRadDSTParser.h"
#include "PRadEvioParser.h"
#include "PRadBenchMark.h"
#include "PRadDAQUnit.h"
#include "PRadGEMSystem.h"
#include "PRadEventStruct.h"

#include "TFile.h"
#include "TTree.h"

#include <iostream>
#include <vector>
#include <map>

#define MAXGEMHIT 2000
#define NPLANE 4

using namespace std;

int main()
{
  int run = 1345;
  PRadDataHandler *handler = new PRadDataHandler();
  PRadDSTParser *dst_parser = new PRadDSTParser(handler);
  handler->ReadConfig("config.txt");
  handler->SetHyCalClusterMethod("Island"); //using PrimEx island reconstruction algorithm

  PRadGEMSystem *gem_srs = handler->GetSRS(); // Get the GEM system here for easy usage later

  dst_parser->OpenInput(Form("../../dst_data/prad_00%d.dst", run));

  TFile* f = new TFile(Form("test_run_00%d_weight_3.6_non_lin_test.root", run),"RECREATE");
  TTree* t = new TTree("T", "T");

  //globle variables for writing root output
  Int_t eventNumber;
  Double_t Ebeam;
  //for HyCal
  Int_t clusterN;
  Double_t totalE;
  Double_t clusterE[MAX_CC], clusterX[MAX_CC], clusterY[MAX_CC], clusterZ[MAX_CC], clusterChi2[MAX_CC], clusterSigmaE[MAX_CC];
  UInt_t clusterFlag[MAX_CC];
  Int_t clusterNHit[MAX_CC];
  Int_t clusterStatus[MAX_CC];
  Int_t clusterType[MAX_CC];
  Int_t clusterCID[MAX_CC];

  //for GEM 1D hit
  Int_t nGEM1DHit[NPLANE];
  Double_t hit1DPos[NPLANE][MAXGEMHIT], hit1DTCharge[NPLANE][MAXGEMHIT], hit1DSize[NPLANE][MAXGEMHIT];
  Double_t hit1DPCharge[NPLANE][MAXGEMHIT];

  t->Branch("EventNum", &eventNumber, "EventNum/I");
  t->Branch("nHyCalHit", &clusterN, "nHyCalHit/I");
  t->Branch("EBeam", &Ebeam, "EBeam/D");
  t->Branch("TotalE", &totalE, "TotalE/D");
  t->Branch("HyCalHit.Flag", &clusterFlag[0], "HyCalHit.Flag[nHyCalHit]/i");
  t->Branch("HyCalHit.X", &clusterX[0], "HyCalHit.X[nHyCalHit]/D");
  t->Branch("HyCalHit.Y", &clusterY[0], "HyCalHit.Y[nHyCalHit]/D");
  t->Branch("HyCalHit.Z", &clusterZ[0], "HyCalHit.Z[nHyCalHit]/D");
  t->Branch("HyCalHit.E", &clusterE[0], "HyCalHit.E[nHyCalHit]/D");
  t->Branch("HyCalHit.Chi2", &clusterChi2[0], "HyCalHit.Chi2[nHyCalHit]/D");
  t->Branch("HyCalHit.NModule", &clusterNHit[0], "HyCalHit.NModule[nHyCalHit]/I");
  t->Branch("HyCalHit.Status", &clusterStatus[0], "HyCalHit.Status[nHyCalHit]/I");
  t->Branch("HyCalHit.Type", &clusterType[0], "HyCalHit.Type[nHyCalHit]/I");
  t->Branch("HyCalHit.CID", &clusterCID[0], "HyCalHit.CID[nHyCalHit]/I");
  t->Branch("HyCalHit.SigmaE", &clusterSigmaE[0], "HyCalHit.SigmaE[nHyCalHit]/D");

  for (int i=0; i<NPLANE; i++){
    t->Branch(Form("nGEMHit.%d", i+1), &nGEM1DHit[i], Form("nGEMHit.%d/I", i+1));
    t->Branch(Form("GEMHit.%d.Pos", i+1), &hit1DPos[i][0], Form("GEMHit.%d.Pos[nGEMHit.%d]/D", i+1, i+1));
    t->Branch(Form("GEMHit.%d.TCharge", i+1), &hit1DTCharge[i][0], Form("GEMHit.%d.TCharge[nGEMHit.%d]/D", i+1, i+1));
    t->Branch(Form("GEMHit.%d.PCharge",i+1), &hit1DPCharge[i][0], Form("GEMHit.%d.PCharge[nGEMHit.%d]/D", i+1, i+1));
    t->Branch(Form("GEMHit.%d.Size",i+1), &hit1DSize[i][0], Form("GEMHit.%d.Size[nGEMHit.%d]/D", i+1, i+1));
  }


  int count = 0;

  while (dst_parser->Read()){
    if (dst_parser->EventType() == PRad_DST_Event) {
      auto event = dst_parser->GetEvent();

      if (! (event.trigger == PHYS_LeadGlassSum ||
             event.trigger == PHYS_TotalSum)) continue;

      if (count%10000 == 0)
          cout << "----------event " << count << "-------------" << endl;

      if (count > 200000) break;

      count++;

      //HyCal cluster reconstruction
      handler->HyCalReconstruct(event);

      //GEM cluster reconstruction
      gem_srs->Reconstruct(event);

      //Getting the beam energy from epics
      Ebeam = handler->GetEPICSValue("MBSY2C_energy", eventNumber);
      eventNumber = handler->GetCurrentEventNb();

      //Getting HyCal Clusters
      HyCalHit* thisHit = handler->GetHyCalCluster(clusterN);
      totalE = 0.;
      for (int i = 0; i < clusterN ; i++){
        clusterFlag[i] = thisHit[i].flag;
        clusterX[i] = thisHit[i].x_log;
        clusterY[i] = thisHit[i].y_log;
        clusterZ[i] = thisHit[i].dz;
        clusterE[i] = thisHit[i].E;
        totalE += clusterE[i];
        clusterNHit[i] = thisHit[i].nblocks;
        clusterChi2[i] = thisHit[i].chi2;
        clusterStatus[i] = thisHit[i].status;
        clusterType[i] = thisHit[i].type;
        clusterCID[i] = thisHit[i].cid;
        clusterSigmaE[i] = thisHit[i].sigma_E;
      }

      //Getting GEM 1D cluster
      vector< list< GEMPlaneCluster >* > vlist;
      vlist.push_back(&(gem_srs->GetDetectorPlane("pRadGEM1X")->GetPlaneCluster()));
      vlist.push_back(&(gem_srs->GetDetectorPlane("pRadGEM1Y")->GetPlaneCluster()));
      vlist.push_back(&(gem_srs->GetDetectorPlane("pRadGEM2X")->GetPlaneCluster()));
      vlist.push_back(&(gem_srs->GetDetectorPlane("pRadGEM2Y")->GetPlaneCluster()));

      for (unsigned int i=0; i<vlist.size(); i++){
        nGEM1DHit[i] = vlist.at(i)->size();

        int index = 0;
        for (list<GEMPlaneCluster>::iterator it = vlist.at(i)->begin(); it != vlist.at(i)->end(); it++){
          hit1DPos[i][index] = (*it).position;
          hit1DTCharge[i][index] = (*it).total_charge;
          hit1DPCharge[i][index] = (*it).peak_charge;
          hit1DSize[i][index] = (*it).hits.size();
          index++;
        }
      }
      t->Fill();

    }else if (dst_parser->EventType() == PRad_DST_Epics) {
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


















