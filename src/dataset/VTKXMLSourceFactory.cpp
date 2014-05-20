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
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include <libxml/parser.h>
#include "utils/Logger.hpp"
#include "utils/Path.hpp"
#include "dataset/VTKXMLSourceFactory.hpp"
#include "dataset/PolyhedralMeshSource.hpp"
#include "dataset/PolygonMeshSource.hpp"

namespace {

struct Tag
{
    Tag()
        : m_data_array_components(1)
    {}

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

    std::vector<char>       m_char_buffer;  // temp buffer between invocations of characters_func
    std::string             m_data_array_name;
    int                     m_data_array_components;
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

struct CellData
{
    std::string             m_name;
    std::vector<float>      m_data;
};


struct callback_data {
    bool                                m_success;
    Logger                              m_log;
    std::vector<Tag>                    m_stack;

    size_t                              m_pieces_n;
    size_t                              m_piece_points_n;
    size_t                              m_piece_cells_n;
    std::vector<float>                  m_piece_points;
    std::vector<int>                    m_piece_connectivity;
    std::vector<int>                    m_piece_offsets;
    std::vector<int>                    m_piece_types;

    std::vector<std::string>            m_piece_vtx_data_name;
    std::vector< std::vector<float> >   m_piece_vtx_data_vals;

