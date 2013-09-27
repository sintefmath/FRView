/* Copyright STIFTELSEN SINTEF 2013
 * 
 * This file is part of FRView.
 * FRView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * FRView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *  
 * You should have received a copy of the GNU Affero General Public License
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PerfTimer.hpp"

PerfTimer::PerfTimer()
{
    reset();
}

void
PerfTimer::reset()
{
    clock_gettime( CLOCK_MONOTONIC, &m_time );
}


double
PerfTimer::delta( const PerfTimer& start, const PerfTimer& stop )
{
    timespec delta;
    if( (stop.m_time.tv_nsec-start.m_time.tv_nsec) < 0 ) {
        delta.tv_sec = stop.m_time.tv_sec - start.m_time.tv_sec - 1;
        delta.tv_nsec = 1000000000 + stop.m_time.tv_nsec - start.m_time.tv_nsec;
    }
    else {
        delta.tv_sec = stop.m_time.tv_sec - start.m_time.tv_sec;
        delta.tv_nsec = stop.m_time.tv_nsec - start.m_time.tv_nsec;
    }
    return delta.tv_sec + 1e-9*delta.tv_nsec;
}


