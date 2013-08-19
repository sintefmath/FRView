#include <list>
#include <string>
#include <tinia/trell/IPCGLJobController.hpp>
#include "utils/Logger.hpp"
#include "job/CPViewJob.hpp"

namespace resources {
    extern std::string cameramanipulator;
}

int
main( int argc, char** argv )
{
    bool is_master = (argc > 2) && (strcmp( argv[1], argv[2] ) == 0 );
    if( is_master ){
        std::cerr << "Is master.\n";
    }
    initializeLoggingFramework( &argc, argv );

    std::list<std::string> files;

    CPViewJob job( files );
    tinia::trell::IPCGLJobController controller( is_master );
    controller.setJob( &job );
    controller.addScript( resources::cameramanipulator );
    controller.run( argc, argv );
    return 0;
}
