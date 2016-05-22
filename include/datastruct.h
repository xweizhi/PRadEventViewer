#ifndef PRAD_DATA_STRUCT_H
#define PRAD_DATA_STRUCT_H

#include <string>

enum PRadEventType
{
    CODA_Unknown = 0x0,
    CODA_Event = 0x81,
    CODA_Prestart = 0x11,
    CODA_Go = 0x12,
    CODA_End = 0x20,
};

enum PRadTriggerType
{
    TI_Internal = 0,
    PULS_Pedestal,
    PHYS_TotalSum,
    LMS_Led,
    LMS_Alpha,
    PHYS_TaggerE,
    PULS_Clock,
    MAX_Trigger,
};

#define PHYS_TYPE (PHYS_TotalSum | PHYS_TaggerE)
#define LMS_TYPE (LMS_Led | LMS_Alpha)
#define PULS_TYPE (PULS_Pedestal | PULS_Clock)

enum EvioBankType
{
    Unknown_32bit = 0x0,
    UnsignedInt_32bit = 0x01,
    Float_32bit = 0x02,
    CharString_8bit = 0x03,
    SignedShort_16bit = 0x04,
    UnsignedShort_16bit = 0x05,
    SignedChar_8bit = 0x06,
    UnsignedChar_8bit = 0x07,
    Double_64bit = 0x08,
    SignedInt_64bit = 0x09,
    UnsignedInt_64bit = 0x0a,
    SignedInt_32bit = 0x0b,
    EvioTagSegment = 0x0c,
    EvioSegment_B = 0x0d,
    EvioBank_B = 0x0e,
    EvioComposite = 0x0f,
    EvioBank = 0x10,
    EvioSegment = 0x20,
    EvioHollerit = 0x21,
    EvioNValue = 0x22,
};

enum PRadROCID
{
    PRadTS = 1,
    PRadTagE = 2,
    PRadROC_1 = 4,
    PRadROC_2 = 5,
    PRadROC_3 = 6,
    PRadSRS_1 = 7,
    PRadSRS_2 = 8,
};

enum PRadBankID
{
    TI_BANK = 0xe10a,
    TAG_BANK = 0xe10b,
    GEM_BANK = 0xe11f,
    FASTBUS_BANK = 0xe120,
    TDC_BANK = 0xe121,
    DSC_BANK = 0xe122,
    EVINFO_BANK = 0xc000,
};


struct ChannelAddress
{
    size_t crate;
    size_t slot;
    size_t channel;

    bool operator < (const ChannelAddress &rhs) const {
        if( crate != rhs.crate )
            return crate < rhs.crate ;
        else if( slot != rhs.slot )
            return slot < rhs.slot ;
        else if( channel != rhs.channel )
            return channel < rhs.channel ;
        else
        return false ;
    }

    bool operator == (const ChannelAddress &rhs) const {
        if( (crate != rhs.crate) ||
            (slot != rhs.slot)   ||
            (channel != rhs.channel) )
            return false;
        else
            return true;
    }

};


// some words defined in readout list
#define ADC1881M_DATABEG 0xdc0adc00 //&0xff0fff00
#define ADC1881M_DATAEND 0xfabc0005

#define V767_HEADER_BIT  (1 << 22)
#define V767_END_BIT     (1 << 21)
#define V767_INVALID_BIT (V767_HEADER_BIT | V767_END_BIT)

// v1190 type check (data >> 27)
enum V1190WordType {
  V1190_GLOBAL_HEADER = 0x08,  // 01000
  V1190_GLOBAL_TRAILER = 0x10, // 10000
  V1190_GLOBAL_TIMETAG = 0x11, // 10001
  V1190_TDC_HEADER = 0x01,     // 00001
  V1190_TDC_TRAILER = 0x03,    // 00011
  V1190_TDC_ERROR = 0x04,      // 00100
  V1190_TDC_MEASURE = 0x00,    // 00000
  V1190_FILLER_WORD = 0x18,    // 11000
};

/* 32 bit event header structure
 * -------------------
 * |     length      |
 * -------------------
 * |  tag  |type| num|
 * -------------------
 */
struct PRadEventHeader
{
    unsigned int length;
    unsigned char num;
    unsigned char type;
    unsigned short tag;
};

struct JLabTIData
{
    unsigned int time_gated;
    unsigned int time_ungated;
    unsigned char trigger_type;
    unsigned char lms_phase;
};

struct ADC1881MData
{
    ChannelAddress config;
    unsigned short val;
};

struct TDCV767Data
{
    ChannelAddress config;
    unsigned int val;
};

struct TDCV1190Data
{
    ChannelAddress config;
    unsigned int val;
};

struct GEMAPVData
{
    unsigned char FEC;
    unsigned char APV;
    struct {
        unsigned short first;
        unsigned short second;
    } val;
};

struct CAENHVData
{
    ChannelAddress config;
    std::string name;
    bool ON;
    float Vmon;
    float Vset;
};

#endif
