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

#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <libxml/parser.h>
#include "utils/Logger.hpp"
#include "dataset/VtkXmlParser.hpp"

namespace {

float dummy_vertices[3*4] = {
  0.f, 0.f, 0.f,
  1.f, 0.f, 0.f,
  0.f, 1.f, 0.f,
  0.f, 0.f, 1.f
};

int dummy_tetras[4*1] = {
    0, 1, 2, 3
};



struct Tag
{
    enum Type {
        TAG_UNKNOWN,
        TAG_VTKFILE,
        TAG_UNSTRUCTURED_GRID,
        TAG_PIECE,
        TAG_POINTS,
        TAG_POINT_DATA,
        TAG_CELLS,
        TAG_CELL_DATA,
        TAG_DATA_ARRAY,
        TAG_SENTINEL
    }   m_type;
    
    enum HandleCharacters {
        CHARACTER_IGNORE,
        CHARACTER_FLOAT_ARRAY,
        CHARACTER_INT_ARRAY
    }   m_handle_chars;
};


static const xmlChar* const tag_names[ Tag::TAG_SENTINEL+1 ] = 
{
    (const xmlChar*)"??Unknown",
    (const xmlChar*)"VTKFile",
    (const xmlChar*)"UnstructuredGrid",
    (const xmlChar*)"Piece",
    (const xmlChar*)"Points",
    (const xmlChar*)"PointData",
    (const xmlChar*)"Cells",
    (const xmlChar*)"CellData",
    (const xmlChar*)"DataArray",
    (const xmlChar*)"??Sentinel"
};


struct callback_data {
    bool                    m_success;
    Logger                  m_log;
   
    std::string             m_name;
    int                     m_components;
    
