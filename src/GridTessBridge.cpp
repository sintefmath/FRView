/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <math.h>
#include "GridTessBridge.hpp"



void
GridTessBridge::reserveVertices( unsigned int N )
{
    m_vertices.reserve( 4*N );
}

void
GridTessBridge::reserveEdges( unsigned int N )
{
}

void
GridTessBridge::reserveTriangles( unsigned int N )
{
    m_triangle_info.reserve( 3*N ); // ??
    m_triangles.reserve( 3*N );
}

void
GridTessBridge::reserveCells( unsigned int N )
{
    m_cell_index.resize( N );
    m_cell_corner.resize( 8*N );
}


void
GridTessBridge::addVertex( const float x, const float y, const float z, const float w )
{
    m_vertices.push_back( x );
    m_vertices.push_back( y );
    m_vertices.push_back( z );
    m_vertices.push_back( w );
}

void
GridTessBridge::addEdge( const unsigned int ix0, const unsigned int ix1,
                         const unsigned int cell_a, const unsigned cell_b,
                         const unsigned int cell_c, const unsigned cell_d )
{
}

unsigned int
GridTessBridge::vertexCount() const
{
    return m_vertices.size()/4;
}

unsigned int
GridTessBridge::cellCount() const
{
    return m_cell_index.size();
}

void
GridTessBridge::setCellCount( unsigned int N )
{
    m_cell_index.resize( N );
    m_cell_corner.resize( 8*N );
}

void
GridTessBridge::addCell( const unsigned int global_index,
                         const unsigned int vertex_0,
                         const unsigned int vertex_1,
                         const unsigned int vertex_2,
                         const unsigned int vertex_3,
                         const unsigned int vertex_4,
                         const unsigned int vertex_5,
                         const unsigned int vertex_6,
                         const unsigned int vertex_7 )
{
    m_cell_index.push_back( global_index );
    m_cell_corner.push_back( vertex_0 );
    m_cell_corner.push_back( vertex_1 );
    m_cell_corner.push_back( vertex_2 );
    m_cell_corner.push_back( vertex_3 );
    m_cell_corner.push_back( vertex_4 );
    m_cell_corner.push_back( vertex_5 );
    m_cell_corner.push_back( vertex_6 );
    m_cell_corner.push_back( vertex_7 );
}

void
GridTessBridge::setCell( const unsigned int index,
                         const unsigned int global_index,
                         const unsigned int vertex_0,
                         const unsigned int vertex_1,
                         const unsigned int vertex_2,
                         const unsigned int vertex_3,
                         const unsigned int vertex_4,
                         const unsigned int vertex_5,
                         const unsigned int vertex_6,
                         const unsigned int vertex_7 )
{
    m_cell_index[ index ] = global_index;
    m_cell_corner[ 8*index + 0 ] = vertex_0;
    m_cell_corner[ 8*index + 1 ] = vertex_1;
    m_cell_corner[ 8*index + 2 ] = vertex_2;
    m_cell_corner[ 8*index + 3 ] = vertex_3;
    m_cell_corner[ 8*index + 4 ] = vertex_4;
    m_cell_corner[ 8*index + 5 ] = vertex_5;
    m_cell_corner[ 8*index + 6 ] = vertex_6;
    m_cell_corner[ 8*index + 7 ] = vertex_7;
}

void
GridTessBridge::addTriangle( const Orientation orientation,
                             const bool is_fault,
                             const unsigned int cell_a,
                             const bool edge_a_0_1,
                             const bool edge_a_1_2,
                             const bool edge_a_2_0,
                             const unsigned cell_b,
                             const bool edge_b_0_1,
                             const bool edge_b_1_2,
                             const bool edge_b_2_0,
                             const unsigned int ix0, const unsigned ix1, const unsigned ix2 )
{
#ifdef DEBUG
    Logger log = getLogger( "GridTessBridge.addTriangle" );
    if( (ix0 == ix1) || (ix1 == ix2) || (ix0 == ix2) ) {
        LOGGER_ERROR( log, "degenerate triangle" );
    }

    if( cell_a == ~0u && cell_b == ~0u ) {
        LOGGER_ERROR( log, "triangle without any adjacent cells" );
//            return;
    }
    //for( unsigned int i=0; i<m_triangles.size(); i+= 3 ) {
    //    if( (m_triangles[i]==ix0) && (m_triangles[i+1]==ix1) && (m_triangles[i+2]==ix2) ) {
    //        LOGGER_ERROR( log, "duplicate triangle detected." );
    //    }
    //}
#endif
    const unsigned int flags_a = (is_fault   ? 1u<<31u : 0u ) |
                                 (edge_a_0_1 ? 1u<<29u : 0u ) |
                                 (edge_a_1_2 ? 1u<<30u : 0u ) |
                                 (edge_a_2_0 ? 1u<<28u : 0u );
    const unsigned int flags_b = (edge_b_0_1 ? 1u<<30u : 0u ) |
                                 (edge_b_1_2 ? 1u<<29u : 0u ) |
                                 (edge_b_2_0 ? 1u<<28u : 0u );
    //const unsigned int flags = (unsigned int)(orientation)<<29u;
    m_triangle_info.push_back( flags_a | cell_a );
    m_triangle_info.push_back( flags_b | cell_b );
    m_triangles.push_back( ix0 );
    m_triangles.push_back( ix1 );
    m_triangles.push_back( ix2 );
}


