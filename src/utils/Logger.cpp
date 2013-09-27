/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <string>
#include <vector>
#ifdef FRVIEW_HAS_LOG4CXX
#include <log4cxx/logger.h>
#include <log4cxx/helpers/properties.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#endif
#include "Logger.hpp"

#ifdef FRVIEW_HAS_LOG4CXX
using std::string;
using std::vector;

enum LogLevel {
    LEVEL_FATAL,
    LEVEL_ERROR,
    LEVEL_WARN,
    LEVEL_INFO,
    LEVEL_DEBUG,
    LEVEL_TRACE
};

void
initializeLoggingFramework( int* argc, char** argv )
{
    log4cxx::helpers::Properties props;
    props.setProperty( LOG4CXX_STR( "log4j.appender.A1" ), LOG4CXX_STR( "org.apache.log4j.ConsoleAppender" ) );
    props.setProperty( LOG4CXX_STR( "log4j.appender.A1.layout" ), LOG4CXX_STR( "org.apache.log4j.PatternLayout" ) );
    props.setProperty( LOG4CXX_STR( "log4j.appender.A1.layout.ConversionPattern" ), LOG4CXX_STR( "%-4r %-5p %40c - %m%n" ) );

    LogLevel level = LEVEL_DEBUG;
    string logconfig;


    vector<char*> new_argv;
    for( int i=0; i<*argc; i++) {
        string param( argv[i] );
        if( i==0 ) {
            // application name
            new_argv.push_back( argv[i] );
        }
        else if( param == "--loglevel" && (i+1) < *argc ) {
            string value( argv[i+1] );
            if( value == "fatal" ) {
                level = LEVEL_FATAL;
            }
            else if ( value == "error" ) {
                level = LEVEL_ERROR;
            }
            else if( value == "warn" ) {
                level = LEVEL_WARN;
            }
            else if( value == "info" ) {
                level = LEVEL_INFO;
            }
            else if( value == "debug" ) {
                level = LEVEL_DEBUG;
            }
            else if( value == "trace" ) {
                level = LEVEL_TRACE;
            }
            i++;
        }
        else if( param == "--logconfig" && (i+1) < *argc ) {
            logconfig = argv[i+1];
            i++;
        }
        else {
            new_argv.push_back( argv[i] );
        }
    }

    // remove known arguments
    *argc = new_argv.size();
    for(int i=0; i<*argc; i++ ) {
        argv[i] = new_argv[i];
    }

    log4cxx::PropertyConfigurator::configure( props );
    switch( level ) {
    case LEVEL_FATAL:
        props.setProperty( LOG4CXX_STR( "log4j.rootLogger" ), LOG4CXX_STR( "FATAL, A1" ) );
        break;
    case LEVEL_ERROR:
        props.setProperty( LOG4CXX_STR( "log4j.rootLogger" ), LOG4CXX_STR( "ERROR, A1" ) );
        break;
    case LEVEL_WARN:
        props.setProperty( LOG4CXX_STR( "log4j.rootLogger" ), LOG4CXX_STR( "WARN, A1" ) );
        break;
    case LEVEL_INFO:
        props.setProperty( LOG4CXX_STR( "log4j.rootLogger" ), LOG4CXX_STR( "INFO, A1" ) );
        break;
    case LEVEL_DEBUG:
        props.setProperty( LOG4CXX_STR( "log4j.rootLogger" ), LOG4CXX_STR( "DEBUG, A1" ) );
        break;
    case LEVEL_TRACE:
        props.setProperty( LOG4CXX_STR( "log4j.rootLogger" ), LOG4CXX_STR( "TRACE, A1" ) );
        break;
    }
    props.setProperty( LOG4CXX_STR( "log4j.appender.A1" ), LOG4CXX_STR( "org.apache.log4j.ConsoleAppender" ) );
    props.setProperty( LOG4CXX_STR( "log4j.appender.A1.layout" ), LOG4CXX_STR( "org.apache.log4j.PatternLayout" ) );
    props.setProperty( LOG4CXX_STR( "log4j.appender.A1.layout.ConversionPattern" ), LOG4CXX_STR( "%-4r %-5p %40c - %m%n" ) );
    log4cxx::PropertyConfigurator::configure( props );

    if( !logconfig.empty() ) {
        log4cxx::PropertyConfigurator::configure( logconfig );
    }
}
#else
void
initializeLoggingFramework( int*, char**) {
    // With standard std::cout-logging this doesn't need
    // to do anything.
}
#endif
