//============================================================================//
// A C++ wrapper for island reconstruction method                             //
//                                                                            //
// Weizhi Xiong, Chao Peng                                                    //
// 09/28/2016                                                                 //
//============================================================================//
#include <cmath>
#include <cstdio>
#include <cstring>
#include "PRadIslandWrapper.h"
#include "PRadDataHandler.h"
#include "PRadDAQUnit.h"

PRadIslandWrapper::PRadIslandWrapper(PRadDataHandler *h, const std::string &config_path)
: fHandler(h), fConfigPath(config_path), fMinHitE(5.e-3)
{
    InitTable();
}
//________________________________________________________________
void PRadIslandWrapper::InitTable()
{
    InitConstants();

    for(int k = 0; k <= 4; ++k)
        for(int i = 1; i <= MCOL; ++i)
            for(int j = 1; j <= MROW; ++j)
                fModuleStatus[k][i-1][j-1] = 0;
    //  mark PWO hole:
    fModuleStatus[0][17-1][17-1] = -1;
    fModuleStatus[0][17-1][18-1] = -1;
    fModuleStatus[0][18-1][17-1] = -1;
    fModuleStatus[0][18-1][18-1] = -1;

    //  mark dead channles:
    for(int i = 0; i < T_BLOCKS; ++i)
    {
        int isector = fBlockINFO[i].sector;
        if(isector >=0 && isector <= 4)
        {
            int thisid = fBlockINFO[i].id;
            if(thisid == i+1)
            {
                int thisrow = fBlockINFO[i].row;
                int thiscol = fBlockINFO[i].col;
                fModuleStatus[isector][thiscol-1][thisrow-1] = 0; // HYCAL_STATUS[i];
                if(thisid == 1835)
                    fModuleStatus[isector][thiscol-1][thisrow-1] = 1; // HYCAL_STATUS[i];
            }
        }
    }
}
//________________________________________________________________
void PRadIslandWrapper::InitConstants()
{
    //load module info to the fBlockINFO array
    FILE *fp;
    int ival;
    union {int i; float f;} ieu;

    std::string block_info = fConfigPath + "blockinfo.dat";
    fp = fopen(block_info.c_str(), "r");
    for(int i = 0; i < T_BLOCKS; ++i)
    {
        if(fread(&ival, sizeof(ival), 1, fp))
            fBlockINFO[i].id = ival;

        if(fread(&ival, sizeof(ival), 1, fp))
        {
            ieu.i = ival;
            fBlockINFO[i].x = ieu.f;
        }

        if(fread(&ival, sizeof(ival), 1, fp))
        {
            ieu.i = ival;
            fBlockINFO[i].y = ieu.f;
        }

        if(fread(&ival, sizeof(ival), 1, fp))
            fBlockINFO[i].sector = ival;

        if(fread(&ival, sizeof(ival), 1, fp))
            fBlockINFO[i].row = ival;

        if(fread(&ival, sizeof(ival), 1, fp))
            fBlockINFO[i].col = ival;
    }
    fclose(fp);
}
//______________________________________________________________
void PRadIslandWrapper::Clear()
{
    fNHyCalClusters = 0;
    fNHyCalHits = 0;
}
//_______________________________________________________________
hycalcluster_t *PRadIslandWrapper::GetHyCalCluster(EventData& thisEvent)
{
    //clear and get ready for the new event
    Clear();

    //main function of the hycal reconstruction

    //first load data to the hit array and ech array
    //the second array is used in the fortran island code

    //if not physics event, don't reconstruct it
    if(!thisEvent.is_physics_event())
        return fHyCalCluster;

    LoadModuleData(thisEvent);

    //call island reconstruction of each sectors
    //HyCal has 5 sectors, 4 for lead glass one for crystal

    for(int i = 0; i < MSECT; ++i)
    {
        CallIsland(i);
    }

    //glue clusters at the transition region
    GlueTransitionClusters();

    ClusterProcessing();

    return fHyCalCluster;
}
//_______________________________________________________________
void PRadIslandWrapper::LoadModuleData(EventData& thisEvent)
{
    //load data to hycalhit array and ech array
    fHandler->ChooseEvent(thisEvent);
    std::vector<PRadDAQUnit*> moduleList = fHandler->GetChannelList();

    for(unsigned int i = 0; i < moduleList.size(); ++i)
    {
        PRadDAQUnit* thisModule = moduleList.at(i);
        if(!thisModule->IsHyCalModule())
            continue;
        if(thisModule->GetEnergy()*1e-3 < fMinHitE)
            continue;

        fHyCalHit[fNHyCalHits].e = thisModule->GetEnergy()*1e-3; //to GeV
        fHyCalHit[fNHyCalHits].id = thisModule->GetPrimexID();
        fNHyCalHits++;
    }
}
//________________________________________________________________
void PRadIslandWrapper::CallIsland(int isect)
{
    //float xsize, ysize;
    SET_EMIN  = 0.05;   // banks->CONFIG->config->CLUSTER_ENERGY_MIN;
    SET_EMAX  = 9.9;    // banks->CONFIG->config->CLUSTER_ENERGY_MAX;
    SET_HMIN  = 1;      // banks->CONFIG->config->CLUSTER_MIN_HITS_NUMBER;
    SET_MINM  = 0.01;   // banks->CONFIG->config->CLUSTER_MAX_CELL_MIN_ENERGY;
    ZHYCAL    = 580.;   // ALignment[1].al.z;
    ISECT     = isect;

    for(int icol = 1; icol <= MCOL; ++icol)
    {
        for(int irow = 1; irow <= MROW; ++irow)
        {
            ECH(icol,irow) = 0;
            STAT_CH(icol,irow) = fModuleStatus[isect][icol-1][irow-1];
        }
    }

    int coloffset = 0, rowoffset = 0;
    switch(isect)
    {
    case 0:
        NCOL = 34; NROW = 34;
        SET_XSIZE = CRYS_SIZE_X; SET_YSIZE = CRYS_SIZE_Y;
        break;
    case 1:
        NCOL = 24; NROW =  6;
        SET_XSIZE = GLASS_SIZE; SET_YSIZE = GLASS_SIZE;
        coloffset = 0, rowoffset = 0;
        break;
    case 2:
        NCOL =  6; NROW =  24;
        SET_XSIZE = GLASS_SIZE; SET_YSIZE = GLASS_SIZE;
        coloffset = 24, rowoffset = 0;
        break;
    case 3:
        NCOL = 24; NROW =  6;
        SET_XSIZE = GLASS_SIZE; SET_YSIZE = GLASS_SIZE;
        coloffset =  6, rowoffset = 24;
        break;
    case 4:
        NCOL =  6; NROW = 24;
        SET_XSIZE = GLASS_SIZE; SET_YSIZE = GLASS_SIZE;
        coloffset = 0, rowoffset = 6;
        break;
    default:
        printf("call_island bad sector given : %i\n",isect);
        exit(1);
    }

    for(int i = 0; i < fNHyCalHits; ++i)
    {
        int id  = fHyCalHit[i].id;
        if(fBlockINFO[id-1].sector != isect)
            continue;
        float e = fHyCalHit[i].e;

        if(e < fMinHitE)
            continue;

        int column, row;
        if(id > 1000) {
            column = (id-1001)%NCOL+1;
            row    = (id-1001)/NROW+1;
        } else {
            column = (id-1)%(NCOL+NROW)+1-coloffset;
            row    = (id-1)/(NCOL+NROW)+1-rowoffset;
        }

        ECH(column,row) = int(e*1.e4+0.5);
        ich[column-1][row-1] = id;
    }

    // main island read profile every time, too much redundant work!
    char config_path[64];
    strcpy(config_path, fConfigPath.c_str());
    main_island_(config_path, strlen(config_path));

    for(int k = 0; k < adcgam_cbk_.nadcgam; ++k)
    {
        int n = fNHyCalClusters;

        if(fNHyCalClusters == MAX_CLUSTERS)
        {
            printf("max number of clusters reached\n");
            return;
        }

        float e     = adcgam_cbk_.u.fadcgam[k][0];
        float x     = adcgam_cbk_.u.fadcgam[k][1];
        float y     = adcgam_cbk_.u.fadcgam[k][2];
        float xc    = adcgam_cbk_.u.fadcgam[k][4];
        float yc    = adcgam_cbk_.u.fadcgam[k][5];

        float chi2  = adcgam_cbk_.u.fadcgam[k][6];
        int  type   = adcgam_cbk_.u.iadcgam[k][7];
        int  dime   = adcgam_cbk_.u.iadcgam[k][8];
        int  status = adcgam_cbk_.u.iadcgam[k][10];

        if(type >= 90)
        {
            printf("island warning: cluster with type 90+ truncated\n");
            continue;
        }
        if(dime >= MAX_CC)
        {
            printf("island warning: cluster with nhits %i truncated\n",dime);
            continue;
        }

        fHyCalCluster[n].type     = type;
        fHyCalCluster[n].nhits    = dime;
        fHyCalCluster[n].E        = e;
        fHyCalCluster[n].time     = 0.0;
        fHyCalCluster[n].x        = x;        // biased, unaligned
        fHyCalCluster[n].y        = y;
        fHyCalCluster[n].chi2     = chi2;
        fHyCalCluster[n].sigma_E  = 0.0;
        fHyCalCluster[n].status   = status;

        float ecellmax = -1.; int idmax = -1;
        float sW      = 0.0;
        float xpos    = 0.0;
        float ypos    = 0.0;
        float W;

        for(int j = 0; j < (dime>MAX_CC ? MAX_CC : dime); ++j)
        {
            int id = ICL_INDEX(k,j);
            int kx = (id/100), ky = id%100;
            id = ich[kx-1][ky-1];
            fClusterStorage[n].id[j] = id;

            float ecell = 1.e-4*(float)ICL_IENER(k,j);
            float xcell = fBlockINFO[id-1].x;
            float ycell = fBlockINFO[id-1].y;

            if(type%10 == 1 || type%10 == 2)
            {
	            xcell += xc;
	            ycell += yc;
            }

            if(ecell > ecellmax)
            {
	            ecellmax = ecell;
	            idmax = id;
            }

            if(ecell > 0.)
            {
	            W  = 4.2 + log(ecell/e);
	            if(W > 0)
                {
	                sW += W;
	                xpos += xcell*W;
	                ypos += ycell*W;
	            }
            }

            fClusterStorage[n].E[j]  = ecell;
            fClusterStorage[n].x[j]  = xcell;
            fClusterStorage[n].y[j]  = ycell;

        }

        fHyCalCluster[n].sigma_E    = ecellmax;  // use it temproraly

        if(sW) {
            fHyCalCluster[n].x1    = xpos/sW;
            fHyCalCluster[n].y1    = ypos/sW;
        } else {
            printf("WRN bad cluster log. coord , center id = %i %f\n", idmax, fHyCalCluster[n].E);
            fHyCalCluster[n].x1    = 0.;
            fHyCalCluster[n].y1    = 0.;
        }

        fHyCalCluster[n].id      = idmax;

        for(int j = dime; j < MAX_CC; ++j)  // zero the rest
            fClusterStorage[n].id[j] = 0;

        fNHyCalClusters += 1;
        if(fNHyCalClusters == MAX_CLUSTERS)
        {
            printf("max number of clusters reached\n");
            return;
        }
    }
}