    std::vector<std::string>            m_piece_cell_data_name;
    std::vector< std::vector<float> >   m_piece_cell_data_vals;
};


void
characters_func(void *user_data, const xmlChar *ch, int len)
{
    callback_data* cbd = static_cast<callback_data*>( user_data );
    Tag& tag = cbd->m_stack.back();
    if( !cbd->m_success || (tag.m_handle_chars == Tag::CHARACTER_IGNORE) || len < 1 ) {
        return;
    }
    size_t o = tag.m_char_buffer.size();
    tag.m_char_buffer.resize( o + len );
    memcpy( tag.m_char_buffer.data() + o, ch, len );
}

void
start_element( void* user_data, const xmlChar* name, const xmlChar** attrs )
{
    callback_data* cbd = static_cast<callback_data*>( user_data );
    if( !cbd->m_success ) {
        return;
    }
    cbd->m_stack.resize( cbd->m_stack.size()+1 );   // add tag to stack
    Tag& parent = cbd->m_stack[ cbd->m_stack.size() - 2 ]; 
    Tag& tag    = cbd->m_stack[ cbd->m_stack.size() - 1 ];

    tag.m_type = Tag::TAG_UNKNOWN;
    tag.m_handle_chars = Tag::CHARACTER_IGNORE;

    // --- determine tag type --------------------------------------------------
    if( parent.m_type != Tag::TAG_UNKNOWN ) {
        for( int i=1; i<Tag::TAG_SENTINEL; i++ ) {
            if( xmlStrEqual( name, tag_names[ i ] ) ) {
                cbd->m_stack.back().m_type = (Tag::Type)i;
                break;
            }
        }
    }
    
    // --- parse attributes and set up character handling
    switch ( cbd->m_stack.back().m_type ) {
    //case Tag::TAG_UNKNOWN:
    //case Tag::TAG_VTKFILE:
    //case Tag::TAG_UNSTRUCTURED_GRID:
    
    case Tag::TAG_PIECE:
        if( cbd->m_pieces_n != 0 ) {
            LOGGER_ERROR( cbd->m_log, tag_names[ tag.m_type ]  << ": only one piece is currently supported." );
            cbd->m_success = false;
            return;
        }
        
        cbd->m_piece_points_n = 0;
        cbd->m_piece_cells_n = 0;
        for(int i=0; attrs[i] != NULL; i+=2 ) {
            if( xmlStrEqual( attrs[i], (const xmlChar*)"NumberOfPoints" ) ) {
                cbd->m_piece_points_n =  atoi( (const char*)attrs[i+1] );
            }
            else if( xmlStrEqual( attrs[i], (const xmlChar*)"NumberOfCells" ) ) {
                cbd->m_piece_cells_n = atoi( (const char*)attrs[i+1] );
            }
            else {
                LOGGER_WARN( cbd->m_log, tag_names[ tag.m_type ] << ": " << attrs[i] << "='" << attrs[i+1] << "' ignored." );
            }
        }
        break;

    //case Tag::TAG_POINTS:
    //case Tag::TAG_POINT_DATA:
    //case Tag::TAG_CELLS:
    //case Tag::TAG_CELL_DATA:
        
    case Tag::TAG_DATA_ARRAY:
        if(1) {
        
            if( attrs != NULL ) {
                for(int i=0; attrs[i] != NULL; i+=2 ) {
                    if( xmlStrEqual( attrs[i], (const xmlChar*)"type" ) ) {
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
                            tag.m_handle_chars = Tag::CHARACTER_FLOAT_ARRAY;
                        }
                    }
                    else if( xmlStrEqual( attrs[i], (const xmlChar*)"Name" ) ) {
                        tag.m_data_array_name = (const char*)attrs[i+1];
                    }
                    else if( xmlStrEqual( attrs[i], (const xmlChar*)"NumberOfComponents" ) ) {
                        tag.m_data_array_components = atoi( (const char*)attrs[i+1] );
                    }
                    else if( xmlStrEqual( attrs[i], (const xmlChar*)"format" ) ) {
                        if( !xmlStrEqual( attrs[i+1], (const xmlChar*)"ascii" ) ) {
                            LOGGER_ERROR( cbd->m_log, tag_names[ tag.m_type ]  << ": format='" << attrs[i+1] << "', only format='ascii' is currently supported" );
                            cbd->m_success = false;
                            return;
                        }
                    }
                    else if( xmlStrEqual( attrs[i], (const xmlChar*)"offset" ) ) {  //(( ascoo, binary, appended))
                        
                    }
                }
            }

        }
        
        break;

    default:
        if( attrs != NULL ) {
            for(int i=0; attrs[i] != NULL; i+=2 ) {
                LOGGER_WARN( cbd->m_log, '<' << tag_names[ tag.m_type ] << "> attribute " << attrs[i] << "='" << attrs[i+1] << "' ignored." );
            }
        }
        break;
    
    }
}



template<typename T>
void
body_contents_into_array( callback_data*   cbd,
                            std::vector<T>&  result,
                            Tag&             tag )
{
    result.clear();
    if( tag.m_handle_chars != Tag::CHARACTER_IGNORE ) {
        tag.m_char_buffer.push_back( '\0' ); // zero-terminate string

#ifdef DEBUG
        int tokens = 0;
        int nonprint = 0;
        int zerobytes = 0;
        int checksum = 0;
        for(size_t i=0; i<tag.m_char_buffer.size(); i++ ) {
            checksum = (13*checksum + tag.m_char_buffer[i])&0xffff;
            if( tag.m_char_buffer[i] == '\0' ) {
                zerobytes++;
            }
            else if( (tag.m_char_buffer[i] < 9 ) || (tag.m_char_buffer[i] > 126 ) ) {
                nonprint++;
            }
        }
 #endif

        char* p = tag.m_char_buffer.data();
        char* e = p+1;
        switch ( tag.m_handle_chars ) {
        case Tag::CHARACTER_IGNORE:
            break;
        case Tag::CHARACTER_FLOAT_ARRAY:
            while(1) {
                errno = 0;
                float v = strtof( p, &e );
                if( errno != 0 ) {
                    LOGGER_ERROR( cbd->m_log, "handle_value_array: parse_floats: " << strerror(errno) );
                    cbd->m_success = false;
                    return;
                }
                else if( p == e ) {
                    break;  // no more digits could be found
                }
                else {
                    result.push_back( static_cast<T>( v ) );
#ifdef DEBUG
                    tokens++;
#endif
                    p = e;
                }
            }
            break;

        case Tag::CHARACTER_INT_ARRAY:
            while(1) {
                errno = 0;
                int v = strtol( p, &e, 0 );
                if( errno != 0 ) {
                    LOGGER_ERROR( cbd->m_log, "handle_value_array: parse_int: " << strerror(errno) );
                    cbd->m_success = false;
                    return;
                }
                else if( p == e ) {
                    break;  // no more digits could be found
                }
                else {
                    result.push_back( static_cast<T>( v ) );
#ifdef DEBUG
                    tokens++;
#endif
                    p = e;
                }
            }
            break;
        }
#ifdef DEBUG
        LOGGER_DEBUG( cbd->m_log, "Parsed character data: " <<
                      tag.m_char_buffer.size() << " characters, " <<
                      tokens << " tokens, " <<
                      nonprint << " non-printable chars, " <<
                      zerobytes << " zero-bytes, checksum=" <<
                      checksum );
#endif
    }
}

void
handle_field_data( callback_data*                     cbd,
                   std::vector<std::string>&          names,
                   std::vector<std::vector<float> >&  values,
                   const size_t                       N,
                   const Tag&                         parent,
                   Tag&                               tag )
{
    if( cbd->m_success == false ) {
        return;
    }

    if( tag.m_data_array_name.empty() ) {
        LOGGER_ERROR( cbd->m_log,
                      tag_names[ parent.m_type] << '/' << tag_names[ tag.m_type ]  <<
                      " name attribute required." );
        cbd->m_success = false;
        return;
    }
    if( tag.m_data_array_components != 1 ) {
        LOGGER_ERROR( cbd->m_log,
                      tag_names[ parent.m_type] << '/' << tag_names[ tag.m_type ]  <<
                      " only single component is currently supported." );
        cbd->m_success = false;
        return;
    }

    values.push_back( std::vector<float>() );
    values.reserve( N );
    body_contents_into_array( cbd, values.back(), tag );

    if( values.back().size() != N ) {
        cbd->m_piece_cell_data_vals.pop_back();
        LOGGER_ERROR( cbd->m_log,
                      tag_names[ parent.m_type] << '/' << tag_names[ tag.m_type ]  <<
                      "/Name='" << tag.m_data_array_name << "':" <<
                      "Expected " << cbd->m_piece_cells_n <<
                       " values, got " << values.back().size() );
        cbd->m_success = false;
        return;
    }
    names.push_back( tag.m_data_array_name );
}

void
end_element( void* user_data, const xmlChar* name )
{
    callback_data* cbd = static_cast<callback_data*>( user_data );
    if( !cbd->m_success ) {
        return;
    }
    
    // --- sanity checks -------------------------------------------------------
    if( cbd->m_stack.size() < 2 ) {
        LOGGER_ERROR( cbd->m_log, "end_element: Stack underflow." );
        cbd->m_success = false;
        return;
    }
    Tag& tag = cbd->m_stack.back();
    if( tag.m_type >= Tag::TAG_SENTINEL ) {
        LOGGER_ERROR( cbd->m_log, "end_element: Illegal type on stack:" << cbd->m_stack.back().m_type );
        cbd->m_success = false;
        return;
    }
    // and unless unknown, check that tag matches
    if( tag.m_type != Tag::TAG_UNKNOWN ) {
        if( xmlStrEqual( name, tag_names[ tag.m_type ] ) == 0 ) {
            LOGGER_ERROR( cbd->m_log, "end_element: Expected " << tag_names[ tag.m_type ]
                                       << ", got " << name );
            cbd->m_success = false;
            return;
        }
    }
    
    // --- perform action on closing of tag ------------------------------------
    Tag& parent = cbd->m_stack[ cbd->m_stack.size() - 2 ]; 
    
    switch ( tag.m_type ) {
    case Tag::TAG_UNKNOWN:
        break;

    case Tag::TAG_VTKFILE:
        break;

    case Tag::TAG_UNSTRUCTURED_GRID:
        break;
    
    case Tag::TAG_PIECE:
        // run sanity checks
        if( cbd->m_piece_points.size() != 3 * cbd->m_piece_points_n ) {
            LOGGER_ERROR( cbd->m_log,
                          tag_names[ tag.m_type ]  <<
                          " points expected " << 3*cbd->m_piece_points_n << " floats, got " << cbd->m_piece_points.size() );
            cbd->m_success = false;
            return;
        }
        if( cbd->m_piece_types.size() != cbd->m_piece_cells_n ) {
            LOGGER_ERROR( cbd->m_log,
                          tag_names[ tag.m_type ]  <<
                          " expected " << cbd->m_piece_cells_n << " types, got " << cbd->m_piece_types.size() );
            cbd->m_success = false;
            return;
        }
        if( cbd->m_piece_offsets.size() != cbd->m_piece_cells_n ) {
            LOGGER_ERROR( cbd->m_log,
                          tag_names[ tag.m_type ]  <<
                          " expected " << cbd->m_piece_cells_n << " offsets, got " << cbd->m_piece_offsets.size() );
            cbd->m_success = false;
            return;
        }
        cbd->m_pieces_n++;
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
        switch ( parent.m_type ) {
        // --- <Points><DataArray> ---------------------------------------------
        case Tag::TAG_POINTS:
            if( tag.m_data_array_components != 3 ) {
                LOGGER_ERROR( cbd->m_log,
                              tag_names[ parent.m_type] << '/' << tag_names[ tag.m_type ]  <<
                              " only NumberOfComponents='3' is currently supported" );
                cbd->m_success = false;
                return;
            }
            body_contents_into_array( cbd, cbd->m_piece_points, tag );
            break;
        
        // --- <Cells><DataArray> ----------------------------------------------
        case Tag::TAG_CELLS:
            if( tag.m_data_array_name == "connectivity" ) {
                body_contents_into_array( cbd, cbd->m_piece_connectivity, tag );
            }
            else if( tag.m_data_array_name == "offsets" ) {
                body_contents_into_array( cbd, cbd->m_piece_offsets, tag );
            }
            else if( tag.m_data_array_name == "types" ) {
                body_contents_into_array( cbd, cbd->m_piece_types, tag );
            }
            break;
            
        // --- <PointData><DataArray> ------------------------------------------
        case Tag::TAG_POINT_DATA:
            break;

            // --- <CellData><DataArray> ----------------------------------------------
        case Tag::TAG_CELL_DATA:
            // A bit unsure how to interpret Scalars/Normals/.. attributes.
            handle_field_data( cbd,
                               cbd->m_piece_cell_data_name,
                               cbd->m_piece_cell_data_vals,
                               cbd->m_piece_cells_n,
                               parent,
                               tag );
             break;
        default:
            break;
        }
        
        
        break;

    case Tag::TAG_SENTINEL:
        break;
    }

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
 

} // of anonymous namespace

namespace dataset {

boost::shared_ptr<dataset::AbstractDataSource>
VTKXMLSourceFactory::FromVTUFile( const std::string& filename )
{
    // parse XML
    Logger log = getLogger( "dataset.VTKXMLSourceFactory" );
    xmlSAXHandler saxf;
    bzero( &saxf, sizeof(saxf ) );
    saxf.startElement = start_element;
    saxf.endElement = end_element;
    saxf.characters = characters_func;

    callback_data cd;
    cd.m_success = true;
    cd.m_log = log;
    cd.m_stack.resize( 1 );
    cd.m_stack[0].m_type = Tag::TAG_SENTINEL;
    cd.m_pieces_n = 0;
    
    int rv = xmlSAXUserParseFile( &saxf, &cd, filename.c_str() );
    if( rv != 0 ) {
        LOGGER_ERROR( log, "xmlSAXUserParseFile returned " << rv );
        throw std::runtime_error( "Failed to parse XML file" );
    }
    if( !cd.m_success ) {
        throw std::runtime_error( "Failed to interpret XML file" );
    }

    
    // convert various primitives to polyhedrons
    bool volume_data = false;
    bool surface_data = false;
    std::vector<int>   indices;
    std::vector<int>   polygons;
    std::vector<int>   cells;
    
    
    for(size_t c=0; c<cd.m_piece_cells_n; c++ ) {
        size_t o = c < 1 ? 0 : cd.m_piece_offsets[c-1];
        size_t n = cd.m_piece_offsets[c] - o;
        
        switch( cd.m_piece_types[c] ) {
        case 1: // VTK_VERTEX
        case 2: // VTK_POLY_VERTEX
        case 3: // VTK_LINE
        case 4: // VTK_POLY_LINE
        case 8: // VTK_PIXEL
        case 11: // VTK_VOXEL
            LOGGER_WARN( log, "Unsupported type " << cd.m_piece_types[c] );
            break;
            
        case 5:  // VTK_TRIANGLE
            surface_data = true;
            if( n != 3 ) {
                LOGGER_ERROR( log, "VTK_TRIANGLE expects 3 vertices, got " << n );
                throw std::runtime_error( "Inconsistency in VTK data" );
            }
            cells.push_back( polygons.size() );
            polygons.push_back( indices.size() );
            indices.push_back( cd.m_piece_connectivity[o+0] );
            indices.push_back( cd.m_piece_connectivity[o+1] );
            indices.push_back( cd.m_piece_connectivity[o+2] );
            break;
        case 6:  // VTK_TRIANGLE_STRIP
        case 7:  // VTK_POLYGON
        case 9:  // VTK_QUAD
        case 12: // VTK_HEXAHEDRON
        case 13: // VTK_WEDGE
        case 14: // VTK_PYRAMID
            LOGGER_WARN( log, "Unimplemented type " << cd.m_piece_types[c] );
            break;

        case 10: // VTK_TETRA
            if( n != 4 ) {
                LOGGER_ERROR( log, "VTK_TETRA expects 4 vertices, got " << n );
                throw std::runtime_error( "Inconsistency in VTK data" );
            }
            volume_data = true;
            cells.push_back( polygons.size() );

            // create tetra faces
            polygons.push_back( indices.size() );
            indices.push_back( cd.m_piece_connectivity[o+0] );
            indices.push_back( cd.m_piece_connectivity[o+3] );
            indices.push_back( cd.m_piece_connectivity[o+2] );
            
            polygons.push_back( indices.size() );
            indices.push_back( cd.m_piece_connectivity[o+1] );
            indices.push_back( cd.m_piece_connectivity[o+3] );
            indices.push_back( cd.m_piece_connectivity[o+0] );

            polygons.push_back( indices.size() );
            indices.push_back( cd.m_piece_connectivity[o+2] );
            indices.push_back( cd.m_piece_connectivity[o+3] );
            indices.push_back( cd.m_piece_connectivity[o+1] );

            polygons.push_back( indices.size() );
            indices.push_back( cd.m_piece_connectivity[o+1] );
            indices.push_back( cd.m_piece_connectivity[o+0] );
            indices.push_back( cd.m_piece_connectivity[o+2] );

            break;
 
        default:
            LOGGER_WARN( log, "Unknown type " << cd.m_piece_types[c] );
            break;
        }
    }
    cells.push_back( polygons.size() );
    polygons.push_back( indices.size() );

    std::string path, stem, suffix;
    utils::Path::split( path, stem, suffix, filename );
    
    boost::shared_ptr<dataset::AbstractDataSource> retval;
    if( volume_data && surface_data ) {
        LOGGER_ERROR( log, "Both surface and volumetric data in " << filename );
    }
    else if( volume_data ) {
        LOGGER_DEBUG( log, "created volume with "
                      << (cd.m_piece_points.size()/3) << " vertices, "
                      << indices.size() << " indices, "
                      << (polygons.size()-1) << " polygons, and "
                      << (cells.size()-1) << " cells." );
        retval.reset( new PolyhedralMeshSource( stem,
                                                cd.m_piece_points,
                                                indices,
                                                polygons,
                                                cells,
                                                cd.m_piece_cell_data_name,
                                                cd.m_piece_cell_data_vals ) );
    }
    else if( surface_data ) {
        LOGGER_DEBUG( log, "created surface with "
                      << (cd.m_piece_points.size()/3) << " vertices, "
                      << indices.size() << " indices, "
                      << (polygons.size()-1) << " polygons, and "
                      << (cells.size()-1) << " cells." );
        retval.reset( new PolygonMeshSource( stem,
                                             cd.m_piece_points,
                                             indices,
                                             polygons,
                                             cells,
                                             cd.m_piece_cell_data_name,
                                             cd.m_piece_cell_data_vals ) );
        
    }
    else {
        LOGGER_WARN( log, "No data found in " << filename );
    }
    return retval;
}



} // of namespace dataset
