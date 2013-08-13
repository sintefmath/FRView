#pragma once
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
#else
// Detailed logging removed for release builds
#define LOGGER_TRACE(a,b)
#define LOGGER_INFO(a,b)
#define LOGGER_DEBUG(a,b)
#endif
// Warnings and up are always logged
#define LOGGER_WARN(a,b)    LOG4CXX_WARN(a,b)
#define LOGGER_ERROR(a,b)   LOG4CXX_ERROR(a,b)
#define LOGGER_FATAL(a,b)   LOG4CXX_FATAL(a,b)

#ifdef CHECK_INVARIANTS
#define LOGGER_INVARIANT(a,b) if(!(b)) { \
    LOGGER_FATAL( a, "Invariant (" << #b << ") broken at" << __FILE__ << '@' << __LINE__ ); \
}

#define LOGGER_INVARIANT_EQUAL(a,b,c) if(!((b) == (c))) { \
    LOGGER_FATAL( a, "Invariant (" << #b << "=" << (b) << ") == (" << #c << "=" << (c) << ") broken at" << __FILE__ << '@' << __LINE__ ); \
}
#define LOGGER_INVARIANT_EITHER_EQUAL(a,b,c,d,e) if(!( ((b) == (c)) || ((d) == (e))  ) ) { \
    LOGGER_FATAL( a, "Invariant (" << #b << "=" << (b) << ") == (" << #c << "=" << (c) << ") || (" << #d << "=" << (d) << ") == (" << #e << "=" << (e) << ") broken at" << __FILE__ << '@' << __LINE__ ); \
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
#define LOGGER_INVARIANT_EITHER_EQUAL(a,b,c,d,e)
#define LOGGER_INVARIANT_LESS(a,b,c)
#define LOGGER_INVARIANT_LESS_EQUAL(a,b,c)
#endif



void
initializeLoggingFramework( int* argc, char** argv );
