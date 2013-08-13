/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <string>
#include <sstream>
#include "Eclipse.hpp"
#include "EclipseReader.hpp"
#include "Logger.hpp"

int
main( int argc, char** argv )
{
    Logger log = getLogger( "main" );
    bool contents = false;
    initializeLoggingFramework( &argc, argv );
    for( int i=1; i<argc; i++ ) {
        std::string file( argv[i] );
        if( file == "--contents" ) {
            contents = true;
            continue;
        }
        LOGGER_INFO( log, "--- Inspecting '" << file << "' --- " );
        Eclipse::Reader reader( file );
        std::list<Eclipse::Block> blocks = reader.blocks();
        for( auto it=blocks.begin(); it!=blocks.end(); ++it ) {
            const Eclipse::Block& block = *it;

            LOGGER_DEBUG( log,
                          "Block keyword=" << std::string( block.m_keyword_string, block.m_keyword_string+8 ) <<
                          " (" << Eclipse::keywordString( block.m_keyword ) << ") " <<
                          ", type=" << Eclipse::typeString(block.m_datatype) <<
                          ", count=" << block.m_count <<
                          ", offset=" << block.m_offset );
            if( contents ) {
                if( block.m_datatype == Eclipse::TYPE_BOOL ) {
                    std::vector<bool> contents;
                    reader.blockContent( contents, block );
                    for( size_t i=0; i<contents.size(); i++) {
                        LOGGER_DEBUG( log, "  " << i << ": " << contents[i] );
                    }
                }
                else if( block.m_datatype == Eclipse::TYPE_INTEGER ) {
                    std::vector<int> contents;
                    reader.blockContent( contents, block );
                    for( size_t i=0; i<contents.size(); i++) {
                        LOGGER_DEBUG( log, "  " << i << ": " << contents[i] );
                    }
                }
                else if( block.m_datatype == Eclipse::TYPE_FLOAT ) {
                    std::vector<float> contents;
                    reader.blockContent( contents, block );
                    for( size_t i=0; i<contents.size(); i++) {
                        LOGGER_DEBUG( log, "  " << i << ": " << contents[i] );
                    }
                }
                else if( block.m_datatype == Eclipse::TYPE_DOUBLE ) {
                    std::vector<double> contents;
                    reader.blockContent( contents, block );
                    for( size_t i=0; i<contents.size(); i++) {
                        LOGGER_DEBUG( log, "  " << i << ": " << contents[i] );
                    }
                }
                else if( block.m_datatype == Eclipse::TYPE_STRING ) {
                    std::vector<std::string> contents;
                    reader.blockContent( contents, block );
                    for( size_t i=0; i<contents.size(); i++) {
                        LOGGER_DEBUG( log, "  " << i << ": " << contents[i] );
                    }
                }
            }
        }
    }
}
