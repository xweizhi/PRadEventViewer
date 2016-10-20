#ifndef PRAD_EVENT_STRUCT_H
#define PRAD_EVENT_STRUCT_H

#include <string>
#include <vector>
#include <deque>
#include <utility>
#include "datastruct.h"
#include "TObject.h"

// some discriminator related settings
#define REF_CHANNEL 7
#define REF_PULSER_FREQ 500000

#define FCUP_CHANNEL 6
#define FCUP_OFFSET 100.0
#define FCUP_SLOPE 906.2

#define MAX_CC 60
#define MAX_GEM_MATCH 100
#define NGEM 2

struct RunInfo
{
    int run_number;
    double beam_charge;
    double dead_count;
    double ungated_count;

    RunInfo()
    : run_number(0), beam_charge(0.), dead_count(0.), ungated_count(0.)
    {};

    RunInfo(const int &run, const double &c, const double &d, const double &ug)
    : run_number(run), beam_charge(c), dead_count(d), ungated_count(ug)
    {};

    void clear()
    {
        run_number = 0;
        beam_charge = 0.;
        dead_count = 0.;
        ungated_count = 0.;
    }
};

struct TriggerChannel
{
    std::string name;
    uint32_t id;
    double freq;

    TriggerChannel()
    : name("undefined"), id(0), freq(0.)
    {};
    TriggerChannel(const std::string &n, const uint32_t &i)
    : name(n), id(i), freq(0.)
    {};
};

// online information
struct OnlineInfo
{
    double live_time;
    double beam_current;
    std::vector<TriggerChannel> trigger_info;

    OnlineInfo()
    : live_time(0.), beam_current(0.)
    {};

    void add_trigger(const std::string &n, const uint32_t &i)
    {
        trigger_info.emplace_back(n, i);
    };
};

typedef struct ChannelData
{
    unsigned short channel_id;
    unsigned short value;

    ChannelData()
    : channel_id(0), value(0)
    {};
    ChannelData(const unsigned short &i, const unsigned short &v)
    : channel_id(i), value(v)
    {};

} TDC_Data, ADC_Data;

struct DSC_Data
{
    unsigned int gated_count;
    unsigned int ungated_count;

    DSC_Data()
    : gated_count(0), ungated_count(0)
    {};
    DSC_Data(const unsigned int &g1, const unsigned int &g2)
    : gated_count(g1), ungated_count(g2)
    {};
};

struct APVAddress
{
    unsigned char fec;
    unsigned char adc;
    unsigned char strip;

    APVAddress() {};
    APVAddress(const unsigned char &f,
               const unsigned char &a,
               const unsigned char &s)
    : fec(f), adc(a), strip(s)
    {};
};

struct GEM_Data
{
    APVAddress addr;
    std::vector<float> values;

    GEM_Data() {};
    GEM_Data(const unsigned char &f,
             const unsigned char &a,
             const unsigned char &s)
    : addr(f, a, s)
    {};

    void set_address (const unsigned char &f,
                      const unsigned char &a,
                      const unsigned char &s)
    {
        addr.fec = f;
        addr.adc = a;
        addr.strip = s;
    }

    void add_value(const float &v)
    {
        values.push_back(v);
    }
};

struct EPICSData
{
    int event_number;
    std::vector<float> values;

    EPICSData()
    {};
    EPICSData(const int &ev, std::vector<float> &val)
    : event_number(ev), values(val)
    {};

    void clear()
    {
        event_number = 0;
        values.clear();
    };
};

struct EventData
{
    // event info
    int event_number;
    unsigned char type;
    unsigned char trigger;
    uint64_t timestamp;

    // data banks
    std::vector< ADC_Data > adc_data;
    std::vector< TDC_Data > tdc_data;
    std::vector< GEM_Data > gem_data;
    std::vector< DSC_Data > dsc_data;

    // constructors
    EventData()
    : event_number(0), type(0), trigger(0), timestamp(0)
    {};
    EventData(const unsigned char &t)
    : event_number(0), type(t), trigger(0), timestamp(0)
    {};
    EventData(const unsigned char &t,
              const PRadTriggerType &trg,
              std::vector<ADC_Data> &adc,
              std::vector<TDC_Data> &tdc,
              std::vector<GEM_Data> &gem,
              std::vector<DSC_Data> &dsc)
    : event_number(0), type(t), trigger((unsigned char)trg), timestamp(0),
      adc_data(adc), tdc_data(tdc), gem_data(gem), dsc_data(dsc)
    {};

    // functions
    void initialize(const unsigned char &t = 0) // for data taking
    {
        type = t;
        trigger = 0;
        adc_data.clear();
        tdc_data.clear();
        gem_data.clear();
        dsc_data.clear();
    };
    void clear() // fully clear
    {
        initialize();
        event_number = 0;
        timestamp = 0;
    };

    void update_type(const unsigned char &t) {type = t;};
    void update_trigger(const unsigned char &t) {trigger = t;};
    void update_time(const uint64_t &t) {timestamp = t;};

