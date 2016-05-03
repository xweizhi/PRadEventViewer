#ifndef PRAD_DATA_STRUCT_H
#define PRAD_DATA_STRUCT_H

#include <string>

enum PRadEventType
{
    LMS_Led = 0x01,
    LMS_Alpha = 0x02,
    PHYS_Pedestal = 0x03,
    PHYS_TotalSum = 0x04,
    CODA_Prestart = 0x11,
    CODA_Go = 0x12,
    CODA_End = 0x20,
};

enum PRadHeaderType
{
    UnsignedInt32bit = 0x01,
    EvioBank = 0x10
};

enum PRadROCID
{
// test data at early stage using different IDs for coda components
// PRadTS = 1,
// PRadROC_4 = 14,
// PRadROC_5 = 12,
// PRadROC_6 = 11,
    PRadTS = 2,
    PRadROC_4 = 4,
    PRadROC_5 = 5,
    PRadROC_6 = 6,
};

enum PRadBankID
{
    TI_BANK = 0x4,
    TDC_BANK = 0x5,
    DSC_BANK = 0x6,
    FASTBUS_BANK = 0x7,
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
#define ADC1881M_DATABEG 0xdc0adc00 //&0xff0fff00
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

struct ADC1881MData
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
