#ifndef PRAD_EVENT_FILTER
#define PRAD_EVENT_FILTER

#include <vector>
#include <string>
#include "PRadEventStruct.h"

class PRadEventFilter
{
private:
    struct ev_interval
    {
        int begin;
        int end;

        ev_interval() {};
        ev_interval(int b, int e) : begin(b), end(e) {};
    };

public:
    PRadEventFilter();
    virtual ~PRadEventFilter();
    void LoadBadEventList(const std::string &path);
    bool IsBadEvent(const EventData &event);

private:
    std::vector<ev_interval> bad_events_list;
};

#endif
