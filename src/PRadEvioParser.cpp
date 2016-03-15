//============================================================================//
// A class to parse data buffer from file or ET                               //
// Make sure the endianness is correct or change the code befire using it     //
// Multi-thread can be disabled in PRadDataHandler.h                          //
//                                                                            //
// Chao Peng                                                                  //
// 02/27/2016                                                                 //
//============================================================================//

#include "PRadEvioParser.h"
#include "PRadDataHandler.h"
#include <thread>
#include <iostream>
#include <iomanip>

#define HEADER_SIZE 2

using namespace std;

PRadEvioParser::PRadEvioParser(PRadDataHandler *handler) : myHandler(handler) {}

void PRadEvioParser::parseEventByHeader(PRadEventHeader *header)
{
    // first check event type
    switch(header->tag)
    {
    case PhysicsType1:
    case PhysicsType2:
        break; // go on to process
    case PreStartEvent:
    case GoEvent:
    case EndEvent:
    default:
        return; // not interested event type
    }

    const uint32_t *buffer = (const uint32_t*) header;
    size_t evtSize = header->length;
    size_t dataSize = 0;
    size_t index = 0;
#ifdef MULTI_THREAD // definition in PRadDataHandler.h
    vector<thread> bank_threads;
#endif
    index += HEADER_SIZE; // skip event header

    while(index < evtSize)
    {
        PRadEventHeader* evtHeader = (PRadEventHeader*) &buffer[index];
        dataSize = evtHeader->length - 1;
        index += HEADER_SIZE; // header info is read

        // check the header, skip uninterested ones
        switch(evtHeader->type)
        {
        case EvioBank: // Bank type header for ROC
            switch(evtHeader->tag)
            {
            case PRadROC_6: // PRIMEXROC6
            case PRadROC_5: // PRIMEXROC5
            case PRadROC_4: // PRIMEXROC4
                continue; // Interested in ROC 4~6, to next header

            case PRadTS: // Not interested in ROC 1, PRIMEXTS2
            default: // unrecognized ROC
                // Skip the whole segment
                break;
            }
            break;

        case UnsignedInt32bit: // uint32 data bank
            switch(evtHeader->tag)
            {
            default:
                break;
            case EVINFO_BANK: // Bank contains the event information
                eventNb = buffer[index];
                break;
            case TI_BANK: // Bank 0x4, TI data, not interested
                // skip the whole segment
                break;
            case FASTBUS_BANK: // Bank 0x7, Fastbus data
                // Self defined crate data header
                if((buffer[index]&0xff0fff00) == ADC1881M_DATABEG) {
#ifdef MULTI_THREAD
                    bank_threads.push_back(thread(&PRadEvioParser::parseADC1881M, this, &buffer[index]));
#else
                    parseADC1881M(&buffer[index]);
#endif
                } else {
                    cerr << "Incorrect Fastbus bank header!" << endl;
                }
                break;
            case GEMDATA_BANK: // Bank 0x8, gem data, single FEC right now
                parseGEMData(&buffer[index], dataSize);
                break;
            }
            break;

        default:
            // Unknown header
            break;
        }

        index += dataSize; // Data are either processed or skipped above
    }

#ifdef MULTI_THREAD
    // wait for all threads finished
    for(size_t i = 0; i < bank_threads.size(); ++i)
    {
        if(bank_threads[i].joinable()) bank_threads[i].join();
    }
#endif

    myHandler->EndofThisEvent(); // inform handler the end of event
}

void PRadEvioParser::parseADC1881M(const uint32_t *data)
{
    // number of boards given by the self defined info word in CODA readout list
    const unsigned char boardNum = data[0]&0xFF;
    unsigned int index = 1, wordCount;
    ADC1881MData adcData;

    adcData.config.crate = (data[0]>>20)&0xF;

    // parse the data for all boards
    for(unsigned char i = 0; i < boardNum; ++i)
    {
        if(data[index] == ADC1881M_DATAEND) // self defined, end of crate word
            break;
        adcData.config.slot = (data[index]>>27)&0x1F;
        wordCount = (data[index]&0x7F) + index;
        while(++index < wordCount)
        {
            if(((data[index]>>27)&0x1F) == (unsigned int)adcData.config.slot) {
                adcData.config.channel = (data[index]>>17)&0x3F;
                adcData.val = data[index]&0x1FFF;
                myHandler->FeedData(adcData); // feed data to handler
            } else { // show the error message
                cerr << "*** MISMATCHED CRATE ADDRESS ***" << endl;
                cerr << "GEOGRAPHICAL ADDRESS = "
                     << "0x" << hex << setw(8) << setfill('0') // formating
                     << adcData.config.slot
                     << endl;
                cerr << "BOARD ADDRESS = "
                     << "0x" << hex << setw(8) << setfill('0')
                     << ((data[index]&0xf8000000)>>27)
                     << endl;
                cerr << "DATA WORD = "
                     << "0x" << hex << setw(8) << setfill('0')
                     << data[index]
                     << endl;
            }
        }
    }

}

// temporary decoder for gem data, not finished
void PRadEvioParser::parseGEMData(const uint32_t *data, unsigned int size)
{
#define GEMDATA_APVBEG 0x00434441 //&0x00ffffff
#define GEMDATA_SECTION_BEG 0x00002000
#define GEMDATA_SECTION_END 0xc0da0100
#define GEMDATA_FECEND 0xfafafafa
// gem data structure
// single FEC
// 3 words header
// 0xff|ffffff
// apv |  frame counter, same in one event
// 0xff|434441
// apv |  fix number
// 0xffffffff
// unknown
// 0xffff|ffff
// data  |  data

    GEMAPVData gemData;
    gemData.FEC = 0;

    for(unsigned int i = 0; i < size; ++i)
    {
        if((data[i]&0xffffff) == GEMDATA_APVBEG) {
            gemData.APV = (data[i]>>24)&0xff;
            i += 2;
            for(; (data[i]&0xffffff) != data[0] && data[i] != GEMDATA_FECEND; ++i) {
                if((i+8) < size && data[i] == GEMDATA_SECTION_BEG && data[i+7] == GEMDATA_SECTION_END) {
                    i += 7; // strip off section words
                    continue;
                }
                gemData.val.first = data[i]&0xffff;
                gemData.val.second = (data[i]>>16)&0xffff;
                myHandler->FeedData(gemData); // two adc values are sent in one package to reduce the number of function calls
            }
            // APV ends
        }
    }
}