    std::vector<Tag>        m_stack;
    std::vector<char>       m_char_buffer;
    std::vector<float>      m_float_buffer;
    std::vector<int>        m_int_buffer;
};

void
start_element( void* user_data, const xmlChar* name, const xmlChar** attrs )
{
    callback_data* cbd = static_cast<callback_data*>( user_data );
    if( !cbd->m_success ) {
        return;
    }

    Tag::Type parent = cbd->m_stack.back().m_type;
    cbd->m_stack.resize( cbd->m_stack.size()+1 );   // add tag to stack
    cbd->m_stack.back().m_type = Tag::TAG_UNKNOWN;
    cbd->m_char_buffer.clear();

    // --- determine tag    
    if( parent != Tag::TAG_UNKNOWN ) {
        for( int i=1; i<Tag::TAG_SENTINEL; i++ ) {
            if( xmlStrEqual( name, tag_names[ i ] ) ) {
                cbd->m_stack.back().m_type = (Tag::Type)i;
                break;
            }
        }
    }
    
    // --- parse attributes and set up character handling
    switch ( cbd->m_stack.back().m_type ) {

    case Tag::TAG_UNKNOWN:
        break;

    case Tag::TAG_VTKFILE:
        if( attrs != NULL ) {
            for(int i=0; attrs[i] != NULL; i+=2 ) {
                LOGGER_WARN( cbd->m_log, "start_element: in " << tag_names[ cbd->m_stack.back().m_type ]
                        << ", ignoring attribute '" << attrs[i] << "' (=" << attrs[i+1] << ")." );
            }
        }
        break;

    case Tag::TAG_UNSTRUCTURED_GRID:
        if( attrs != NULL ) {
            for(int i=0; attrs[i] != NULL; i+=2 ) {
                LOGGER_WARN( cbd->m_log, "start_element: in " << tag_names[ cbd->m_stack.back().m_type ]
                        << ", ignoring attribute '" << attrs[i] << "' (=" << attrs[i+1] << ")." );
            }
        }
        break;
    
    case Tag::TAG_PIECE:
        for(int i=0; attrs[i] != NULL; i+=2 ) {
            if( xmlStrEqual( attrs[i], (const xmlChar*)"NumberOfPoints" ) ) {
                LOGGER_DEBUG( cbd->m_log, "start_element: " << tag_names[ cbd->m_stack.back().m_type ] << ": " << attrs[i] << "=" << attrs[i+1] );
            }
            else if( xmlStrEqual( attrs[i], (const xmlChar*)"NumberOfCells" ) ) {
                LOGGER_DEBUG( cbd->m_log, "start_element: " << tag_names[ cbd->m_stack.back().m_type ] << ": " << attrs[i] << "=" << attrs[i+1] );
            }
            else {
                LOGGER_WARN( cbd->m_log, "start_element: in " << tag_names[ cbd->m_stack.back().m_type ]
                        << ", ignoring attribute '" << attrs[i] << "' (=" << attrs[i+1] << ")." );
            }
        }
        break;

    case Tag::TAG_POINTS:
        if( attrs != NULL ) {
            for(int i=0; attrs[i] != NULL; i+=2 ) {
                if( xmlStrEqual( attrs[i], (const xmlChar*)"NumberOfComponents" ) ) {
                    LOGGER_DEBUG( cbd->m_log, "start_element: " << tag_names[ cbd->m_stack.back().m_type ] << ": " << attrs[i] << "=" << attrs[i+1] );
                }
                else {
                    LOGGER_WARN( cbd->m_log, "start_element: in " << tag_names[ cbd->m_stack.back().m_type ]
                            << ", ignoring attribute '" << attrs[i] << "' (=" << attrs[i+1] << ")." );
                }
            }
        }
        break;

    case Tag::TAG_POINT_DATA:
        if( attrs != NULL ) {
            for(int i=0; attrs[i] != NULL; i+=2 ) {
                LOGGER_WARN( cbd->m_log, "start_element: in " << tag_names[ cbd->m_stack.back().m_type ]
                        << ", ignoring attribute '" << attrs[i] << "' (=" << attrs[i+1] << ")." );
            }
        }
        break;
    
    case Tag::TAG_CELLS:
        if( attrs != NULL ) {
            for(int i=0; attrs[i] != NULL; i+=2 ) {
                LOGGER_WARN( cbd->m_log, "start_element: in " << tag_names[ cbd->m_stack.back().m_type ]
                        << ", ignoring attribute '" << attrs[i] << "' (=" << attrs[i+1] << ")." );
            }
        }
        break;

    case Tag::TAG_CELL_DATA:
        if( attrs != NULL ) {
            for(int i=0; attrs[i] != NULL; i+=2 ) {
                LOGGER_WARN( cbd->m_log, "start_element: in " << tag_names[ cbd->m_stack.back().m_type ]
                        << ", ignoring attribute '" << attrs[i] << "' (=" << attrs[i+1] << ")." );
            }
        }
        break;
        
    case Tag::TAG_DATA_ARRAY:
        if(1) {
        
            if( attrs != NULL ) {
                for(int i=0; attrs[i] != NULL; i+=2 ) {
                    if( xmlStrEqual( attrs[i], (const xmlChar*)"Type" ) ) {
                        if( xmlStrEqual( attrs[i+1], (const xmlChar*)"Int8" )
                                || xmlStrEqual( attrs[i+1], (const xmlChar*)"UInt8" )
                                || xmlStrEqual( attrs[i+1], (const xmlChar*)"Int16" )
                                || xmlStrEqual( attrs[i+1], (const xmlChar*)"UInt16" )
                                || xmlStrEqual( attrs[i+1], (const xmlChar*)"Int32" )
                                || xmlStrEqual( attrs[i+1], (const xmlChar*)"UInt32" )
                                || xmlStrEqual( attrs[i+1], (const xmlChar*)"Int64" )
                                || xmlStrEqual( attrs[i+1], (const xmlChar*)"UInt64" ) )
                        {
                            cbd->m_stack.back().m_handle_chars = Tag::CHARACTER_INT_ARRAY;
                        }
                        else if( xmlStrEqual( attrs[i+1], (const xmlChar*)"Float32" ) 
                                || xmlStrEqual( attrs[i+1], (const xmlChar*)"Float64" ) )
                        {
                            cbd->m_stack.back().m_handle_chars = Tag::CHARACTER_FLOAT_ARRAY;
                        }
                        else {
                           // error 
                        }
                    }
                    else if( xmlStrEqual( attrs[i], (const xmlChar*)"Name" ) ) {
                        cbd->m_name = (const char*)attrs[i+1];
                    }
                    else if( xmlStrEqual( attrs[i], (const xmlChar*)"NumberOfComponents" ) ) {
                        cbd->m_components = atoi( (const char*)attrs[i+1] );
                    }
                    else if( xmlStrEqual( attrs[i], (const xmlChar*)"format" ) ) {
//                        if( xmlStrEqual( attrs[i+1], "" ))
                        
                        //(( ascoo, binary, appended))
                        
                    }
                    else if( xmlStrEqual( attrs[i], (const xmlChar*)"offset" ) ) {  //(( ascoo, binary, appended))
                        
                    }
                }
            }

        }
        
        break;

    case Tag::TAG_SENTINEL:
        break;
    }
    
    LOGGER_DEBUG( cbd->m_log, "> " <<  tag_names[ cbd->m_stack.back().m_type ] << ",size=" << cbd->m_stack.size() << ", back=" << cbd->m_stack.back().m_type );
}

void
end_element( void* user_data, const xmlChar* name )
{
    callback_data* cbd = static_cast<callback_data*>( user_data );
    if( !cbd->m_success ) {
        return;
    }
    // make sure that we have a legal type
    if( cbd->m_stack.back().m_type >= Tag::TAG_SENTINEL ) {
        LOGGER_ERROR( cbd->m_log, "end_element: Illegal type on stack:" << cbd->m_stack.back().m_type );
        cbd->m_success = false;
        return;
    }
    // and unless unknown, check that tag matches
    if( cbd->m_stack.back().m_type != Tag::TAG_UNKNOWN ) {
        if( xmlStrEqual( name, tag_names[ cbd->m_stack.back().m_type ] ) == 0 ) {
            LOGGER_ERROR( cbd->m_log, "end_element: Expected " << tag_names[ cbd->m_stack.back().m_type ]
                                       << ", got " << name );
            cbd->m_success = false;
            return;
        }
    }

    switch ( cbd->m_stack.back().m_type ) {
    case Tag::TAG_UNKNOWN:
        break;

    case Tag::TAG_VTKFILE:
        break;

    case Tag::TAG_UNSTRUCTURED_GRID:
        break;
    
    case Tag::TAG_PIECE:
        break;

    case Tag::TAG_POINTS:
        break;

    case Tag::TAG_POINT_DATA:
        break;
    
    case Tag::TAG_CELLS:
        break;

    case Tag::TAG_CELL_DATA:
        break;
        
    case Tag::TAG_DATA_ARRAY:
        switch ( cbd->m_stack[ cbd->m_stack.size()-2 ].m_type ) {
        case Tag::TAG_CELLS:
            if( cbd->m_name == "connectivity" ) {
                LOGGER_DEBUG( cbd->m_log, "CONNECTIVITY!" );
            }
            else if( cbd->m_name == "offsets" ) {
                LOGGER_DEBUG( cbd->m_log, "OFFSETS" );
            }
            else if( cbd->m_name == "types" ) {
                LOGGER_DEBUG( cbd->m_log, "TYPES" );
            }
            break;
        default:
            break;
        }
        
        
        break;

    case Tag::TAG_SENTINEL:
        break;
    }

    
    LOGGER_DEBUG( cbd->m_log, "< " <<  tag_names[ cbd->m_stack.back().m_type ] << ", size=" << cbd->m_stack.size());
    
    Tag::Type tag_type = cbd->m_stack.back().m_type;
    
    switch ( tag_type ) {
    case Tag::TAG_SENTINEL:
        LOGGER_FATAL( cbd->m_log, "end_element: Encountered TAG_SENTINEL" );
        cbd->m_success = false;
        break;
    case Tag::TAG_VTKFILE:
        if( xmlStrEqual( name, (const xmlChar*)"VTKFile" ) == 0 ) {
            LOGGER_FATAL( cbd->m_log, "end_element: Expected VTKFile, got " << name );
            cbd->m_success = false;
        }
        break;
    default:
        break;
    }
    
    cbd->m_stack.pop_back();    // pop tag from stack
   
}
 

}