void
GridTessBridge::addPolygon( const Orientation   orientation,
                            const bool          is_fault,
                            const unsigned int  cell_a,
                            const unsigned int  cell_b,
                            const Segment*      segments,
                            const unsigned int  N,
                            const unsigned int  split_hint )
{
    Logger log = getLogger( "GridTessBridge.addPolygon" );

    // Doesn't make sense, and algorithm assumes that this is the case.
    LOGGER_INVARIANT_LESS_EQUAL( log, 2, split_hint );


    std::vector<Segment> s( N );
    for(unsigned int i=0; i<N; i++) {
        s[i] = segments[i];
#ifdef CHECK_INVARIANTS
        for( unsigned int k=0; k<i; k++ ) {
            if( s[k].m_v == s[i].m_v ) {
                LOGGER_ERROR( log, "Duplicate vertex" );
            }
        }
#endif
    }

    unsigned int split_i1 = split_hint - 1u;
    while( s.size() > 2 ) {

        if( split_i1 < 1 ) {
            split_i1 = ~0u;
        }

        float px = vertexX( s[1].m_v ) - vertexX( s[0].m_v );
        float py = vertexY( s[1].m_v ) - vertexY( s[0].m_v );
        float pz = vertexZ( s[1].m_v ) - vertexZ( s[0].m_v );
        float r = 1.f/sqrtf( px*px + py*py + pz*pz );
        px = r*px;
        py = r*py;
        pz = r*pz;

        unsigned int best_ix = 0;
        float best_dot = 1.f;

        // Find sharpest corner. We assume convexity here...
        if( s.size() > 3 ) {


            unsigned int stop = split_i1 < s.size() ? split_i1 : s.size();
            for( unsigned int i0=0; i0<stop; i0++ ) {
                unsigned int i1 = (i0+1)%s.size();
                unsigned int i2 = (i0+2)%s.size();
                float cx = vertexX( s[i2].m_v ) - vertexX( s[i1].m_v );
                float cy = vertexY( s[i2].m_v ) - vertexY( s[i1].m_v );
                float cz = vertexZ( s[i2].m_v ) - vertexZ( s[i1].m_v );
                float r = 1.f/sqrtf( cx*cx + cy*cy + cz*cz );
                cx = r*cx;
                cy = r*cy;
                cz = r*cz;

                float dot = px*cx + py*cy + pz*cz;
                if( dot < best_dot ) {
                    best_ix = i0;
                    best_dot = dot;
                }
                px = cx;
                py = cy;
                pz = cz;
            }
        }

        // produce triange i,i+1,i+2
        unsigned int i0 = best_ix;
        unsigned int i1 = (i0+1)%s.size();
        unsigned int i2 = (i0+2)%s.size();

        unsigned int flags_a = cell_a;
        flags_a |= (is_fault   ? 1u<<31u : 0u );
        flags_a |= (s[i0].m_flags & 1u) << 29u;     // 0,1
        flags_a |= (s[i1].m_flags & 1u) << 30u;     // 1,2
        flags_a |= ((s.size()==3) ? (s[i2].m_flags & 0x1u) : 0) << 28u;
        //flags_a |= (1u<<30u);
        //flags_a |= (1u<<29u);
        //flags_a |= (1u<<28);

        unsigned int flags_b = cell_b;
        flags_b |= (s[i0].m_flags & 2u) << 29u;
        flags_b |= (s[i1].m_flags & 2u) << 28u;
        flags_b |= ((s.size()==3) ? (s[i2].m_flags & 0x2 ) : 0 ) << 27u;
        //flags_b |= (1u<<30u);
        //flags_b |= (1u<<29u);
        //flags_b |= (1u<<28u);


        m_triangle_info.push_back( flags_a );
        m_triangle_info.push_back( flags_b );
        m_triangles.push_back( s[i0].m_v );
        m_triangles.push_back( s[i1].m_v );
        m_triangles.push_back( s[i2].m_v );

        // remove i+1
        split_i1--;
        for(unsigned int i=i1; i<s.size()-1; i++ ) {
            s[i] = s[i+1];
        }
        s[i0].m_flags = 0u;
        s.resize( s.size()-1 );
    }
}