    void add_adc(const ADC_Data &a) {adc_data.push_back(a);};
    void add_tdc(const TDC_Data &t) {tdc_data.push_back(t);};
    void add_gemhit(const GEM_Data &g) {gem_data.push_back(g);};
    void add_dsc(const DSC_Data &d) {dsc_data.push_back(d);};

    std::vector<ADC_Data> &get_adc_data() {return adc_data;};
    std::vector<TDC_Data> &get_tdc_data() {return tdc_data;};
    std::vector<GEM_Data> &get_gem_data() {return gem_data;};
    std::vector<DSC_Data> &get_dsc_data() {return dsc_data;};

    const std::vector<ADC_Data> &get_adc_data() const {return adc_data;};
    const std::vector<TDC_Data> &get_tdc_data() const {return tdc_data;};
    const std::vector<GEM_Data> &get_gem_data() const {return gem_data;};
    const std::vector<DSC_Data> &get_dsc_data() const {return dsc_data;};

    bool is_physics_event() const
    {
        return ( (trigger == PHYS_LeadGlassSum) ||
                 (trigger == PHYS_TotalSum)     ||
                 (trigger == PHYS_TaggerE)      ||
                 (trigger == PHYS_Scintillator) );
    };
    bool is_monitor_event() const
    {
        return ( (trigger == LMS_Led) ||
                 (trigger == LMS_Alpha) );
    };

    double get_beam_time() const
    {
        double elapsed_time = 0.;
        if(dsc_data.size() > REF_CHANNEL)
        {
            elapsed_time = (double)dsc_data.at(REF_CHANNEL).ungated_count/(double)REF_PULSER_FREQ;
        }

        return elapsed_time;
    };

    double get_live_time() const
    {
        double live_time = 1.;
        if(dsc_data.size() > REF_CHANNEL)
        {
            live_time -= (double)dsc_data.at(REF_CHANNEL).gated_count/(double)dsc_data.at(REF_CHANNEL).ungated_count;
        }

        return live_time;
    };

    double get_beam_charge() const
    {
        double beam_charge = 0.;
        if(dsc_data.size() > FCUP_CHANNEL)
        {
            beam_charge = ((double)dsc_data.at(FCUP_CHANNEL).ungated_count - FCUP_OFFSET)/FCUP_SLOPE;
        }

        return beam_charge;
    };

    double get_beam_current() const
    {
        double beam_time = get_beam_time();
        if(beam_time > 0.)
            return get_beam_charge()/beam_time;
        else
            return 0.;
    };

    DSC_Data get_dsc_channel(const uint32_t &idx) const
    {
        if(dsc_data.size() <= idx)
            return DSC_Data();
        else
            return dsc_data.at(idx);
    };

    DSC_Data get_ref_channel() const
    {
        return get_dsc_channel(REF_CHANNEL);
    };

    DSC_Data get_dsc_scaled_by_ref(const uint32_t &idx) const
    {
        if(idx >= dsc_data.size())
            return DSC_Data();

        uint64_t ref_pulser = get_ref_channel().ungated_count;
        uint64_t ungated_scaled = (dsc_data.at(idx).ungated_count*ref_pulser)/REF_PULSER_FREQ;
        uint64_t gated_scaled = (dsc_data.at(idx).gated_count*ref_pulser)/REF_PULSER_FREQ;

        return DSC_Data((unsigned int)gated_scaled, (unsigned int)ungated_scaled);
    };

    bool operator == (const int &ev) const
    {
        return ev == event_number;
    };
    bool operator > (const int &ev) const
    {
        return ev > event_number;
    };
    bool operator < (const int &ev) const
    {
        return ev < event_number;
    };
    bool operator > (const EventData &other) const
    {
        return other.event_number > event_number;
    };
    bool operator < (const EventData &other) const
    {
        return other.event_number < event_number;
    };
};

enum HyCalClusterStatus{
    kPWO = 0,     //cluster center at PWO region
    kLG,          //cluster center at LG region
    kTransition,  //cluster center at LG region
    kSplit,       //cluster after splitting
    kDeadModule,  //cluster near dead module
    kInnerBound,  //cluster near the inner hole of HyCal
    kOuterBound,  //cluster near the outer boundary of HyCal
    kGEM1Match,   //cluster found a match GEM hit on GEM 1
    kGEM2Match,   //cluster found a match GEM hit on GEM 2
    kGEMMatch,    //cluster found a match GEM hit from either one
    kOverlapMatch,//cluster found a match on both GEM
    kMultiGEMHits //multiple GEM hits appear near HyCal Hit
};

struct HyCalHit
{
#define TIME_MEASURE_SIZE 3
    unsigned int flag;  // overall status of the cluster
    short type;         // Cluster types: 0,1,2,3,4;-1
    short status;       // Spliting status
    short nblocks;      // Number of blocks in a cluster
    short cid;          // Cluster's central cell ID
    float E;            // Cluster's energy (MeV)
    float x;            // Cluster's x-position (mm)
    float y;            // Cluster's y-position (mm)
    float x_log;        // x reconstruct with log scale (mm)
    float y_log;        // y reconstruct with log scale (mm)
    float x_gem;        // x coor from GEM after match
    float y_gem;        // y coor from GEM after match
    float z_gem;        // z coor from GEM after match
    float chi2;         // chi2 comparing to shower profile
    float sigma_E;
    float dz;           // z shift due to shower depth corretion
    unsigned short time[TIME_MEASURE_SIZE];      // time information from central TDC group
    unsigned short moduleID[MAX_CC];
    float moduleE[MAX_CC];

