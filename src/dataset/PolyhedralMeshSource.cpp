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
#include "dataset/PolyhedralMeshSource.hpp"
#include "render/GridTessBridge.hpp"

namespace {

/** Helper struct used to build cell-cell connectivity. */
struct HalfTriangle
{
    HalfTriangle( int a, int b, int c, int cell )
    {
#ifdef DEBUG
        assert( a != b );
        assert( a != c );
        assert( b != c );
#endif
        std::cerr << a << ", " << b << ", " << c << " -> ";
        
        // make sure a is the smallest index
        if( b < a ) {
            int t = a;
            a = b;
            b = c;
            c = t;
        }
        else if( c < a ) {
            int t = b;
            b = a;
            a = c;
            c = t;
        }
#ifdef DEBUG
        std::cerr << a << ", " << b << ", " << c << "\n";
               
        
        assert( a < b );
        assert( a < c );
#endif
        // optionally flip 
        if( b < c ) {
            m_flipped = false;
        }
        else {
            int t = b;
            b = c;
            c = t;
            m_flipped = true;
        }
        
        m_a = a;
        m_b = b;
        m_c = c;
        m_cell = cell;
    }

    /** Lexicographical sort. */
    bool
    operator<( const HalfTriangle& other ) const
    {
        if( m_a < other.m_a ) {
            return true;
        }
        else if( m_a == other.m_a ) {
            if( m_b < other.m_b ) {
                return true;
            }
            else if( m_b == other.m_b ) {
                
                if( m_c < other.m_c ) {
                    return true;
                }
            }
        }
        return false;
    }
    
    bool
    match( const HalfTriangle& other ) const {
        return (m_a == other.m_a) && (m_b == other.m_b) && (m_c == other.m_c);
    }

    int m_a;
    int m_b;
    int m_c;
    int m_cell;
    bool m_flipped;

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

    std::vector<Index> tmp;
    
    std::vector<Index> indices;
    
    // copy indices and remove symmetries s.t. pairs of polygons can be directly
    // compared
    for(Index c=0; (c+1)<m_cells.size(); c++ ) {
        Index p_o = m_cells[c];
        Index p_n = m_cells[c+1]-p_o;
        
        tmp.clear();
        for(Index p=0; p<p_n; p++) {
            Index i_o = m_polygons[p_o+p];
            Index i_n = m_polygons[p_o+p+1]-i_o;

            // Find min index & copy to tmp
            for(Index i=0; i<i_n; i++ ) {
                tmp.push_back( m_indices[i_o+i] );
            }
        }
        std::sort( tmp.begin(), tmp.end() );
        typename std::vector<Index>::iterator it = std::unique( tmp.begin(), tmp.end() );
        tmp.resize( std::distance( tmp.begin(), it ) );
        
        for( size_t i=0; i<tmp.size(); i++ ) {
            std::cerr << c << ": " << tmp[i] << ", ";
        }
        std::cerr << "\n";
        
        
    }

    
    
    // extract half-triangles
    std::vector<HalfTriangle> ht;

#if 0
    m_tessellation.setCellCount( indices.size()/4 );
    for(size_t i=0; i<indices.size(); i+=4 ) {
        m_tessellation.setCell( i, i,
                                 indices[i+0], indices[i+0],
                                 indices[i+1], indices[i+1],
                                 indices[i+2], indices[i+2],
                                 indices[i+3], indices[i+3] );
        
        
        ht.push_back( HalfTriangle( indices[i+0], indices[i+3], indices[i+2], i ) );
        ht.push_back( HalfTriangle( indices[i+1], indices[i+3], indices[i+0], i ) );
        ht.push_back( HalfTriangle( indices[i+2], indices[i+3], indices[i+1], i ) );
        ht.push_back( HalfTriangle( indices[i+0], indices[i+2], indices[i+1], i ) );
    }

    std::sort( ht.begin(), ht.end() );
    
    for( size_t j=0; j<ht.size(); ) {
        size_t i=j+1;
        for( ; ht[j].match( ht[i] ); i++ ) ;
        
        
        j = i;
    }
#endif    
}

template void PolyhedralMeshSource::tessellation< render::GridTessBridge >( render::GridTessBridge& tessellation,
boost::shared_ptr<tinia::model::ExposedModel> model );


} // of namespace input
