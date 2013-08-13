#pragma once
#include <time.h>
#include <boost/utility.hpp>

class PerfTimer : public boost::noncopyable
{
public:
    PerfTimer();

    static
    double
    delta( const PerfTimer& start, const PerfTimer& stop );

private:
    timespec    m_time;
};

