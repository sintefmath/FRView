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

#include <iostream>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "dataset/PolyhedralMeshSource.hpp"
#include "render/GridTessBridge.hpp"
#include "render/GridFieldBridge.hpp"

namespace {

template<typename Index>
struct HalfPolygon
{
    HalfPolygon( Index cell, Index* indices, int n, bool side )
        : m_cell( cell ), m_indices( indices ), m_n( n ), m_side( side ) {}

    /** Used for lexicographical sort. */
    bool
    operator<( const HalfPolygon& other ) const
    {
        // fewer indices is smaller than many indices
        /*if( m_n < other.m_n ) {
            return true;
        }
        else if( m_n > other.m_n ) {
            return false;
        }*/
        // same number of indices, compare lexicographically
        for( int i=0; i<m_n; i++ ) {
            if( m_indices[i] < other.m_indices[i] ) {
                return true;
            }
            else if( m_indices[i] > other.m_indices[i] ) {
                return false;
            }
        }
        return true;    // identical
    }

    bool
    match( const HalfPolygon& other ) const
    {
        if( m_n != other.m_n ) {
            return false;
        }
        for( int i=0; i<m_n; i++ ) {
            if( m_indices[i] != other.m_indices[i] ) {
                return false;
            }
        }
        return true;    // identical
    }
    
    
    Index           m_cell;
    Index*          m_indices;
    int             m_n;        // should be safe to assume that the vertex
                                // count of a polygon is less than 31 bits.
    bool            m_side;     // side 0 is not flipped, side 1 is flipped.
};


}


