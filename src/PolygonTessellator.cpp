#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include "Logger.hpp"
#include "PerfTimer.hpp"
#include "GridTessBridge.hpp"
#include "PolygonTessellator.hpp"
#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

#ifdef CHECK_INVARIANTS
#include "CellSanityChecker.hpp"
#endif

static const std::string package = "PolygonTessellator";

template<typename Triangulation>
PolygonTessellator<Triangulation>::PolygonTessellator( Triangulation& triangulation )
    : m_triangulation( triangulation )
{}


template<typename Triangulation>
void
PolygonTessellator<Triangulation>::addPolygon( const Interface  interface,
                                               const Segment*   segments,
                                               const Index      N )
{
    Polygon p;
    p.m_interface = interface,
    p.m_offset_0 = m_polygon_vertices.size();
    for( Index i=0; i<N; i++ ) {
        m_polygon_vertices.push_back( segments[i] );
    }
    p.m_offset_1 = m_polygon_vertices.size();
    m_polygon_info.push_back( p );
}



template<typename Triangulation>
void
PolygonTessellator<Triangulation>::triangulate()
{
    Logger log = getLogger( package + ".triangulate" );

    // Get bounding box for all vertices
    Real4 minimum;
    Real4 maximum;
    m_triangulation.boundingBox( minimum, maximum );

    LOGGER_DEBUG( log, "extent =[" <<
                  (maximum.x()-minimum.x() ) << ", " <<
                  (maximum.y()-minimum.y() ) << ", " <<
                  (maximum.z()-minimum.z() ) << "]");

    Real scale = std::max( maximum.x()-minimum.x(),
                           maximum.y()-minimum.y() ) / (maximum.z() - minimum.z() );


    Index N = m_polygon_info.size();

    PerfTimer start;

    const Real4* v = &m_triangulation.vertex(0);

    for( Index l=0; l<N; l++ ) {

        if( (l != 0u) && ((l%163840) == 0u) ) {
            LOGGER_DEBUG( log, "Triangulating polygons... " << ((l*100)/N) << "%" );
        }

        const Polygon& p = m_polygon_info[l];
        Index N = p.m_offset_1-p.m_offset_0;
        if( N == 3 ) {
            // Triangle
            m_triangulation.addTriangle( p.m_interface,
                                         m_polygon_vertices[ p.m_offset_0 + 0 ],
                                         m_polygon_vertices[ p.m_offset_0 + 1 ],
                                         m_polygon_vertices[ p.m_offset_0 + 2 ] );
        }
        else if( N < 3 ) {
        }
        else {
            std::vector<Segment> segments(N);
            std::vector<Real4>  positions( N );


            Real4 o = m_triangulation.vertex( m_polygon_vertices[ p.m_offset_0  ].vertex() );
            for( Index k=0; k<N; k++ ) {
                segments[k] = m_polygon_vertices[ p.m_offset_0 + k ];
                Real4 p = v[segments[k].vertex()];
                positions[k] = Real4( (p.x()-o.x()),
                                      (p.y()-o.y()),
                                      scale*(p.z()-o.z()));
            }

            while( segments.size() > 4 ) {
                Index N = segments.size();
                // Search for ear
                Index ear_i0 = 0;
                Real  ear_score = 0.f;
                bool candidate = false;
                for( Index i1=0; i1<N; i1++ ) {
                    Index i0 = (i1 + N - 1)%N;
                    Index i2 = (i1 + 1)%N;
#ifdef __SSE4_1__
#if 0
//                    Triangulating polygons... done (1.0137 secs)
                    __m128 _p0 = _mm_load_ps( positions[i0].v );
                    __m128 _p1 = _mm_load_ps( positions[i1].v );
                    __m128 _p2 = _mm_load_ps( positions[i2].v );
                    __m128 _v02 = _mm_sub_ps( _p2, _p0 );
                    __m128 _v01 = _mm_sub_ps( _p1, _p0 );

                    __m128 _v01_I_v02  = _mm_sub_ps( _v01,
                                                     _mm_mul_ps( _mm_div_ps( _mm_dp_ps( _v01, _v02, 0x7f ),
                                                                             _mm_dp_ps( _v02, _v02, 0x7f ) ),
                                                                 _v02 ) );



                    Real4 foo;
                    _mm_store_ps( foo.v, _v01_I_v02 );
                    LOGGER_DEBUG( log, "sse: " <<
                                   foo.v[0] << ", " <<
                                   foo.v[1] << ", " <<
                                   foo.v[2] << ", " <<
                                   foo.v[3] );

                    _v01_I_v02 = _mm_dp_ps( _v01, _v02, 0x71 );
                    _mm_store_ps( foo.v, _v01_I_v02 );
/*
                    LOGGER_DEBUG( log, "b " <<
                                   foo.v[0] << ", " <<
                                   foo.v[1] << ", " <<
                                   foo.v[2] << ", " <<
                                   foo.v[3] );
*/
                    //                            _mm_sub_ps( _v01,
//                                        _mm_dp_ps(_v01,v02,0x7)
//                                         )
 //                           v01 - (glm::dot(v01,v02)/glm::dot(v02,v02))*v02;

#endif
#endif

                    glm::vec3 p0 = glm::vec3( positions[ i0 ].x(), positions[ i0 ].y(), positions[ i0 ].z() );
                    glm::vec3 p1 = glm::vec3( positions[ i1 ].x(), positions[ i1 ].y(), positions[ i1 ].z() );
                    glm::vec3 p2 = glm::vec3( positions[ i2 ].x(), positions[ i2 ].y(), positions[ i2 ].z() );
                    glm::vec3 v02 = p2-p0;
                    glm::vec3 v01 = p1-p0;
                    glm::vec3 v01_I_v02 = v01 - (glm::dot(v01,v02)/glm::dot(v02,v02))*v02;
/*
                    LOGGER_DEBUG( log, "res: " <<
                                  v01_I_v02.x << ", " <<
                                  v01_I_v02.y << ", " <<
                                  v01_I_v02.z );
*/
                    Real v01_I_v02_l2 = glm::dot( v01_I_v02, v01_I_v02 );
                    if( v01_I_v02_l2 > std::numeric_limits<Real>::epsilon() ) {
                        candidate = true;
                        glm::vec3 n = (1.f/sqrtf(v01_I_v02_l2))*v01_I_v02;

                        // Determine circumcircle for the three points
                        glm::vec3 a = p1-p0;
                        glm::vec3 b = p2-p0;
                        float den = 2.f*fabsf( glm::dot(a,a)*glm::dot(b,b) - glm::dot(a,b)*glm::dot(a,b) );
                        glm::vec3 t = glm::dot(a,a)*b - glm::dot(b,b)*a;
                        glm::vec3 p = (1.f/den)*( glm::dot(t,b)*a - glm::dot(t,a)*b ) + p0;
                        Real d = glm::dot( p0, n );
                        Real R = std::numeric_limits<Real>::max();
                        for( Index k=3; k<N; k++ ) {
                            Index ik = (i0+k)%N;
                            glm::vec3 pk = glm::vec3( positions[ik].x(), positions[ik].y(),positions[ik].z() );
                            Real val = glm::dot( pk, n ) - d;
                            if( val < -std::numeric_limits<Real>::epsilon() ) {
                                Real r = glm::dot( pk-p,pk-p );
                                if( r < R ) {
                                    R = r;
                                }
                            }
                            else {
                                R = 0.f;
                            }
                        }
                        if( R > ear_score ) {
                            ear_score = R;
                            ear_i0 = i0;
                        }
                    }
                }
                if( !candidate ) {
                    LOGGER_ERROR( log, "Failed to find ear." );
                }
                Index ear_i1 = (ear_i0+1)%N;
                Index ear_i2 = (ear_i0+2)%N;
                Segment d = Segment( segments[ear_i2].vertex() );
                m_triangulation.addTriangle( p.m_interface,
                                             segments[ear_i0],
                                             segments[ear_i1],
                                             d );
                segments[ ear_i0 ].clearEdges();
                for( Index k=ear_i1+1; k<N; k++ ) {
                    segments[k-1] = segments[k];
                    positions[k-1] = positions[k];
                }
                segments.pop_back();
            }

            // Quadrilateral
            glm::vec3 v0 = glm::vec3( positions[0].x(), positions[0].y(),positions[0].z() );
            glm::vec3 v1 = glm::vec3( positions[0].x(), positions[0].y(),positions[0].z() );
            glm::vec3 v2 = glm::vec3( positions[0].x(), positions[0].y(),positions[0].z() );
            glm::vec3 v3 = glm::vec3( positions[0].x(), positions[0].y(),positions[0].z() );

            glm::vec3 n0 = glm::cross( v1-v0, v0-v3 );
            glm::vec3 n1 = glm::cross( v2-v1, v1-v0 );
            glm::vec3 n2 = glm::cross( v3-v2, v2-v1 );
            glm::vec3 n3 = glm::cross( v0-v3, v3-v2 );
            Index shift = 0;
            if( glm::dot(n0,n2) > glm::dot(n1,n3) ) {
                shift = 1;
            }
            Segment da =  Segment( segments[ shift + 2 ].vertex() );
            Segment db =  Segment( segments[ shift + 0 ].vertex() );
            m_triangulation.addTriangle( p.m_interface,
                                         segments[ shift + 0 ],
                                         segments[ shift + 1 ], da );
            m_triangulation.addTriangle( p.m_interface,
                                         segments[ shift + 2 ],
                                         segments[ (shift + 3)%4 ],
                                         db );
        }
    }
    PerfTimer stop;
    double t = PerfTimer::delta(start, stop);
    LOGGER_DEBUG( log, "Triangulating polygons... done (" << t << " secs)" );

}

