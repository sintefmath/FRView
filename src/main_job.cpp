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

#include <GL/glew.h>
#include <string>
#include <clocale>
#include <list>
#include <tinia/qtcontroller/QTController.hpp>

#include "job/FRViewJob.hpp"
#include "utils/Logger.hpp"

namespace resources {
    extern std::string cameramanipulator;
}

int
main( int argc, char** argv )
{
    // Make sure that text-based data sources are parsed in a predictable
    // manner, in particular, let the decimal point be '.'.  As this function
    // is rather thread-unsafe, we call it here before any threads are created.
    std::setlocale( LC_ALL, "C" );

    initializeLoggingFramework( &argc, argv );
    std::list<std::string> files;
    for(int i=1; i<argc; i++ ) {
        files.push_back( argv[i] );
    }

    FRViewJob job( files );
    tinia::qtcontroller::QTController controller;
    controller.setJob( &job );
    //controller.addScript( resources::cameramanipulator );
    controller.run( argc, argv );
    return 0;
}
