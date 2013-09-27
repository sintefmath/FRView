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

#pragma once
#include <GL/glew.h>

// Define blank macros if we aren't debugging.
// These will be identical in log4cxx and std::cout
#ifndef DEBUG
// Detailed logging removed for release builds
#define LOGGER_TRACE(a,b)
#define LOGGER_INFO(a,b)
#define LOGGER_DEBUG(a,b)
#endif


#ifdef FRVIEW_HAS_LOG4CXX
#include <log4cxx/logger.h>

typedef log4cxx::LoggerPtr Logger;

static inline Logger getLogger( const std::string name )
{
    return log4cxx::Logger::getLogger( name );
}

#ifdef DEBUG
// Detailed logging only enabled for debug builds
#define LOGGER_TRACE(a,b)   LOG4CXX_TRACE(a,b)
#define LOGGER_INFO(a,b)    LOG4CXX_INFO(a,b)
#define LOGGER_DEBUG(a,b)   LOG4CXX_DEBUG(a,b)
#endif

// Warnings and up are always logged
#define LOGGER_WARN(a,b)    LOG4CXX_WARN(a,b)
#define LOGGER_ERROR(a,b)   LOG4CXX_ERROR(a,b)
#define LOGGER_FATAL(a,b)   LOG4CXX_FATAL(a,b)



#else // ifdef FRVIEW_HAS_LOG4CXX
#include <iostream>
#include <string>

typedef std::string Logger;
static inline Logger getLogger(const std::string name)
{
    return name;
}

#ifdef DEBUG
// Detailed logging only enabled for debug builds
#define LOGGER_TRACE(a,b)   std::cout << "TRACE(" << a << "): " << b << std::endl;
#define LOGGER_INFO(a,b)    std::cout << "INFO(" << a << "): " << b << std::endl;
#define LOGGER_DEBUG(a,b)   std::cout << "DEBUG("<< a << "): " << b << std::endl;
#endif

// Warnings and up are always logged
#define LOGGER_WARN(a,b)    std::cerr << "WARN(" << a << "): " << b << std::endl;
#define LOGGER_ERROR(a,b)   std::cerr << "ERROR(" << a << "): " << b << std::endl;
#define LOGGER_FATAL(a,b)   std::cerr << "FATAL(" << a << "): " << b << std::endl;
#endif // ifdef FRVIEW_HAS_LOG4CXX

// Check invariants are logger agnostic
#ifdef CHECK_INVARIANTS
#define LOGGER_INVARIANT(a,b) if(!(b)) { \
    LOGGER_FATAL( a, "Invariant (" << #b << ") broken at" << __FILE__ << '@' << __LINE__ ); \
}

#define LOGGER_INVARIANT_EQUAL(a,b,c) if(!((b) == (c))) { \
    LOGGER_FATAL( a, "Invariant (" << #b << "=" << (b) << ") == (" << #c << "=" << (c) << ") broken at" << __FILE__ << '@' << __LINE__ ); \
}
#define LOGGER_INVARIANT_NOT_EQUAL(a,b,c) if(!((b) != (c))) { \
    LOGGER_FATAL( a, "Invariant (" << #b << "=" << (b) << ") != (" << #c << "=" << (c) << ") broken at" << __FILE__ << '@' << __LINE__ ); \
}
#define LOGGER_INVARIANT_WITHIN_CLOSED_RANGE(a,b,c,d) if( ((b)<(c))||((d)<(b)) ) { \
    LOGGER_FATAL( a, "Invariant (" << #b << " in [" << #c << ", "<< #d << "] broken at " << __FILE__ << '@' << __LINE__ ); \
}
#define LOGGER_INVARIANT_EITHER_EQUAL(a,b,c,d,e) if(!( ((b) == (c)) || ((d) == (e))  ) ) { \
    LOGGER_FATAL( a, "Invariant (" << #b << "=" << (b) << ") == (" << #c << "=" << (c) << ") || (" << #d << "=" << (d) << ") == (" << #e << "=" << (e) << ") broken at" << __FILE__ << '@' << __LINE__ ); \
}
#define LOGGER_INVARIANT_EITHER_NOT_EQUAL(a,b,c,d,e) if(!( ((b) != (c)) || ((d) != (e))  ) ) { \
    LOGGER_FATAL( a, "Invariant (" << #b << "=" << (b) << ") != (" << #c << "=" << (c) << ") || (" << #d << "=" << (d) << ") != (" << #e << "=" << (e) << ") broken at" << __FILE__ << '@' << __LINE__ ); \
}
#define LOGGER_INVARIANT_LESS(a,b,c) if(!((b) < (c))) { \
    LOGGER_FATAL( a, "Invariant (" << #b << "=" << (b) << ") < (" << #c << "=" << (c) << ") broken at" << __FILE__ << '@' << __LINE__ ); \
}
#define LOGGER_INVARIANT_LESS_EQUAL(a,b,c) if(!((b) <= (c))) { \
    LOGGER_FATAL( a, "Invariant (" << #b << "=" << (b) << ") <= (" << #c << "=" << (c) << ") broken at" << __FILE__ << '@' << __LINE__ ); \
}
#else
#define LOGGER_INVARIANT(a,b)
#define LOGGER_INVARIANT_EQUAL(a,b,c)
#define LOGGER_INVARIANT_NOT_EQUAL(a,b,c)
#define LOGGER_INVARIANT_WITHIN_CLOSED_RANGE(a,b,c,d)
#define LOGGER_INVARIANT_EITHER_EQUAL(a,b,c,d,e)
#define LOGGER_INVARIANT_EITHER_NOT_EQUAL(a,b,c,d,e)
#define LOGGER_INVARIANT_LESS(a,b,c)
#define LOGGER_INVARIANT_LESS_EQUAL(a,b,c)
#endif

void
initializeLoggingFramework( int* argc, char** argv );
