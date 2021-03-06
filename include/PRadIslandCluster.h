#ifndef PRAD_ISLAND_CLUSTER_H
#define PRAD_ISLAND_CLUSTER_H

#include <string>
#include "PRadHyCalCluster.h"
#include <vector>

//this is a c++ wrapper around the primex island algorithm
//used for HyCal cluster reconstruction

//define some global constants
#define MSECT 5
#define MCOL 34
#define MROW 34

#define CRYSTAL_BLOCKS 1156 // 34x34 array (2x2 hole in the center)
#define GLASS_BLOCKS 900    // 30x30 array holes are also counted (18x18 in the center)
#define BLANK_BLOCKS 100    // For simplifying indexes

#define T_BLOCKS 2156

#define MAX_HHITS 1728 // For Hycal

#define nint_phot_cell  5
#define ncoef_phot_cell 3
#define dcorr_phot_cell 16

#define CRYS_HALF_ROWS 17 //CRYS_ROWS/2
#define GLASS_HALF_ROWS 15 //GLASS_ROWS/2
#define CRYS_HALF_SIZE_X 1.0385   // CRYS_SIZE_X/2
#define CRYS_HALF_SIZE_Y 1.0375   // CRYS_SIZE_Y/2
#define GLASS_HALF_SIZE 1.9075    // GLASS_SIZE/2
#define GLASS_OFFSET_X CRYS_SIZE_X*CRYS_ROWS //Distance from center to glass by X axis
#define GLASS_OFFSET_Y CRYS_SIZE_Y*CRYS_ROWS //Distance from center to glass by Y axis

#define CRYS_ROWS 34
#define GLASS_ROWS 30
#define CRYS_SIZE_X 2.077   // real X-size of crystal
#define CRYS_SIZE_Y 2.075   // real Y-size of crystal
#define GLASS_SIZE 3.815    // real size of glass

typedef struct
{
    int id;                 // ID of block
    float x;                // Center of block x-coord
    float y;                // Center of block y-coord
    int sector;             // 0 for W, 1 - 4 for Glass (clockwise starting at noon)
    int row;                // row number within sector
    int col;                // column number within sector
} blockINFO_t;


typedef struct
{
    int  id[MAX_CC];   // ID of ith block, where i runs from 0 to 8
    float E[MAX_CC];   // Energy of ith block
    float x[MAX_CC];   // Center of ith block x-coord
    float y[MAX_CC];   // Center of ith block y-coord
} cluster_t;

typedef struct
{
    int  id;   // ID of ADC
    float e;   // Energy of ADC
} cluster_block_t;

extern "C"
{
    void load_pwo_prof_(char* config_dir, int str_len);
    void load_lg_prof_(char* config_dir, int str_len);
    void main_island_();
    extern struct
    {
        int ech[MROW][MCOL];
    } ech_common_;

    extern struct
    {
        int stat_ch[MROW][MCOL];
    } stat_ch_common_;

    extern struct
    {
        int icl_index[MAX_CC][200], icl_iener[MAX_CC][200];
    } icl_common_;

    extern struct
    {
        float xsize, ysize, mine, maxe;
        int min_dime;
        float minm;
        int ncol, nrow;
        float zhycal;
        int isect;
    } set_common_;

    extern struct
    {
        int nadcgam;
        union
        {
            int iadcgam[50][11];
            float fadcgam[50][11];
        } u;
    } adcgam_cbk_;

    extern struct
    {
        float fa[100];
    } hbk_common_;

    #define ECH(M,N) ech_common_.ech[N-1][M-1]
    #define STAT_CH(M,N) stat_ch_common_.stat_ch[N-1][M-1]
    #define ICL_INDEX(M,N) icl_common_.icl_index[N][M]
    #define ICL_IENER(M,N) icl_common_.icl_iener[N][M]
    #define HEGEN(N) read_mcfile_com_.hegen[N-1]
    #define SET_XSIZE set_common_.xsize
    #define SET_YSIZE set_common_.ysize
    #define SET_EMIN  set_common_.mine
    #define SET_EMAX  set_common_.maxe
    #define SET_HMIN  set_common_.min_dime
    #define SET_MINM  set_common_.minm
    #define NCOL      set_common_.ncol
    #define NROW      set_common_.nrow
    #define ZHYCAL    set_common_.zhycal
    #define ISECT     set_common_.isect
    #define FA(N) hbk_common_.fa[N-1]
}

class PRadIslandCluster : public PRadHyCalCluster
{
public:
    PRadIslandCluster(PRadDataHandler *h = nullptr);
    virtual ~PRadIslandCluster() {;}

    void  SetHandler(PRadDataHandler* theHandler) { fHandler = theHandler; }
    void  Configure(const std::string &c_path);
    void  LoadBlockInfo(const std::string &path);
    void  LoadCrystalProfile(const std::string &path);
    void  LoadLeadGlassProfile(const std::string &path);
    void  LoadNonLinearity(const std::string &path);
    void  Reconstruct(EventData &event);
    void  Clear();

protected:
    void  LoadModuleData(EventData& thisEvent);
    void  CallIsland(int isect);
    void  GlueTransitionClusters();
    void  ClusterProcessing();
    int   ClustersMinDist(int i,int j);
    void  MergeClusters(int i, int j);
    float EnergyCorrect (float c_energy, int central_id);
    void  FinalProcessing();
    float GetShowerDepth(int type, float & E);
    float GetDoubleExpWeight(float& e, float& E);
    float Finv(float y);
    float Fx(float& x);
    

    int   fDoShowerDepth;
    int   fUse2ExpWeight;
    int   fDoNonLinCorr;
    float fMinHitE;
    float fWeightFreePar; 
    float fMinClusterE;
    float fMinCenterE;
    float fZLGToPWO;
    float fZHyCal;
    float fCutOffThr;
    float f2ExpFreeWeight;

    blockINFO_t fBlockINFO[T_BLOCKS];
    float fNonLin[T_BLOCKS];
    int fModuleStatus[MSECT][MCOL][MROW];
    int fNClusterBlocks;
    cluster_block_t fClusterBlock[T_BLOCKS];
    cluster_t fClusterStorage[MAX_HCLUSTERS];
    int ich[MCOL][MROW];
    std::vector<blockINFO_t> fDeadModules;
    float fE0[2];
    float fZ0[2];
};

#endif
