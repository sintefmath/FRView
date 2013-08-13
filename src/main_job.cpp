/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <GL/glew.h>
#include <string>
#include <list>
#include <qtobserver/QTObserver.hpp>

#include "CPViewJob.hpp"
#include "Logger.hpp"

int
main( int argc, char** argv )
{
    initializeLoggingFramework( &argc, argv );
    std::list<std::string> files;
    for(int i=1; i<argc; i++ ) {
        files.push_back( argv[i] );
    }
    CPViewJob *job = new CPViewJob( files );
    qtobserver::QTObserver *qtObserver = new qtobserver::QTObserver();
    qtObserver->setJob(job);
    qtObserver->run(argc, argv);
}
