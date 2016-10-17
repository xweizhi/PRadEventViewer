//============================================================================//
// An class used to cut off bad events for PRad                               //
// Now it reads a list of bad events, expecting more criterion implemented    //
//                                                                            //
// Maxime Levillain, Chao Peng                                                //
// 10/17/2016                                                                 //
//============================================================================//

#include "PRadEventFilter.h"
#include "ConfigParser.h"

PRadEventFilter::PRadEventFilter()
{
    // place holder
}

PRadEventFilter::~PRadEventFilter()
{
    // place holder
}

void PRadEventFilter::LoadBadEventList(const std::string &path)
{
    ConfigParser c_parser;

    // remove *, space and tab at both ends of each element
    c_parser.SetWhiteSpace(" \t*");

    if (!c_parser.OpenFile(path)) {
        std::cerr << "PRad Event Filter Error: Cannot open bad event list "
                  <<"\"" << path << "\""
                  << std::endl;
      return;
    }

    bad_events_list.clear();
    int val1, val2;

    while(c_parser.ParseLine())
    {
        if(c_parser.NbofElements() != 2)
            continue;

        c_parser >> val1 >> val2;
        bad_events_list.emplace_back(val1, val2);
    }

    c_parser.CloseFile();
}

bool PRadEventFilter::IsBadEvent(const EventData &event)
{
    bool bad = false;

    for(auto &interval : bad_events_list)
    {
        // in the bad event list
        if((event.event_number >= interval.begin) &&
           (event.event_number <= interval.end))
        {
            bad = true;
            break;
        }
    }

    return bad;
}
