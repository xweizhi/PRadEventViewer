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

struct EventData
{
    int event_number;
    unsigned char type;
    unsigned char latch_word;
    unsigned char lms_phase;
    uint64_t timestamp;
    std::vector< ADC_Data > adc_data;
    std::vector< TDC_Data > tdc_data;
    std::vector< GEM_Data > gem_data;

    EventData()
    : type(0), lms_phase(0), timestamp(0)
    {};
    EventData(const PRadTriggerType &t)
    : type((unsigned char)t), lms_phase(0), timestamp(0)
    {};
    EventData(const PRadTriggerType &t, std::vector< ADC_Data > &adc, std::vector< TDC_Data > &tdc)
    : type((unsigned char)t), lms_phase(0), timestamp(0), adc_data(adc), tdc_data(tdc)
    {};

    void clear() {type = 0; adc_data.clear(); tdc_data.clear();};
    void update_type(const unsigned char &t) {type = t;};
    void update_time(const uint64_t &t) {timestamp = t;};
    void add_adc(const ADC_Data &a) {adc_data.push_back(a);};
    void add_tdc(const TDC_Data &t) {tdc_data.push_back(t);};
    void add_gem(const GEM_Data &g) {gem_data.push_back(g);};
    bool isPhysicsEvent()
    {
        return ( (type == PHYS_LeadGlassSum) ||
                 (type == PHYS_TotalSum)     ||
                 (type == PHYS_TaggerE)      ||
                 (type == PHYS_Scintillator) );
    };
};

struct EPICSValue
{
    int att_event;
    float value;

    EPICSValue()
    : att_event(0), value(0)
    {};
    EPICSValue(const int &e, const float &v)
    : att_event(e), value(v)
    {};

    bool operator < (const int &e) const
    {
        return att_event < e;
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