void
VtkXmlParser::parse(std::vector<float>&         vertices,
                     std::vector<int> &tetrahedra,
                     const std::string &filename)
{
    Logger log = getLogger( "VtkXmlParser.parse" );
    xmlSAXHandler saxf;
    bzero( &saxf, sizeof(saxf ) );
    saxf.startElement = start_element;
    saxf.endElement = end_element;

    callback_data saxd;
    saxd.m_success = true;
    saxd.m_log = log;
    saxd.m_stack.resize( 1 );
    saxd.m_stack[0].m_type = Tag::TAG_SENTINEL;
    
    int rv = xmlSAXUserParseFile( &saxf, &saxd, filename.c_str() );
    if( rv != 0 ) {
        LOGGER_ERROR( log, "xmlSAXUserParseFile returned " << rv );
        throw std::runtime_error( "failed to parse xml file" );
    }
    
    LOGGER_DEBUG( log, "blurp" );
    exit(0);
    
    vertices.resize( sizeof(dummy_vertices)/sizeof(float) );
    std::copy_n( &dummy_vertices[0], vertices.size(), vertices.begin() );
    
    tetrahedra.resize( sizeof(dummy_tetras)/sizeof(int) );
    std::copy_n( &dummy_tetras[0], tetrahedra.size(), tetrahedra.begin() );
}