namespace dataset {

template<typename Tessellation>
void
PolyhedralMeshSource::tessellation( Tessellation& tessellation,
                                    boost::shared_ptr<tinia::model::ExposedModel> model )
{
    Logger log = getLogger( "dataset.PolyhedralMeshSource.tessellation" );
    
    typedef float                               SrcReal;
    typedef typename Tessellation::Real         Real;
    typedef typename Tessellation::Index        Index;
    typedef typename Tessellation::Real4        Real4;
    typedef typename Tessellation::Orientation  Orientation;
    typedef typename Tessellation::Segment      Segment;
    typedef typename Tessellation::Interface    Interface;
    static const Orientation ORIENTATION_I = Tessellation::ORIENTATION_I;
    static const Orientation ORIENTATION_J = Tessellation::ORIENTATION_J;
    static const Orientation ORIENTATION_K = Tessellation::ORIENTATION_K;
    static const Index IllegalIndex = (~(Index)0u);

    model->updateElement<std::string>( "asyncreader_what", "Tetra!" );
    model->updateElement<int>( "asyncreader_progress", 0 );

    // copy vertices
    for(size_t i=0; i<m_vertices.size(); i+=3 ) {
        tessellation.addVertex( Real4( m_vertices[i+0],
                                       m_vertices[i+1],
                                       m_vertices[i+2] ) );
    }

    // used to hold the unique set of indices for a cell.
    std::vector<Index> cell_corners;
    
    // copy of indices with rotation and reverse-symmetries removed, such that
    // matching polygons will have identical index sequences.
    std::vector<Index> indices( m_indices.size() );

    // Half-polygons that we will match. Note that these contain pointers into
    // the indices-arrays, and thus, indices must not be resized (may trigger
    // reallocation).
    std::vector<HalfPolygon<Index> > half_polygons;
    
    // Populate cell_corners and indices
    
    tessellation.setCellCount( m_cells.size()-1 );
    for(Index c=0; (c+1)<m_cells.size(); c++ ) {
        Index p_o = m_cells[c];
        Index p_n = m_cells[c+1]-p_o;
        
        cell_corners.clear();
        Index k = 0;
        for(Index p=0; p<p_n; p++) {
            Index i_o = m_polygons[p_o+p];
            Index i_n = m_polygons[p_o+p+1]-i_o;

            // find smallest index in polygon
            for(Index i=0; i<i_n; i++ ) {
                if( m_indices[i_o+i] < m_indices[i_o+k] ) {
                    k = i;
                }
            }

            // check if we should reverse the order of the indices
            bool flip = m_indices[ i_o + ((k+2)%i_n) ] < m_indices[ i_o + ((k+1)%i_n) ];
            
            // rotate (and optionally flip) indices into new buffer
            for( Index i=0; i<i_n; i++ ) {
                Index ix = m_indices[ i_o + ((k+ (flip ? i_n-i : i ))%i_n) ];
                indices[ i_o + i ] = ix;
                cell_corners.push_back( ix );
            }
            
            half_polygons.push_back( HalfPolygon<Index>( c,
                                                         indices.data() + i_o,
                                                         i_n,
                                                         flip ) );
            
            // invariant check that index 0 is smallest and that index 1 is
            // smaller than index 2.
            for( Index i=1; i<i_n; i++) {
                assert( indices[i_o] < indices[i_o+i] );
            }
            assert( indices[i_o+1] < indices[i_o+2] );
        }

        // remove duplicate cell vertex indices        
        std::sort( cell_corners.begin(), cell_corners.end() );
        typename std::vector<Index>::iterator it = std::unique( cell_corners.begin(), cell_corners.end() );
        cell_corners.resize( std::distance( cell_corners.begin(), it ) );
        
        tessellation.setCell( c, c,
                              cell_corners[0],
                              cell_corners[1 % cell_corners.size() ],
                              cell_corners[2 % cell_corners.size() ],
                              cell_corners[3 % cell_corners.size() ],
                              cell_corners[4 % cell_corners.size() ],
                              cell_corners[5 % cell_corners.size() ],
                              cell_corners[6 % cell_corners.size() ],
                              cell_corners[7 % cell_corners.size() ] );
        
        if( cell_corners.size() != 4 ) {
            std::cerr << c << ": ";
            for( size_t i=0; i<cell_corners.size(); i++ ) {
                std::cerr << cell_corners[i] << ", ";
            }
            std::cerr << "\n";
        }

    }
    
    // --- match half-polygons to find adjacent cells --------------------------

    // step 1: sort half-polygons lexicographically
    std::sort( half_polygons.begin(), half_polygons.end() );

    // step 2: find matching half-polygons
    
    size_t iface_i = 0;
    size_t iface_b = 0;
    size_t iface_f = 0;
    std::vector<Segment> segments;
    for( size_t j=0; j<half_polygons.size(); ) {

        
        
        // create normal vector; we currently assume planar polygons, should
        // do something more clever.

        glm::vec3 a = glm::make_vec3( m_vertices.data() + 3*half_polygons[j].m_indices[0] );
        glm::vec3 b = glm::make_vec3( m_vertices.data() + 3*half_polygons[j].m_indices[1] );
        glm::vec3 c = glm::make_vec3( m_vertices.data() + 3*half_polygons[j].m_indices[2] );
        glm::vec3 n = glm::normalize( glm::cross( b-a, c-a ) );

        Index n_ix = tessellation.addNormal( Real4( n.x, n.y, n.z ) );
        
        Orientation orientation = ORIENTATION_I;
        if( glm::abs(n.y) > glm::abs(n.x) ) {
            orientation = ORIENTATION_J;
        }
        if( glm::abs(n.z) > glm::abs(n.y) ) {
            orientation = ORIENTATION_K;
        }

        // create segment list
        segments.resize( half_polygons[j].m_n );
        for( int i=0; i<half_polygons[j].m_n; i++ ) {
            segments[i] = Segment( n_ix, half_polygons[j].m_indices[i], 3 );
        }

        // find matching half-polygons            
        size_t i=j+1;
        for( ; i<half_polygons.size(); i++ ) {
            if( !half_polygons[j].match( half_polygons[i ] ) ) {
                break;
            }
        }
        
        // specify connectivity
        Interface interface;
        if( i == j+1 ) {
            // boundary face
            if( half_polygons[j].m_side == false ) {
                interface = Interface( half_polygons[j].m_cell, IllegalIndex, orientation );
            }
            else {
                interface = Interface( IllegalIndex, half_polygons[j].m_cell, orientation );
            }
            iface_b++;
            tessellation.addPolygon( interface, segments.data(), segments.size() );
        }
        else if( i == j+2 ) {
            // interior face
            if( half_polygons[j].m_side == half_polygons[j+1].m_side ) {
                LOGGER_WARN( log, "Inconsistent mesh face." );
                continue;
            }
            if( half_polygons[j].m_side == false ) {
                interface = Interface( half_polygons[j].m_cell, half_polygons[j+1].m_cell, orientation );
            }
            else {
                interface = Interface( half_polygons[j+1].m_cell, half_polygons[j].m_cell, orientation );
            }
            iface_i++;
            tessellation.addPolygon( interface, segments.data(), segments.size() );
        }
        else {
            // non-manifold face
            LOGGER_WARN( log, "Non-manifold face." );
            iface_f++;
        }
        j=i;
    }
    LOGGER_DEBUG( log, iface_b << " boundary interfaces, " <<
                  iface_i << " internal interfaces, and " <<
                  iface_f << " non-manifold interfaces." );


}

template void PolyhedralMeshSource::tessellation< render::GridTessBridge >( render::GridTessBridge& tessellation,
boost::shared_ptr<tinia::model::ExposedModel> model );


template<typename Field>
void
PolyhedralMeshSource::field( Field& field, const size_t index ) const
{
    if( field.count() != m_cell_field_data[ index ].size() ) {
        throw std::runtime_error( "element count mismatch" );
    }
    if( m_cell_field_data[ index ].empty() ) {
        // nothing to do
        return;
    }
    field.minimum() = m_cell_field_data[ index ][0];
    field.maximum() = m_cell_field_data[ index ][0];
    for( size_t i=0; i<m_cell_field_data[ index ].size(); i++ ) {
        typename Field::REAL v = m_cell_field_data[ index ][i];
        field.minimum() = std::min( field.minimum(), v );
        field.maximum() = std::max( field.minimum(), v );
        field.values()[i] = v;
    }
}

template void PolyhedralMeshSource::field<render::GridFieldBridge>( render::GridFieldBridge&, const size_t ) const;

} // of namespace input