//_____________________________________________________________________________
void PRadIslandWrapper::GlueTransitionClusters()
{

    // find adjacent clusters and glue
    int ngroup = 0, group_number[MAX_CLUSTERS], group_size[MAX_CLUSTERS],
        group_member_list[MAX_CLUSTERS][MAX_CLUSTERS];

    for(int i = 0; i < MAX_CLUSTERS; ++i)
        group_number[i] = 0;

    for(int i = 0; i < fNHyCalClusters; ++i)
    {
        int idi = fHyCalCluster[i].id;
        //float xi = fBlockINFO[idi-1].x;
        //float yi = fBlockINFO[idi-1].y;

        for(int j = i+1; j < fNHyCalClusters; ++j)
        {
            int idj = fHyCalCluster[j].id;
            if(fBlockINFO[idi-1].sector == fBlockINFO[idj-1].sector)
                 continue;
            //float xj = fBlockINFO[idj-1].x;
            //float yj = fBlockINFO[idj-1].y;

            if(ClustersMinDist(i,j))
                continue;

            int igr = group_number[i];
            int jgr = group_number[j];

            // create new group and put theese two clusters into it
            if(!igr && !jgr)
            {
                ++ngroup;
                group_number[i] = ngroup; group_number[j] = ngroup;
                group_member_list[ngroup][0] = i;
                group_member_list[ngroup][1] = j;
                group_size[ngroup] = 2;
                continue;
            }
            // add jth cluster into group which ith cluster belongs to
            if(igr && !jgr)
            {
                group_number[j] = igr;
                group_member_list[igr][group_size[igr]] = j;
                group_size[igr] += 1;
                continue;
            }
            // add ith cluster into group which jth cluster belongs to
            if(!igr && jgr)
            {
                group_number[i] = jgr;
                group_member_list[jgr][group_size[jgr]] = i;
                group_size[jgr] += 1;
                continue;
            }
            // add jth group to ith than discard
            if(igr && jgr)
            {
                if(igr == jgr)
                    continue; // nothing to do

                int ni = group_size[igr];
                int nj = group_size[jgr];

                for(int k = 0; k < nj; ++k)
                {
                    group_member_list[igr][ni+k] = group_member_list[jgr][k];
                    group_number[group_member_list[jgr][k]] = igr;
                }

                group_size[igr] += nj;
                group_size[jgr]  = 0;
                continue;
            }
        }
    }

    for(int igr = 1; igr <= ngroup; ++igr)
    {
        if(group_size[igr]<=0)
            continue;
        // sort in increasing cluster number order:

        int list[MAX_CLUSTERS];
        for(int i = 0; i < group_size[igr]; ++i)
            list[i] = i;

        for(int i = 0; i < group_size[igr]; ++i)
            for(int j = i+1; j < group_size[igr]; ++j)
                if(group_member_list[igr][list[j]] < group_member_list[igr][list[i]])
                {
                    int l   = list[i];
                    list[i] = list[j];
                    list[j] = l;
                }

        int i0 = group_member_list[igr][list[0]];
        if(fHyCalCluster[i0].status==-1)
            continue;

        for(int k = 1; k < group_size[igr]; ++k)
        {
            int k0 = group_member_list[igr][list[k]];
            if(fHyCalCluster[k0].status==-1)
            {
                printf("glue island warning neg. status\n");
                continue;
            }

            MergeClusters(i0,k0);
            fHyCalCluster[k0].status = -1;
        }
    }

    // apply energy nonlin corr.:
    for(int i = 0; i < fNHyCalClusters; ++i)
    {
        float olde = fHyCalCluster[i].E;
        int central_id = fHyCalCluster[i].id;
        fHyCalCluster[i].E = EnergyCorrect(olde, central_id);
    }

    int ifdiscarded;

    // discrard clusters merged with others:
    do
    {
        ifdiscarded = 0;
        for(int i = 0; i < fNHyCalClusters; ++i)
        {
            if(fHyCalCluster[i].status == -1 ||
               fHyCalCluster[i].E       < SET_EMIN ||
               fHyCalCluster[i].E       > SET_EMAX ||
               fHyCalCluster[i].nhits   < SET_HMIN ||
               fHyCalCluster[i].sigma_E < SET_MINM)
            {
                fNHyCalClusters -= 1;
                int nrest = fNHyCalClusters - i;

                if(nrest)
                {
                    memmove((&fHyCalCluster[0]+i), (&fHyCalCluster[0]+i+1), nrest*sizeof(hycalcluster_t));
                    memmove((&fClusterStorage[0]+i), (&fClusterStorage[0]+i+1), nrest*sizeof(cluster_t));
                    ifdiscarded = 1;
                }
            }
        }
    } while(ifdiscarded);

}
//________________________________________________________________________
int  PRadIslandWrapper::ClustersMinDist(int i,int j)
{
    // min distance between two clusters cells cut
    double mindist = 1.e6, mindx = 1.e6, mindy = 1.e6, dx, dy, sx1, sy1, sx2, sy2;
    int dime1 = fHyCalCluster[i].nhits, dime2 = fHyCalCluster[j].nhits;

    for(int k1 = 0; k1 < (dime1>MAX_CC ? MAX_CC : dime1); ++k1)
    {
        if(fClusterStorage[i].E[k1] <= 0.)
            continue;
        if(fBlockINFO[fClusterStorage[i].id[k1]-1].sector) {
            sx1 = sy1 = GLASS_SIZE;
        } else {
            sx1 = CRYS_SIZE_X; sy1 = CRYS_SIZE_Y;
        }

        for(int k2 = 0; k2 < (dime2>MAX_CC ? MAX_CC : dime2); ++k2)
        {
            if(fClusterStorage[j].E[k2] <= 0.)
                continue;
            if(fBlockINFO[fClusterStorage[j].id[k2]-1].sector) {
                sx2 = sy2 = GLASS_SIZE;
            } else {
                sx2 = CRYS_SIZE_X; sy2 = CRYS_SIZE_Y;
            }

            dx = fClusterStorage[i].x[k1] - fClusterStorage[j].x[k2];
            dx = 0.5*dx*(1./sx1+1./sx2);
            dy = fClusterStorage[i].y[k1] - fClusterStorage[j].y[k2];
            dy = 0.5*dy*(1./sy1+1./sy2);

            if(dx*dx+dy*dy<mindist)
            {
                mindist = dx*dx+dy*dy;
                mindx = fabs(dx);
                mindy = fabs(dy);
            }
        }
    }

    return (mindx>1.1 || mindy>1.1);
}
//____________________________________________________________________________
void PRadIslandWrapper::MergeClusters(int i, int j)
{
    // ith cluster gona stay; jth cluster gona be discarded
    // leave cluster with greater energy deposition in central cell, discard the second one:
    int i1 = (fHyCalCluster[i].sigma_E > fHyCalCluster[j].sigma_E) ? i : j;
    int dime1 = fHyCalCluster[i].nhits, dime2 = fHyCalCluster[j].nhits;
    //int type1 = fHyCalCluster[i].type, type2 = fHyCalCluster[j].type;
    float e = fHyCalCluster[i].E + fHyCalCluster[j].E;

    fHyCalCluster[i].id      = fHyCalCluster[i1].id;
    fHyCalCluster[i].sigma_E = fHyCalCluster[i1].sigma_E;
    fHyCalCluster[i].x       = fHyCalCluster[i1].x;
    fHyCalCluster[i].y       = fHyCalCluster[i1].y;
    fHyCalCluster[i].chi2    = (fHyCalCluster[i].chi2*dime1 + fHyCalCluster[j].chi2*dime2)/(dime1+dime2);

    if(fHyCalCluster[i].status>=2)
        fHyCalCluster[i].status += 1; // glued

    fHyCalCluster[i].nhits   = dime1+dime2;
    fHyCalCluster[i].E       = e;
    fHyCalCluster[i].type    = fHyCalCluster[i1].type;

    for(int k1 = 0; k1 < (dime1>MAX_CC ? MAX_CC : dime1); ++k1)
    {
        for(int k2 = 0; k2 < (dime2>MAX_CC ? MAX_CC : dime2); ++k2)
        {
            if(fClusterStorage[i].id[k1] == fClusterStorage[j].id[k2])
            {
                if(fClusterStorage[j].E[k2] > 0.)
                {
                    fClusterStorage[i].E[k1] += fClusterStorage[j].E[k2];
                    fClusterStorage[i].x[k1] += fClusterStorage[j].x[k2];
                    fClusterStorage[i].x[k1] *= 0.5;
                    fClusterStorage[i].y[k1] += fClusterStorage[j].y[k2];
                    fClusterStorage[i].y[k1] *= 0.5;

                    fClusterStorage[j].E[k2] = 0.;
                    fClusterStorage[j].x[k2] = 0.;
                    fClusterStorage[j].y[k2] = 0.;
                }
                continue;
            }
        }
    }

    float sW      = 0.0;
    float xpos    = 0.0;
    float ypos    = 0.0;
    float W;

    for(int k1 = 0; k1 < (dime1>MAX_CC ? MAX_CC : dime1); ++k1)
    {
        float ecell = fClusterStorage[i].E[k1];
        float xcell = fClusterStorage[i].x[k1];
        float ycell = fClusterStorage[i].y[k1];
        if(ecell>0.)
        {
            W  = 4.2 + log(ecell/e);
            if(W > 0)
            {
                sW += W;
                xpos += xcell*W;
                ypos += ycell*W;
            }
        }
    }

    for(int k2 = 0; k2 < (dime2>MAX_CC ? MAX_CC : dime2); ++k2)
    {
        float ecell = fClusterStorage[j].E[k2];
        float xcell = fClusterStorage[j].x[k2];
        float ycell = fClusterStorage[j].y[k2];

        if(ecell>0.)
        {
            W  = 4.2 + log(ecell/e);
            if(W > 0)
            {
                sW += W;
                xpos += xcell*W;
                ypos += ycell*W;
            }
        }
    }

    if(sW) {
        float dx = fHyCalCluster[i1].x1;
        float dy = fHyCalCluster[i1].y1;
        fHyCalCluster[i].x1 = xpos/sW;
        fHyCalCluster[i].y1 = ypos/sW;
        dx = fHyCalCluster[i1].x1 - dx;
        dy = fHyCalCluster[i1].y1 - dy;
        fHyCalCluster[i].x += dx;      // shift x0,y0 for glued cluster by x1,y1 shift values
        fHyCalCluster[i].y += dy;
    } else {
        printf("WRN bad cluster log. coord\n");
        fHyCalCluster[i].x1 = 0.;
        fHyCalCluster[i].y1 = 0.;
    }

    // update cluster_storage
    int kk = dime1;

    for(int k = 0; k < (dime2>MAX_CC-dime1 ? MAX_CC-dime1 : dime2); ++k)
    {
        if(fClusterStorage[j].E[k] <= 0.)
            continue;
        if(kk >= MAX_CC)
        {
            printf("WARINING: number of cells in cluster exceeded max %i\n", dime1+dime2);
            break;
        }
        fClusterStorage[i].id[kk] = fClusterStorage[j].id[k];
        fClusterStorage[i].E[kk]  = fClusterStorage[j].E[k];
        fClusterStorage[i].x[kk]  = fClusterStorage[j].x[k];
        fClusterStorage[i].y[kk]  = fClusterStorage[j].y[k];
        fClusterStorage[j].id[k]  = 0;
        fClusterStorage[j].E[k]   = 0.;
        ++kk;
    }

    fHyCalCluster[i].nhits = kk;
}
//____________________________________________________________________________
float PRadIslandWrapper::EnergyCorrect (float c_energy, int /*central_id*/)
{
    float energy = c_energy;
    //float fr = (central_id < 999) ? 0.062 : 0.042;

    //float ecorr = 1. - fr * exp(-Nonlin_en1[central_id-1]*energy);

    //ecorr *= 1. - Nonlin_en2[central_id-1] * energy*1.e-3;
    //if(fabs(ecorr-1.) < 0.3) energy /= ecorr;

    return energy;
}
//____________________________________________________________________________
void PRadIslandWrapper::ClusterProcessing()
{
    float pi = 3.1415926535;
    //  final cluster processing (add status and energy resolution sigma_E):
    for(int i = 0; i < fNHyCalClusters; ++i)
    {
        float e   = fHyCalCluster[i].E;
        int idmax = fHyCalCluster[i].id;

        // apply 1st approx. non-lin. corr. (was obtained for PWO):
        e *= 1. + 200.*pi/pow((pi+e),7.5);
        fHyCalCluster[i].E = e;

        //float x   = fHyCalCluster[i].x1;
        //float y   = fHyCalCluster[i].y1;

        //coord_align(i, e, idmax);
        int status = fHyCalCluster[i].status;
        int type   = fHyCalCluster[i].type;
        int dime   = fHyCalCluster[i].nhits;

        if(status < 0)
        {
            printf("error in island : cluster with negative status");
            exit(1);
        }

        int itp, ist;
        itp  = (idmax>1000) ? 0 : 10;
        if(status==1)
            itp += 1;

        for(int k = 0; k < (dime>MAX_CC ? MAX_CC : dime); ++k)
        {
            if(itp<10) {
                if(fClusterStorage[i].id[k] < 1000)
                {
                    itp =  2;
                    break;
                }
            } else {
                if(fClusterStorage[i].id[k] > 1000)
                {
                    itp = 12;
                    break;
                }
            }
        }

        fHyCalCluster[i].type = itp;

        ist = type;

        if(status > 2)
            ist += 20; // glued
        fHyCalCluster[i].status = ist;

        double se = (idmax>1000) ? sqrt(0.9*0.9*e*e+2.5*2.5*e+1.0) : sqrt(2.3*2.3*e*e+5.4*5.4*e);
        se /= 100.;
        if(itp%10 == 1) {
            se *= 1.5;
        } else if(itp%10 == 2) {
            if(itp>10)
                se *= 0.8;
            else
                se *=1.25;
        }
        fHyCalCluster[i].sigma_E = se;
    }
}
