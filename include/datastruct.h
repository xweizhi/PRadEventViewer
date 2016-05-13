#ifndef PRAD_DATA_STRUCT_H
#define PRAD_DATA_STRUCT_H

#include <string>

enum PRadEventType
{
    Unknown = 0x0,
    PHYS_Pedestal = 0x81,
    PHYS_TotalSum = 0x82,
    LMS_Led = 0x83,
    LMS_Alpha = 0x84,
    BEAM_Tagger = 0x85,
    CODA_Prestart = 0x11,
    CODA_Go = 0x12,
    CODA_End = 0x20,
};

enum PRadHeaderType
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
// test data at early stage using different IDs for coda components
// PRadTS = 1,
// PRadROC_4 = 14,
// PRadROC_5 = 12,
// PRadROC_6 = 11,
    PRadTS = 1,
    PRadROC_1 = 4,
    PRadROC_2 = 5,
    PRadROC_3 = 6,
};

enum PRadBankID
{
    TI_BANK = 0xe10a,
    TDC_BANK = 0xe121,
    DSC_BANK = 0x6,
    FASTBUS_BANK = 0xe120,
    GEM_FEC1_BANK = 0x8,
    GEM_FEC2_BANK = 0x9,
    GEM_FEC3_BANK = 0xA,
    GEM_FEC4_BANK = 0xB,
    GEM_FEC5_BANK = 0xC,
    GEM_FEC6_BANK = 0xD,
    GEM_FEC7_BANK = 0xE,
    GEM_FEC8_BANK = 0xF,
    EVINFO_BANK = 0xC000,
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
//#define ADC1881M_DATABEG 0xdc0adc00 //&0xff0fff00
#define ADC1881M_DATABEG 0
#define ADC1881M_DATAEND 0xfabc0005

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
    unsigned int trigger_type;
};

struct ADC1881MData
{
    ChannelAddress config;
    unsigned short val;
};

struct TDCV767Data
{
    ChannelAddress config;
    unsigned short val;
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
