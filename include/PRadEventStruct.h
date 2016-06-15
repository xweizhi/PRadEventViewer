#ifndef PRAD_EVENT_STRUCT_H
#define PRAD_EVENT_STRUCT_H

#include <string>
#include <vector>

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

struct GEM_Data
{
    unsigned char fec;
    unsigned char adc;
    unsigned char strip;
    unsigned short value;

    GEM_Data()
    : fec(0), adc(0), strip(0), value(0)
    {};
    GEM_Data(const unsigned char &f,
             const unsigned char &a,
             const unsigned char &s,
             const unsigned short &v)
    : fec(f), adc(a), strip(s), value(v)
    {};
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
// members
    int event_number;
    unsigned char type;
    unsigned char trigger;
    uint64_t timestamp;
    int last_epics;
    std::vector< ADC_Data > adc_data;
    std::vector< TDC_Data > tdc_data;
    std::vector< GEM_Data > gem_data;

// constructors
    EventData()
    : type(0), trigger(0), timestamp(0), last_epics(-1)
    {};
    EventData(const unsigned char &t)
    : type(t), trigger(0), timestamp(0), last_epics(-1)
    {};
    EventData(const unsigned char &t,
              const PRadTriggerType &tr,
              std::vector<ADC_Data> &adc,
              std::vector<TDC_Data> &tdc,
              std::vector<GEM_Data> &gem)
    : type(t), trigger((unsigned char)tr), timestamp(0), last_epics(-1),
      adc_data(adc), tdc_data(tdc), gem_data(gem)
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
        last_epics = -1;
    };
    void update_type(const unsigned char &t) {type = t;};
    void update_trigger(const unsigned char &t) {trigger = t;};
    void update_time(const uint64_t &t) {timestamp = t;};
    void add_adc(const ADC_Data &a) {adc_data.push_back(a);};
    void add_tdc(const TDC_Data &t) {tdc_data.push_back(t);};
    void add_gem(const GEM_Data &g) {gem_data.push_back(g);};
    bool isPhysicsEvent()
    {
        return ( (trigger == PHYS_LeadGlassSum) ||
                 (trigger == PHYS_TotalSum)     ||
                 (trigger == PHYS_TaggerE)      ||
                 (trigger == PHYS_Scintillator) );
    };
    bool isMonitorEvent()
    {
        return ( (trigger == LMS_Led) ||
                 (trigger == LMS_Alpha) );
    }
};

struct ScalarChannel
{
    std::string name;
    unsigned int count;
    unsigned int gated_count;

    ScalarChannel() : name("undefined"), count(0), gated_count(0) {};
    ScalarChannel(const std::string &n) : name(n), count(0), gated_count(0) {};
};

struct HyCalHit
{
    double x;
    double y;
    double E;

    HyCalHit() : x(0), y(0), E(0)
    {};
    HyCalHit(const double &cx, const double &cy, const double &cE)
    : x(cx), y(cy), E(cE)
    {};
};

#endif