    //for GEM HyCal match
    unsigned short gemClusterID[NGEM][MAX_GEM_MATCH];
    unsigned short gemNClusters[NGEM];

    HyCalHit()
    : flag(0), type(0), status(0), nblocks(0), cid(0), E(0), x(0), y(0), x_log(0),
      y_log(0), chi2(0), sigma_E(0), dz(0)
    {
        clear_time();
        clear_gem();
        for (int i=0; i<NGEM; i++) gemNClusters[i] = 0;
    }

    HyCalHit(const float &cx, const float &cy, const float &cE)
    : flag(0), type(0), status(0), nblocks(0), cid(0), E(cE), x(cx), y(cy), x_log(0),
      y_log(0), chi2(0), sigma_E(0), dz(0)
    {
        clear_time();
        clear_gem();
        for (int i=0; i<NGEM; i++) gemNClusters[i] = 0;
    }

    HyCalHit(const float &cx, const float &cy, const float &cE, const std::vector<unsigned short> &t)
    : flag(0), type(0), status(0), nblocks(0), cid(0), E(cE), x(cx), y(cy), x_log(0),
      y_log(0), chi2(0), sigma_E(0), dz(0)
    {
        set_time(t);
    }

    HyCalHit(const short &t, const short &s, const short &n,
             const float &cx, const float &cy, const float &cE, const float &ch)
    : flag(0), type(t), status(s), nblocks(n), cid(0), E(cE), x(cx), y(cy), x_log(0),
      y_log(0), chi2(ch), sigma_E(0), dz(0)
    {
        clear_time();
        clear_gem();
        for (int i=0; i<NGEM; i++) gemNClusters[i] = 0;
    }
    void clear_time()
    {
        for(int i = 0; i < TIME_MEASURE_SIZE; ++i)
            time[i] = 0;
    }
    void set_time(const std::vector<unsigned short> &t)
    {
        for(int i = 0; i < TIME_MEASURE_SIZE; ++i)
        {
            if(i < (int)t.size())
                time[i] = t[i];
            else
                time[i] = 0;
        }
    }

    void add_gem_clusterID(const int & detid, const unsigned short &id){
        if (gemNClusters[detid] >= MAX_GEM_MATCH) return;
        gemClusterID[detid][gemNClusters[detid]] = id;
        gemNClusters[detid]++;
    }

    void clear_gem(){
        for (int i=0; i<NGEM; i++) gemNClusters[i] = 0;
        x_gem = 0.; y_gem = 0.; z_gem = -1;
    }

    void DistClean(){
        clear_time();
        clear_gem();
        flag = 0;
    }
};

struct GEMDetCluster {
    float x;
    float y;
    float z;
    float x_charge;
    float y_charge;
    int x_size;
    int y_size;
    int chamber_id;

    GEMDetCluster()
    :x(0.), y(0.), z(0.), x_charge(0.), y_charge(0.),
     x_size(0), y_size(0), chamber_id(-1) {}

    GEMDetCluster(const float& cx, const float& cy, const float& cz,
                  const float& cx_charge, const float& cy_charge,
                  const int& cx_size, const int& cy_size, const int& cchamber_id)
    :x(cx), y(cy), z(cz), x_charge(cx_charge), y_charge(cy_charge),
     x_size(cx_size), y_size(cy_size), chamber_id(cchamber_id) {}

    GEMDetCluster(const GEMDetCluster& rhs)
    :x(rhs.x), y(rhs.y), z(rhs.z), x_charge(rhs.x_charge), y_charge(rhs.y_charge),
     x_size(rhs.x_size), y_size(rhs.y_size), chamber_id(rhs.chamber_id) {}
};

// DST file related info
enum PRadDSTInfo
{
    // event types
    PRad_DST_Event = 0,
    PRad_DST_Epics,
    PRad_DST_Epics_Map,
    PRad_DST_Run_Info,
    PRad_DST_HyCal_Info,
    PRad_DST_GEM_Info,
    PRad_DST_Undefined,
};

enum PRadDSTHeader
{
    // headers
    PRad_DST_Header = 0xc0c0c0,
    PRad_DST_EvHeader = 0xe0e0e0,
};

enum PRadDSTMode
{
    DST_UPDATE_ALL = 0,
    NO_GEM_PED_UPDATE = 1 << 0,
    NO_HYCAL_PED_UPDATE = 1 << 1,
    NO_HYCAL_CAL_UPDATE = 1 << 2,
    NO_RUN_INFO_UPDATE = 1 << 3,
    NO_EPICS_MAP_UPDATE = 1 << 4,
    DST_UPDATE_NONE = 0xffffffff,
};

#endif
