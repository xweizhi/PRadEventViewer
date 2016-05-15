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
#include <fstream>

#define HEADER_SIZE 2

using namespace std;

PRadEvioParser::PRadEvioParser(PRadDataHandler *handler) : myHandler(handler)
{}

void PRadEvioParser::parseEventByHeader(PRadEventHeader *header)
{
    // first check event type
    switch(header->tag)
    {
    case CODA_Event:
        break; // go on to process
    case CODA_Prestart:
    case CODA_Go:
    case CODA_End:
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
        case EvioBank_B:
            switch(evtHeader->tag)
            {
            case PRadTagE:  // Tagger E, ROC id 7
            case PRadSRS_2: // SRS, ROC id 9
            case PRadSRS_1: // SRS, ROC id 8
            case PRadROC_3: // Fastbus, ROC id 6
            case PRadROC_2: // Fastbus, ROC id 5
            case PRadROC_1: // Fastbus, ROC id 4
            case PRadTS: // VME, ROC id 2
                continue; // Interested in ROCs, to next header

            default: // unrecognized ROC
                // Skip the whole segment
                break;
            }
            break;

        case UnsignedInt_32bit: // uint32 data bank
            switch(evtHeader->tag)
            {
            default:
                break;
            case EVINFO_BANK: // Bank contains the event information
                eventNb = buffer[index];
                break;
            case TI_BANK: // Bank 0x4, TI data, contains live time and event type information
                parseTIData(&buffer[index], dataSize, evtHeader->num);
                break;
            case TDC_BANK:
                parseTDCData(&buffer[index], dataSize);
                break;
            case DSC_BANK:
                parseDSCData(&buffer[index]);
                break;
            case FASTBUS_BANK: // Bank 0x7, Fastbus data
                // Self defined crate data header
                if((buffer[index]&0xff0fff00) == ADC1881M_DATABEG) {
#ifdef MULTI_THREAD
                    // for LMS event, since every module is fired, each thread needs to modify the non-local
                    // variable E_total and the container for current event. Which means very frequent actions
                    // on mutex lock and unlock, it indeed undermine the performance
                    // TODO, separate thread for GEM and HyCal only, there won't be any shared object between
                    // these two sub-system
                    bank_threads.push_back(thread(&PRadEvioParser::parseADC1881M, this, &buffer[index]));
#else
                    parseADC1881M(&buffer[index]);
#endif
                } else {
                    cerr << "Incorrect Fastbus bank header!"
                         << "0x" << hex << setw(8) << setfill('0')
                         <<  buffer[index] << endl;
                }
                break;
            case GEM_BANK: // Bank 0x8, gem data, single FEC right now
                parseGEMData(&buffer[index], dataSize, evtHeader->num);
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
    for(auto &thread : bank_threads)
    {
        if(thread.joinable()) thread.join();
    }
#endif

    myHandler->EndofThisEvent(); // inform handler the end of event
}

void PRadEvioParser::parseADC1881M(const uint32_t *data)
{
    // number of boards given by the self defined info word in CODA readout list
    const unsigned char boardNum = data[0]&0xFF;
    unsigned int index = 2, wordCount;
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
void PRadEvioParser::parseGEMData(const uint32_t *data, const size_t &size, const int &fec_id)
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
    gemData.FEC = fec_id;

    for(size_t i = 0; i < size; ++i)
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

void PRadEvioParser::parseTDCData(const uint32_t * data, const size_t &size)
{
    // place holder
    TDCV767Data tdcData;
    tdcData.config.crate = 2;
    tdcData.config.slot = 0;

    if(!(data[0]&V767_HEADER_BIT)) {
        cerr << "Unrecognized V767 header word: "
             << "0x" << hex << setw(8) << setfill('0') << data[0]
             << endl;
        return;
    }
    if(!(data[size-1]&V767_END_BIT)) {
        cerr << "Unrecognized V767 EOB word: "
             << "0x" << hex << setw(8) << setfill('0') << data[size-1]
             << endl;
        return;
    }

    for(size_t i = 1; i < size - 1; ++i)
    {
        if(data[i]&V767_INVALID_BIT) {
            cerr << "Event: "<< dec << eventNb
                 << ", invalid data word: "
                 << "0x" << hex << setw(8) << setfill('0') << data[i]
                 << endl;
            continue;
        }
        tdcData.config.channel = (data[i]>>24)&0x7f;
        tdcData.val = data[i]&0xfffff;
        myHandler->FeedData(tdcData);
    }
}

void PRadEvioParser::parseDSCData(const uint32_t * /*data*/)
{
    // place holder
}

void PRadEvioParser::parseTIData(const uint32_t *data, const size_t &size, const int &roc_id)
{
    // not interested in TI-slaves
    if(roc_id != PRadTS)
        return;
    if(size < 9) {
        cerr << "Too small data size for TI bank, expecting 9, receiving " << size << endl;
        return;
    }
        
    JLabTIData tiData;
    tiData.trigger_type = (data[3]&0xff);
    tiData.lms_phase = ((data[8]&0xff0000)>>20);
    myHandler->FeedData(tiData);
}