template<typename Triangulation>
void
PolygonTessellator<Triangulation>::process()
{
    Logger log = getLogger( package + ".process" );

    triangulate();

#ifdef zCHECK_INVARIANTS
    std::vector<unsigned int> indices;
    std::vector<CellSanityChecker> cells( cellCount() );

    LOGGER_DEBUG( log, "Extracting polygon faces... " );
    for( Index i=0; i<m_polygon_info.size(); i++ ) {
        if( (i != 0u) && ((i%16384) == 0u) ) {
            LOGGER_DEBUG( log, "Extracting polygon faces... " << ((i*100)/m_polygon_info.size()) << "%" );
        }
        indices.clear();
        for( Index k=m_polygon_info[i].m_offset_0; k<m_polygon_info[i].m_offset_1; k++ ) {
            indices.push_back( m_polygon_vertices[k].vertex() );
        }
        if( m_polygon_info[i].m_cell_a != ~0u ) {
            cells[ m_polygon_info[i].m_cell_a ].addPolygon( i, indices );
        }
        if( m_polygon_info[i].m_cell_b != ~0u ) {
            cells[ m_polygon_info[i].m_cell_b ].addPolygonReverse( i, indices );
        }
    }
    LOGGER_DEBUG( log, "Extracting polygon faces... done." );

    LOGGER_DEBUG( log, "Checking topology of each cell...")
    for( Index i=0u; i<cells.size(); i++ ) {
        if( (i != 0u) && ((i%16384) == 0u) ) {
            LOGGER_DEBUG( log, "Checking topology of each cell... " << ((i*100)/cells.size()) << "%" );
        }
        cells[i].checkTriangleTopology();
        cells[i].checkPolygonTopology();
    }
    LOGGER_DEBUG( log, "Checking topology of each cell... done.");
#endif
    m_triangulation.process();
}


template class PolygonTessellator<GridTessBridge>;
