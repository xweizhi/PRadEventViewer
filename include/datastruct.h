#ifndef PRAD_DATA_STRUCT_H
#define PRAD_DATA_STRUCT_H

#include <string>

enum PRadEventType
{
    PhysicsType1 = 0x01,
    PhysicsType2 = 0x02,
    PreStartEvent = 0x11,
    GoEvent = 0x12,
    EndEvent = 0x20,
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
    FASTBUS_BANK = 0x7,
    GEMDATA_BANK = 0x8,
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
