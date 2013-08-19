/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <math.h>
#include <algorithm>
#include <iterator>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "utils/Logger.hpp"
#include "GridTess.hpp"
#include "GridTessBridge.hpp"
#include "utils/PerfTimer.hpp"
#ifdef __SSE2__
#include <xmmintrin.h>
#endif
#ifdef __SSE4_1__
#include <smmintrin.h>
#endif
#ifdef CHECK_INVARIANTS
#include "CellSanityChecker.hpp"
#endif

namespace render {

static const std::string package = "GridTessBridge";

GridTessBridge::GridTessBridge( bool triangulate )
    : m_triangulate( triangulate ),
      m_tri_N( 0u ),
      m_tri_chunk_N( 0u ),
      m_tri_nrm_ix( NULL ),
      m_tri_info( NULL ),
      m_tri_vtx( NULL )
{
    allocTriangleChunks();
    m_polygon_offset.push_back( 0u );
}

void
GridTessBridge::allocTriangleChunks()
{
    m_tri_nrm_ix = new Index[ 3*(m_chunk_size+1) ];
    m_tri_nrm_ix_chunks.push_back( m_tri_nrm_ix );

    m_tri_info = new Index[ 2*(m_chunk_size+1) ];
    m_tri_info_chunks.push_back( m_tri_info );

    m_tri_vtx = new Index[ 3*(m_chunk_size+1) ];
    m_tri_vtx_chunks.push_back( m_tri_vtx );

    m_tri_chunk_N = 0u;
}

GridTessBridge::~GridTessBridge()
{
    for(auto it=m_tri_info_chunks.begin(); it!=m_tri_info_chunks.end(); ++it ) {
        delete *it;
    }
    for(auto it=m_tri_vtx_chunks.begin(); it!=m_tri_vtx_chunks.end(); ++it ) {
        delete *it;
    }
}

void
GridTessBridge::reserveVertices( unsigned int N )
{
    m_vertices.reserve( N );
}

void
GridTessBridge::reserveEdges( Index N )
{
}

void
GridTessBridge::reserveTriangles( Index N )
{
    //m_triangle_info.reserve( 3*N ); // ??
    //m_triangles.reserve( 3*N );
}

void
GridTessBridge::reserveCells( GridTessBridge::Index N )
{
    m_cell_index.resize( N );
    m_cell_corner.resize( 8*N );
}


GridTessBridge::Index
GridTessBridge::addVertex( const Real4 pos  )
{
    Index r = m_vertices.size();
    m_vertices.push_back( pos );
    return r;
}

GridTessBridge::Index
GridTessBridge::addNormal( const Real4 dir )
{
    Logger log = getLogger( package + ".addNormal" );

    Index i = m_normals.size();
//    LOGGER_DEBUG( log,  i << " = " << dir.x() << ", " << dir.y() << ", " << dir.z() << ", " << dir.w() );

    m_normals.push_back( dir );
    return i;
}


unsigned int
GridTessBridge::vertices() const
{
    return m_vertices.size();
}

unsigned int
GridTessBridge::cellCount() const
{
    return m_cell_index.size();
}

void
GridTessBridge::setCellCount( const GridTessBridge::Index N )
{
    m_cell_index.resize( N );
    m_cell_corner.resize( 8*N );
}


void
GridTessBridge::setCell( const GridTessBridge::Index index,
                         const GridTessBridge::Index global_index,
                         const GridTessBridge::Index vertex_0,
                         const GridTessBridge::Index vertex_1,
                         const GridTessBridge::Index vertex_2,
                         const GridTessBridge::Index vertex_3,
                         const GridTessBridge::Index vertex_4,
                         const GridTessBridge::Index vertex_5,
                         const GridTessBridge::Index vertex_6,
                         const GridTessBridge::Index vertex_7 )
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
dump8( const std::string& what, __m128i val )
{
    Logger log = getLogger( package + "dump8" );

    unsigned char foo[16];
    _mm_storeu_si128( (__m128i*)foo, val );

    LOGGER_DEBUG( log, what << ": "
                  << (unsigned int)(foo[0]) << " "
                  << (unsigned int)(foo[1]) << " "
                  << (unsigned int)(foo[2]) << " "
                  << (unsigned int)(foo[3]) << " | "
                  << (unsigned int)(foo[4]) << " "
                  << (unsigned int)(foo[5]) << " "
                  << (unsigned int)(foo[6]) << " "
                  << (unsigned int)(foo[7]) << " | "
                  << (unsigned int)(foo[8]) << " "
                  << (unsigned int)(foo[9]) << " "
                  << (unsigned int)(foo[10]) << " "
                  << (unsigned int)(foo[11]) << " | "
                  << (unsigned int)(foo[12]) << " "
                  << (unsigned int)(foo[13]) << " "
                  << (unsigned int)(foo[14]) << " "
                  << (unsigned int)(foo[15]) );

}

void
dump16( const std::string& what, __m128i val )
{
    Logger log = getLogger( package + "dump8" );

    unsigned short int foo[8];
    _mm_storeu_si128( (__m128i*)foo, val );

    LOGGER_DEBUG( log, what << ": "
                  << (unsigned int)(foo[0]) << " "
                  << (unsigned int)(foo[1]) << " |"
                  << (unsigned int)(foo[2]) << " "
                  << (unsigned int)(foo[3]) << " | "
                  << (unsigned int)(foo[4]) << " "
                  << (unsigned int)(foo[5]) << " | "
                  << (unsigned int)(foo[6]) << " "
                  << (unsigned int)(foo[7]) );

}


void
dump32( const std::string& what, __m128i val )
{
    Logger log = getLogger( package + "dump32" );

    unsigned int foo[4];
    _mm_storeu_si128( (__m128i*)foo, val );

    LOGGER_DEBUG( log, what << ": "
                  << (unsigned int)(foo[0]) << ","
                  << (unsigned int)(foo[1]) << ","
                  << (unsigned int)(foo[2]) << ","
                  << (unsigned int)(foo[3]) );

}


void
foo(  )
{
#if 0
    //  31      30   29   28
    // | fault | S1 | S0 | S2 |
    // |   x   | S0 | S1 | S2 |







    __m128i tt = _mm_castps_si128( _mm_blend_ps( _mm_castsi128_ps( t ),
                                                 _mm_castsi128_ps( _mm_slli_epi32(t,1) ),
                                                 12u ) );
    //  0 1 2 3 4 5 6 7 8 9 A B C D E F
    t = _mm_insert_epi16( zero, _mm_movemask_epi8( tt ), 0 );   // |c| | | | | | | | | | | | | | | |
    t = _mm_cvtsi32_si128( _mm_movemask_epi8( tt ) );



//    dump16( "tF", t );
//    dump32( "tG", t );




    unsigned int cells[4] = { 1, 2, ~0, ~0 };



    tt = _mm_loadl_epi64( (__m128i*)(&cells[0]) );  // = | cell[0] | cell[1] | 0 | 0 |

    tt = _mm_insert_epi16( zero, 258, 0 );
    dump32( "t0", tt );

    dump32( "t1", tt );


    int q = _mm_movemask_epi8( tt );
    LOGGER_DEBUG( log, "q=" << q );


    //    int buts = _mm_movemask_epi8( t );

//    indices = _mm_shuffle_epi8( indices, shuffleargs );


    __m128i flags_a = _mm_srli_epi32( indices, 31 );




    int bits_a = _mm_movemask_ps( _mm_castsi128_ps( _mm_shuffle_epi32( indices, _MM_SHUFFLE(3,1,0,2) ) ) );
    int bits_b = _mm_movemask_ps( _mm_castsi128_ps( _mm_shuffle_epi32(  _mm_slli_epi32( indices,1 ), _MM_SHUFFLE(3,0,1,2) ) ) );

    indices = _mm_slli_epi32( indices, 2 );
    indices = _mm_srli_epi32( indices, 2 );



    LOGGER_DEBUG( log, "mask: " <<  bits_a << ", " << bits_b );

#endif

}

void
GridTessBridge::addPolygon( const Interface interface,
                            const Segment* segments,
                            const Index no )
{
    uint N = no;
    Logger log = getLogger( package + ".addPolygon" );
    if( N < 3 ) {
        LOGGER_ERROR( log, "Got polygon with " << N << " vertices." );
        return;
    }

    uint cell_a = interface.m_value[0];
    uint cell_b = interface.m_value[1];

    std::vector<uint> vtx_ix( N );
    std::vector<uint> nrm_ix( N );
    std::vector<glm::vec3> pos(N);
    for(uint i=0; i<N; i++ ) {
        vtx_ix[i] = segments[i].vertex();
        nrm_ix[i] = segments[i].normal()
                  | (segments[i].edgeA() ? 0x80000000 : 0u)
                  | (segments[i].edgeB() ? 0x40000000 : 0u);
        pos[i] = glm::vec3( m_vertices[ vtx_ix[i] ].x(),
                            m_vertices[ vtx_ix[i] ].y(),
                            m_vertices[ vtx_ix[i] ].z() );
    }

    // compute approximate normal and barycenter
    // Newell's method (GPU gems III)
    glm::vec3 n(0.f);
    for(int i=0; i<N; i++) {
        glm::vec3 q = pos[i];
        glm::vec3 r = pos[(i+1)%N];
        n += glm::vec3( (q.y-r.y)*(q.z+r.z),
                        (q.z-r.z)*(q.x+r.x),
                        (q.x-r.x)*(q.y+r.y) );
    }

    // Check if polygon is convex
    bool convex = true;
    for( int i=0; i<N && convex; i++ ) {
        glm::vec3 q = pos[i];
        glm::vec3 r = pos[(i+2)%N];
        glm::vec3 b = r - q;
        glm::vec3 m = glm::normalize( glm::cross( b, n ) );
        float d = glm::dot( m, q );
        if( glm::dot( pos[(i+1)%N], m ) - d < std::numeric_limits<float>::epsilon() ) {
            convex = false;
        }
        for( int k=3; k<N && convex; k++) {
            if( glm::dot( pos[(i+k)%N], m ) - d > -std::numeric_limits<float>::epsilon() ) {
                convex = false;
            }
        }
    }
    if( convex ) {
        cell_a = cell_a | 0x40000000;
        cell_b = cell_b | 0x40000000;
    }

    if( !m_triangulate ) {
        m_polygon_info.push_back( cell_a );
        m_polygon_info.push_back( cell_b );
        std::copy_n( nrm_ix.begin(), N, std::back_inserter( m_polygon_nrm_ix ) );
        std::copy_n( vtx_ix.begin(), N, std::back_inserter( m_polygon_vtx_ix ) );
        m_polygon_offset.push_back( m_polygon_vtx_ix.size() );
    }
    else {

        uint s = 0;
        if( N > 3) {
            if( N > 4 ) {
                glm::vec3 b(0.f);
                for(int i=0; i<N; i++) {
                    b += pos[i];
                }
                b = (1.f/N)*b;
                glm::vec3 na = glm::abs(n);
                glm::vec3 m;
                if( (na.x < na.y) && (na.x < na.z) ) {
                    m = glm::vec3( 1.f, 0.f, 0.f );
                }
                else if( na.y < na.z ) {
                    m = glm::vec3( 0.f, 1.f, 0.f );
                }
                else {
                    m = glm::vec3( 0.f, 0.f, 1.f );
                }
                glm::vec3 u = glm::normalize( m - (glm::dot(m,n)/glm::dot(n,n))*n );
                glm::vec3 v = glm::normalize( glm::cross( n,u ) );
                // project to plane
                std::vector<glm::vec2> pos2( N );
                for( int i=0; i<N; i++ ) {
                    glm::vec3 t = pos[i]-b;
                    pos2[i] = glm::vec2( glm::dot(u, t), glm::dot(v, t) );
                }

                while( N > 4 ) {
                    glm::uvec3 ear(0,1,2);
                    // Search backwards so we minimize the amount data shift
                    for(uint i=N; i>0; i--) {
                        glm::uvec3 j( i-1, i%N, (i+1)%N );
                        glm::vec2 a = pos2[ j.x ];
                        glm::vec2 b = pos2[ j.y ];
                        glm::vec2 c = pos2[ j.z ];
                        glm::vec2 ab = b-a;
                        glm::vec2 ac = c-a;
                        float det = (ab.x*ac.y) - (ac.x*ab.y);
                        // Is current triangle a convex corner?
                        if( det > 1e-6 ) {
                            bool success = true;
                            // Is current triangle void of other points?
                            for(uint k=3; k<N; k++) {
                                uint l = (j.x+k)%N;
                                glm::vec2 p = pos2[ l ];

                                if( ((p.x-a.x)*ab.y) - (ab.x*(p.y-a.y)) > 0.f ) {   // ab
                                    if( ((p.x-c.x)*ac.y) - (ac.x*(p.y-c.y)) < 0.f ) {   // ca
                                        glm::vec2 bc = c-b;
                                        if( ((p.x-b.x)*bc.y) - (bc.x*(p.y-b.y)) > 0.f ) {   // bc
                                            success = false;
                                            break;
                                        }
                                    }
                                }
                            }
                            if( success ) {
                                ear = j;
                                break;
                            }
                        }
                    }

                    // emit ear
                    m_polygon_info.push_back( cell_a );
                    m_polygon_info.push_back( cell_b );
                    m_polygon_nrm_ix.push_back( nrm_ix[ ear.x ] );
                    m_polygon_nrm_ix.push_back( nrm_ix[ ear.y ] );
                    m_polygon_nrm_ix.push_back( nrm_ix[ ear.x ] & 0x3fffffffu );
                    m_polygon_vtx_ix.push_back( vtx_ix[ ear.x ] );
                    m_polygon_vtx_ix.push_back( vtx_ix[ ear.y ] );
                    m_polygon_vtx_ix.push_back( vtx_ix[ ear.z ] );
                    m_polygon_offset.push_back( m_polygon_vtx_ix.size() );

                    // Decrease polygon count and shift data
                    N--;
                    nrm_ix[ ear.x ] = nrm_ix[ ear.x ] & 0x3fffffffu;
                    for( uint k=ear.y; k<N; k++ ) {
                        nrm_ix[k] = nrm_ix[k+1];
                        vtx_ix[k] = vtx_ix[k+1];
                        pos2[k] = pos2[k+1];
                    }
                }
            }
            glm::vec3 ab = pos[1]-pos[0];
            glm::vec3 bc = pos[2]-pos[1];
            glm::vec3 cd = pos[3]-pos[2];
            glm::vec3 da = pos[0]-pos[3];
            float c0 = glm::dot( glm::cross( ab, bc ), glm::cross( cd, da ) );
            float c1 = glm::dot( glm::cross( bc, cd ), glm::cross( da, ab ) );
            if( c1 > c0 ) {
                s = 1;
            }
            // emit first triangle of quad
            m_polygon_info.push_back( cell_a );
            m_polygon_info.push_back( cell_b );
            m_polygon_nrm_ix.push_back( nrm_ix[ 2+s     ] );
            m_polygon_nrm_ix.push_back( nrm_ix[ (3+s)%4 ] );
            m_polygon_nrm_ix.push_back( nrm_ix[ 0+s     ]  & 0x3fffffffu );
            m_polygon_vtx_ix.push_back( vtx_ix[ 2+s     ] );
            m_polygon_vtx_ix.push_back( vtx_ix[ (3+s)%4 ] );
            m_polygon_vtx_ix.push_back( vtx_ix[ 0+s     ] );
            m_polygon_offset.push_back( m_polygon_vtx_ix.size() );

            nrm_ix[ 2+s ] = nrm_ix[ 2+s ] & 0x3fffffffu;
        }
        m_polygon_info.push_back( cell_a );
        m_polygon_info.push_back( cell_b );
        m_polygon_nrm_ix.push_back( nrm_ix[ 0+s ] );
        m_polygon_nrm_ix.push_back( nrm_ix[ 1+s ] );
        m_polygon_nrm_ix.push_back( nrm_ix[ 2+s ] );
        m_polygon_vtx_ix.push_back( vtx_ix[ 0+s ] );
        m_polygon_vtx_ix.push_back( vtx_ix[ 1+s ] );
        m_polygon_vtx_ix.push_back( vtx_ix[ 2+s ] );
        m_polygon_offset.push_back( m_polygon_vtx_ix.size() );
    }
}


void
GridTessBridge::addTriangle( const Interface interface,
                             const Segment s0, const Segment s1, const Segment s2 )
{
    Logger log = getLogger( package + ".foo" );
    if( m_tri_chunk_N == m_chunk_size ) {
        allocTriangleChunks();
    }
    unsigned int flags_a =
            (s0.edgeA() ? 1u<<30u : 0u ) |
            (s1.edgeA() ? 1u<<29u : 0u ) |
            (s2.edgeA() ? 1u<<28u : 0u );
    unsigned int flags_b =
            (s0.edgeB() ? 1u<<30u : 0u ) |
            (s1.edgeB() ? 1u<<29u : 0u ) |
            (s2.edgeB() ? 1u<<28u : 0u );

#if 0
    Segment segments[4] = { s0, s1, s2 };
    __m128i segs = _mm_loadu_si128( (__m128i*)segments );
    static const unsigned char shf_mem[16] __attribute__((aligned(16))) = {
            0x80, 0x80, 0x80, 0x80,
            0x0b, 0x03, 0x07, 0x80,
            0x80, 0x80, 0x80, 0x80,
            0x0b, 0x07, 0x03, 0x80,
    };
    __m128i indices = segs;                                 // | a0 a1 a2 a3 | b0 b1 b2 b3 | c0 c1 c2 c3 | 0  0  0  0  |
    __m128i shf = _mm_load_si128(  (__m128i*)shf_mem );     // | 80 80 80 80 | 3  7  b  80 | 80 80 80 80 | 3  7  b  80 |
    __m128i t = _mm_shuffle_epi8( indices, shf );           // | 0  0  0  0  | a3 b3 c3 0  | 0  0  0  0  | a3 b3 c3 0  |
    t = _mm_castps_si128( _mm_blend_ps( _mm_castsi128_ps( _mm_slli_epi32(t,1) ),
                                        _mm_castsi128_ps( t ),
                                        12u ) );
    //int mask = _mm_movemask_epi8( t );
    t = _mm_cvtsi32_si128( _mm_movemask_epi8( t ) );        // | Ma Mb 0  0  | 0  0  0  0  | 0  0  0  0  | 0  0  0  0  |
    __m128i zero = _mm_setzero_si128();                     // | 0  0  0  0  | 0  0  0  0  | 0  0  0  0  | 0  0  0  0  |
    t = _mm_unpacklo_epi8( zero, t );                       // | 0  Ma 0  Mb | 0  0  0  0  | 0  0  0  0  | 0  0  0  0  |
    t = _mm_unpacklo_epi16( zero, t );                      // | 0  0  0  Ma | 0  0  0  Mb | 0  0  0  0  | 0  0  0  0  |

    unsigned int tmp[4];
    _mm_store_si128( (__m128i*)tmp, t );
    if( tmp[0] != flags_a || tmp[1] != flags_b ) {
        LOGGER_DEBUG( log, "sse: " <<
                      ((tmp[0]&(1<<30u))!=0) << ", " <<
                      ((tmp[0]&(1<<29u))!=0) << ", " <<
                      ((tmp[0]&(1<<28u))!=0) << " ... " <<
                      ((tmp[1]&(1<<30u))!=0) << ", " <<
                      ((tmp[1]&(1<<29u))!=0) << ", " <<
                      ((tmp[1]&(1<<28u))!=0)
                      );
        LOGGER_DEBUG( log, "cpu: " <<
                      ((flags_a&(1<<30u))!=0) << ", " <<
                      ((flags_a&(1<<29u))!=0) << ", " <<
                      ((flags_a&(1<<28u))!=0) << " ... " <<
                      ((flags_b&(1<<30u))!=0) << ", " <<
                      ((flags_b&(1<<29u))!=0) << ", " <<
                      ((flags_b&(1<<28u))!=0)
                      );

    }
#endif
//    flags_a = flags_a | (is_fault   ? 1u<<31u : 0u );

    m_tri_nrm_ix[ 3*m_tri_chunk_N + 0 ] = s0.normal();
    m_tri_nrm_ix[ 3*m_tri_chunk_N + 1 ] = s1.normal();
    m_tri_nrm_ix[ 3*m_tri_chunk_N + 2 ] = s2.normal();

    //    LOGGER_DEBUG( log, s0.normal() << ", " << s1.normal() << ", " <<  s2.normal() );

    m_tri_info[ 2*m_tri_chunk_N + 0 ] = flags_a | interface.m_value[0];// cell_a;
    m_tri_info[ 2*m_tri_chunk_N + 1 ] = flags_b | interface.m_value[1];// cell_b;

    m_tri_vtx[ 3*m_tri_chunk_N + 0 ] = s0.vertex();
    m_tri_vtx[ 3*m_tri_chunk_N + 1 ] = s1.vertex();
    m_tri_vtx[ 3*m_tri_chunk_N + 2 ] = s2.vertex();

    m_tri_chunk_N++;
    m_tri_N++;
}


void
GridTessBridge::boundingBox( Real4& minimum, Real4& maximum ) const
{
    Logger log = getLogger( package + ".boundingBox" );
    LOGGER_DEBUG( log, "Calculating bounding box... " );
    PerfTimer start;
    Index N = m_vertices.size();
    if( N != 0 ) {
#ifdef __SSE2__
        const Real4* in = m_vertices.data();
        __m128 mi = _mm_load_ps( in->v );
        __m128 ma = mi;
        for( Index i=0; i<N; i++ ) {
            __m128 v = _mm_load_ps( ( in++ )->v );
            mi = _mm_min_ps( mi, v );
            ma = _mm_max_ps( ma, v );
        }
        _mm_store_ps( minimum.v, mi );
        _mm_store_ps( maximum.v, ma );
#else
        Real4 mi = m_vertices[0];
        Real4 ma = m_vertices[0];
        for(Index i=0; i<N; i++ ) {
            const Real4& v = m_vertices[i];
            mi.x() = std::min( mi.x(), v.x() );
            mi.y() = std::min( mi.y(), v.y() );
            mi.z() = std::min( mi.z(), v.z() );
            mi.w() = std::min( mi.w(), v.w() );
            ma.x() = std::max( ma.x(), v.x() );
            ma.y() = std::max( ma.y(), v.y() );
            ma.z() = std::max( ma.z(), v.z() );
            ma.w() = std::max( ma.w(), v.w() );
        }
        minimum = mi;
        maximum = ma;
#endif
    }
    PerfTimer stop;
    LOGGER_DEBUG( log, "Calculating bounding box... done (" << ((1000.0)*PerfTimer::delta( start, stop)) << "ms)" );
}

void
GridTessBridge::addEdge( const Index ix0, const Index ix1,
         const Index cell_a, const Index cell_b,
         const Index cell_c, const Index cell_d )
{
#ifndef OMIT_GEOMETRIC_EDGES
    Logger log = getLogger( package + ".addEdge" );

    // Make sure that edge is not degenerate
    LOGGER_INVARIANT_NOT_EQUAL( log, ix0, ix1 );
    // Make sure that we don't have the same cell on both sides of a pillar face
    if( (cell_a!= ~0u) && (cell_c != ~0u) ) {
        LOGGER_INVARIANT_NOT_EQUAL( log, cell_a, cell_c );
    }
    if( (cell_a!= ~0u) && (cell_d != ~0u) ) {
        LOGGER_INVARIANT_NOT_EQUAL( log, cell_a, cell_d );
    }
    if( (cell_a!= ~0u) && (cell_c != ~0u) ) {
        LOGGER_INVARIANT_NOT_EQUAL( log, cell_b, cell_c );
    }
    if( (cell_b!= ~0u) && (cell_d != ~0u) ) {
        LOGGER_INVARIANT_NOT_EQUAL( log, cell_b, cell_d );
    }

    m_edges.resize( m_edges.size() + 1 );
    Edge& e = m_edges.back();
    e.m_cp[0] = ix0;
    e.m_cp[1] = ix1;
    e.m_cells[0] = cell_a;
    e.m_cells[1] = cell_b;
    e.m_cells[2] = cell_c;
    e.m_cells[3] = cell_d;
#endif
}



void
GridTessBridge::process()
{
    Logger log = getLogger( package + ".process" );

#ifdef zCHECK_INVARIANTS
    std::vector<unsigned int> indices(3);
    std::vector<CellSanityChecker> cells( cellCount() );

    LOGGER_DEBUG( log, "Extracting triangle faces... " );
    Index N = m_triangles.size()/3;
    for( Index i=0; i<N; i++ ) {
        if( (i != 0u) && ((i%16384) == 0u) ) {
            LOGGER_DEBUG( log, "Extracting polygon faces... " << ((i*100)/N) << "%" );
        }
        Index cell_a = m_triangle_info[2*i+0];
        Index cell_b = m_triangle_info[2*i+1];
        indices[0] = m_triangles[3*i+0];
        indices[1] = m_triangles[3*i+1];
        indices[2] = m_triangles[3*i+2];
        if( cell_a != ~0u ) {
            cell_a = cell_a  & (~(15u<<28u));
            cells[cell_a].addPolygon( i, indices );
        }
        if( cell_b != ~0u ) {
            cell_b = cell_b  & (~(15u<<28u));
            cells[cell_b].addPolygonReverse( i, indices );
        }
    }
    LOGGER_DEBUG( log, "Extracting triangle faces... done." );

    N = cells.size();
    LOGGER_DEBUG( log, "Checking topology of each cell...")
    for( Index i=0; i<N; i++ ) {
        if( (i != 0u) && ((i%16384) == 0u) ) {
            LOGGER_DEBUG( log, "Checking topology of each cell... " << ((i*100)/N) << "%" );
        }
        cells[i].checkPolygonTopology();
    }
    LOGGER_DEBUG( log, "Checking topology of each cell... done.");
#endif

}

} // of namespace render
