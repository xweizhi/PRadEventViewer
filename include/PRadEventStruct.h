#ifndef PRAD_EVENT_STRUCT_H
#define PRAD_EVENT_STRUCT_H

#include <string>
#include <vector>
#include <deque>
#include <utility>

// some discriminator related settings
#define REF_CHANNEL 7
#define REF_PULSER_FREQ 500000

#define FCUP_CHANNEL 6
#define FCUP_OFFSET 100.0
#define FCUP_SLOPE 906.2

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

    // move constructor
    EventData(const EventData &e)
    : event_number(e.event_number), type(e.type), trigger(e.trigger), timestamp(e.timestamp),
      adc_data(std::move(e.adc_data)),
      tdc_data(std::move(e.tdc_data)),
      gem_data(std::move(e.gem_data)),
      dsc_data(std::move(e.dsc_data))
    {};

    EventData(EventData &&e)
    : event_number(e.event_number), type(e.type), trigger(e.trigger), timestamp(e.timestamp),
      adc_data(std::move(e.adc_data)),
      tdc_data(std::move(e.tdc_data)),
      gem_data(std::move(e.gem_data)),
      dsc_data(std::move(e.dsc_data))
    {};

    // functions
    void initialize(const unsigned char &t = 0) // for data taking
    {
        type = t;
        trigger = 0;
        adc_data.clear();
        tdc_data.clear();
        gem_data.clear();
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

    bool is_physics_event()
    {
        return ( (trigger == PHYS_LeadGlassSum) ||
                 (trigger == PHYS_TotalSum)     ||
                 (trigger == PHYS_TaggerE)      ||
                 (trigger == PHYS_Scintillator) );
    };
    bool is_monitor_event()
    {
        return ( (trigger == LMS_Led) ||
                 (trigger == LMS_Alpha) );
    };

    double get_beam_time()
    {
        double elapsed_time = 0.;
        if(dsc_data.size() > REF_CHANNEL)
        {
            elapsed_time = (double)dsc_data.at(REF_CHANNEL).ungated_count/(double)REF_PULSER_FREQ;
        }

        return elapsed_time;
    };

    double get_live_time()
    {
        double live_time = 1.;
        if(dsc_data.size() > REF_CHANNEL)
        {
            live_time -= (double)dsc_data.at(REF_CHANNEL).gated_count/(double)dsc_data.at(REF_CHANNEL).ungated_count;
        }

        return live_time;
    };

    double get_beam_charge()
    {
        double beam_charge = 0.;
        if(dsc_data.size() > FCUP_CHANNEL)
        {
            beam_charge = ((double)dsc_data.at(FCUP_CHANNEL).ungated_count - FCUP_OFFSET)/FCUP_SLOPE;
        }

        return beam_charge;
    };

    double get_beam_current()
    {
        double beam_time = get_beam_time();
        if(beam_time > 0.)
            return get_beam_charge()/beam_time;
        else
            return 0.;
    };

    DSC_Data get_dsc_channel(const uint32_t &idx)
    {
        if(dsc_data.size() <= idx)
            return DSC_Data();
        else
            return dsc_data.at(idx);
    };

    DSC_Data get_ref_channel()
    {
        return get_dsc_channel(REF_CHANNEL);
    };

    DSC_Data get_dsc_scaled_by_ref(const uint32_t &idx)
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

struct HyCalHit
{
    double x;
    double y;
    double E;
    std::vector<unsigned short> time;

    HyCalHit() : x(0), y(0), E(0)
    {};
    HyCalHit(const double &cx, const double &cy, const double &cE, const std::vector<unsigned short> &t)
    : x(cx), y(cy), E(cE), time(t)
    {};
};

#endif