void
GridTessBridge::addQuadrilateral( const Orientation orientation,
                                  const bool is_fault,
                                  const unsigned int cell_a,
                                  const bool edge_a_0_1,
                                  const bool edge_a_1_2,
                                  const bool edge_a_2_3,
                                  const bool edge_a_3_0,
                                  const unsigned cell_b,
                                  const bool edge_b_0_1,
                                  const bool edge_b_1_2,
                                  const bool edge_b_2_3,
                                  const bool edge_b_3_0,
                                  const unsigned int ix0,
                                  const unsigned int ix1,
                                  const unsigned int ix2,
                                  const unsigned int ix3 )
{
    unsigned int flags_a, flags_b;

    flags_a = (is_fault   ? 1u<<31u : 0u ) |
            (edge_a_0_1 ? 1u<<29u : 0u ) |
            (edge_a_1_2 ? 1u<<30u : 0u );
    flags_b = (edge_b_0_1 ? 1u<<30u : 0u ) |
            (edge_b_1_2 ? 1u<<29u : 0u );
    m_triangle_info.push_back( flags_a | cell_a );
    m_triangle_info.push_back( flags_b | cell_b );
    m_triangles.push_back( ix0 );
    m_triangles.push_back( ix1 );
    m_triangles.push_back( ix2 );

    flags_a = (is_fault   ? 1u<<31u : 0u ) |
            (edge_a_2_3 ? 1u<<29u : 0u ) |
            (edge_a_3_0 ? 1u<<30u : 0u );
    flags_b = (edge_b_2_3 ? 1u<<30u : 0u ) |
            (edge_b_3_0 ? 1u<<29u : 0u );
    m_triangle_info.push_back( flags_a | cell_a );
    m_triangle_info.push_back( flags_b | cell_b );
    m_triangles.push_back( ix2 );
    m_triangles.push_back( ix3 );
    m_triangles.push_back( ix0 );
}

void
GridTessBridge::addPentagon( const Orientation orientation,
                             const bool is_fault,
                             const unsigned int cell_a,
                             const bool edge_a_0_1,
                             const bool edge_a_1_2,
                             const bool edge_a_2_3,
                             const bool edge_a_3_4,
                             const bool edge_a_4_0,
                             const unsigned cell_b,
                             const bool edge_b_0_1,
                             const bool edge_b_1_2,
                             const bool edge_b_2_3,
                             const bool edge_b_3_4,
                             const bool edge_b_4_0,
                             const unsigned int ix0,
                             const unsigned int ix1,
                             const unsigned int ix2,
                             const unsigned int ix3,
                             const unsigned int ix4 )
{
    unsigned int flags_a, flags_b;
    flags_a = (is_fault   ? 1u<<31u : 0u ) |
            (edge_a_0_1 ? 1u<<29u : 0u ) |
            (edge_a_1_2 ? 1u<<30u : 0u );
    flags_b = (edge_b_0_1 ? 1u<<30u : 0u ) |
            (edge_b_1_2 ? 1u<<29u : 0u );
    m_triangle_info.push_back( flags_a | cell_a );
    m_triangle_info.push_back( flags_b | cell_b );
    m_triangles.push_back( ix0 );
    m_triangles.push_back( ix1 );
    m_triangles.push_back( ix2 );

    flags_a = (is_fault   ? 1u<<31u : 0u ) |
            (edge_a_2_3 ? 1u<<29u : 0u ) |
            (edge_a_3_4 ? 1u<<30u : 0u );
    flags_b = (edge_b_2_3 ? 1u<<30u : 0u ) |
            (edge_b_3_4 ? 1u<<29u : 0u );
    m_triangle_info.push_back( flags_a | cell_a );
    m_triangle_info.push_back( flags_b | cell_b );
    m_triangles.push_back( ix2 );
    m_triangles.push_back( ix3 );
    m_triangles.push_back( ix4 );

    flags_a = (is_fault   ? 1u<<31u : 0u ) |
            (edge_a_4_0 ? 1u<<29u : 0u );
    flags_b = (edge_b_4_0 ? 1u<<30u : 0u );
    m_triangle_info.push_back( flags_a | cell_a );
    m_triangle_info.push_back( flags_b | cell_b );
    m_triangles.push_back( ix4 );
    m_triangles.push_back( ix0 );
    m_triangles.push_back( ix2 );
}

