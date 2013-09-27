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
        eclipse::Reader reader( file );
        std::list<eclipse::Block> blocks = reader.blocks();
        for( auto it=blocks.begin(); it!=blocks.end(); ++it ) {
            const eclipse::Block& block = *it;

            LOGGER_DEBUG( log,
                          "Block keyword=" << std::string( block.m_keyword_string, block.m_keyword_string+8 ) <<
                          " (" << eclipse::keywordString( block.m_keyword ) << ") " <<
                          ", type=" << eclipse::typeString(block.m_datatype) <<
                          ", count=" << block.m_count <<
                          ", offset=" << block.m_offset );
            if( contents ) {
                if( block.m_datatype == eclipse::TYPE_BOOL ) {
                    std::vector<bool> contents;
                    reader.blockContent( contents, block );
                    for( size_t i=0; i<contents.size(); i++) {
                        LOGGER_DEBUG( log, "  " << i << ": " << contents[i] );
                    }
                }
                else if( block.m_datatype == eclipse::TYPE_INTEGER ) {
                    std::vector<int> contents;
                    reader.blockContent( contents, block );
                    for( size_t i=0; i<contents.size(); i++) {
                        LOGGER_DEBUG( log, "  " << i << ": " << contents[i] );
                    }
                }
                else if( block.m_datatype == eclipse::TYPE_FLOAT ) {
                    std::vector<float> contents;
                    reader.blockContent( contents, block );
                    for( size_t i=0; i<contents.size(); i++) {
                        LOGGER_DEBUG( log, "  " << i << ": " << contents[i] );
                    }
                }
                else if( block.m_datatype == eclipse::TYPE_DOUBLE ) {
                    std::vector<double> contents;
                    reader.blockContent( contents, block );
                    for( size_t i=0; i<contents.size(); i++) {
                        LOGGER_DEBUG( log, "  " << i << ": " << contents[i] );
                    }
                }
                else if( block.m_datatype == eclipse::TYPE_STRING ) {
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
