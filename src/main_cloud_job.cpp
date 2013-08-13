#include <list>
#include <string>
#include <tinia/trell/IPCGLJobController.hpp>
#include "Logger.hpp"
#include "CPViewJob.hpp"

int
main( int argc, char** argv )
{
    bool is_master = (argc > 2) && (strcmp( argv[1], argv[2] ) == 0 );
    if( is_master ){
        std::cerr << "Is master.\n";
    }
    std::list<std::string> files;
    files.push_back( "/work/cpgrids/misc/res2/BC0407.EGRID" );
    files.push_back( "/work/cpgrids/misc/res2/BC0407.UNRST" );

    initializeLoggingFramework( &argc, argv );
    CPViewJob* job = new CPViewJob( files );
    tinia::trell::IPCGLJobController* observer = new tinia::trell::IPCGLJobController( is_master );
    observer->setJob( job );
    observer->run( argc, argv );
    exit( EXIT_SUCCESS );
}