void
GridTessBridge::addHexagon( const Orientation orientation,
                            const bool is_fault,
                            const unsigned int cell_a,
                            const bool edge_a_0_1,
                            const bool edge_a_1_2,
                            const bool edge_a_2_3,
                            const bool edge_a_3_4,
                            const bool edge_a_4_5,
                            const bool edge_a_5_0,
                            const unsigned cell_b,
                            const bool edge_b_0_1,
                            const bool edge_b_1_2,
                            const bool edge_b_2_3,
                            const bool edge_b_3_4,
                            const bool edge_b_4_5,
                            const bool edge_b_5_0,
                            const unsigned int ix0,
                            const unsigned int ix1,
                            const unsigned int ix2,
                            const unsigned int ix3,
                            const unsigned int ix4,
                            const unsigned int ix5 )
{
    unsigned int flags_a, flags_b;
    flags_a = (is_fault   ? 1u<<31u : 0u ) |
            (edge_a_0_1 ? 1u<<29u : 0u ) |
            (edge_a_1_2 ? 1u<<30u : 0u );
    flags_b = (edge_b_0_1 ? 1u<<30u : 0u ) |
            (edge_b_1_2 ? 1u<<29u : 0u );
    m_triangle_info.push_back( flags_a | cell_a );
    m_triangle_info.push_back( flags_b | cell_b );
    m_triangles.push_back( ix0 );
    m_triangles.push_back( ix1 );
    m_triangles.push_back( ix2 );

    flags_a = (is_fault   ? 1u<<31u : 0u ) |
            (edge_a_2_3 ? 1u<<29u : 0u ) |
            (edge_a_3_4 ? 1u<<30u : 0u );
    flags_b = (edge_b_2_3 ? 1u<<30u : 0u ) |
            (edge_b_3_4 ? 1u<<29u : 0u );
    m_triangle_info.push_back( flags_a | cell_a );
    m_triangle_info.push_back( flags_b | cell_b );
    m_triangles.push_back( ix2 );
    m_triangles.push_back( ix3 );
    m_triangles.push_back( ix4 );

    flags_a = (is_fault   ? 1u<<31u : 0u ) |
            (edge_a_4_5 ? 1u<<29u : 0u ) |
            (edge_a_5_0 ? 1u<<30u : 0u );
    flags_b = (edge_b_4_5 ? 1u<<30u : 0u ) |
            (edge_b_5_0 ? 1u<<29u : 0u );
    m_triangle_info.push_back( flags_a | cell_a );
    m_triangle_info.push_back( flags_b | cell_b );
    m_triangles.push_back( ix4 );
    m_triangles.push_back( ix5 );
    m_triangles.push_back( ix0 );

    flags_a = (is_fault   ? 1u<<31u : 0u );
    flags_b = 0u;
    m_triangle_info.push_back( flags_a | cell_a );
    m_triangle_info.push_back( flags_b | cell_b );
    m_triangles.push_back( ix0 );
    m_triangles.push_back( ix2 );
    m_triangles.push_back( ix4 );
}

const GridTessBridge::REAL&
GridTessBridge::vertexX( const unsigned int index ) const {
    return m_vertices[ 4*index + 0 ];
}

const GridTessBridge::REAL&
GridTessBridge::vertexY( const unsigned int index ) const {
    return m_vertices[ 4*index + 1 ];
}

const GridTessBridge::REAL&
GridTessBridge::vertexZ( const unsigned int index ) const {
    return m_vertices[ 4*index + 2 ];
}

const size_t
GridTessBridge::triangleCount() const {
    return m_triangles.size()/3;
}

void
GridTessBridge::triangle( unsigned int& cell_a,
                          unsigned int& cell_b,
                          unsigned int& v0, unsigned int& v1, unsigned int& v2,
                          const unsigned int i )
{
    cell_a = m_triangle_info[2*i+0];
    if( cell_a != ~0u ) {
        cell_a &= 0xfffffffu;
    }
    cell_b = m_triangle_info[2*i+1];
    if( cell_b != ~0u ) {
        cell_b &= 0xfffffffu;
    }
    v0 = m_triangles[3*i+0];
    v1 = m_triangles[3*i+1];
    v2 = m_triangles[3*i+2];
}

