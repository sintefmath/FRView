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
#include <tinia/qtcontroller/QTController.hpp>

#include "job/CPViewJob.hpp"
#include "utils/Logger.hpp"

namespace resources {
    extern std::string cameramanipulator;
}

int
main( int argc, char** argv )
{
    initializeLoggingFramework( &argc, argv );
    std::list<std::string> files;
    for(int i=1; i<argc; i++ ) {
        files.push_back( argv[i] );
    }

    CPViewJob job( files );
    tinia::qtcontroller::QTController controller;
    controller.setJob( &job );
    controller.addScript( resources::cameramanipulator );
    controller.run( argc, argv );
    return 0;
}
