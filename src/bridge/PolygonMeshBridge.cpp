/* Copyright STIFTELSEN SINTEF 2014
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

#ifdef __SSE2__
#include <xmmintrin.h>
#endif
#ifdef __SSE4_1__
#include <smmintrin.h>
#endif
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include "utils/Logger.hpp"
#include "utils/PerfTimer.hpp"
#include "bridge/PolygonMeshBridge.hpp"

namespace {
const std::string package = "bridge.PolygonMeshBridge";
}

namespace bridge {


PolygonMeshBridge::PolygonMeshBridge( bool triangulate )
    : m_triangulate( triangulate ),
      m_max_corners( 0 )
{
    m_polygon_offset.push_back( 0 );
    m_cell_offset.push_back( 0 );
    
}

PolygonMeshBridge::Index
PolygonMeshBridge::addVertex( const Real4 pos )
{
    Index rv = m_vertices.size();
    m_vertices.push_back( pos );
    return rv;
}

PolygonMeshBridge::Index
PolygonMeshBridge::addNormal( const Real4 dir )
{
    Index rv = m_normals.size();
    m_normals.push_back( dir );
    return rv;
}

PolygonMeshBridge::Index
PolygonMeshBridge::addCell()
{
    Index rv = m_cell_offset.size()-1;
    m_cell_offset.push_back( 0 );
    return rv;
}

PolygonMeshBridge::Index
PolygonMeshBridge::addPolygon( const Index cell,
                               const Segment* segments,
                               const Index N )
{
    Index rv = m_polygon_cell.size();

    m_max_corners = std::max( m_max_corners, N );
    for(Index i=0; i<N; i++ ) {
        m_polygon_vtx_ix.push_back( segments[i].vertex() );
        m_polygon_nrm_ix.push_back( segments[i].normal()
                                    | (segments[i].edgeA() ? 0x80000000 : 0u)
                                    | (segments[i].edgeB() ? 0x40000000 : 0u) );
    }
    m_polygon_cell.push_back( cell );
    m_polygon_offset.push_back( m_polygon_vtx_ix.size() );
    return rv;
}

void
PolygonMeshBridge::boundingBox( Real4& minimum, Real4& maximum ) const
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
PolygonMeshBridge::process()
{
    Logger log = getLogger( package + ".process" );

    // Determine the unique set of corners for each cell

    LOGGER_DEBUG( log, "Determining unique set of corners for each cell... " );
    PerfTimer start;
    std::vector< boost::tuple<Index,Index> > cell_corners;
    cell_corners.reserve( m_polygon_vtx_ix.size() );
    for(size_t p=0; p<m_polygon_cell.size(); p++ ) {
        for(Index o=m_polygon_offset[p]; o<m_polygon_offset[p+1]; o++ ) {
            cell_corners.push_back( boost::make_tuple<Index,Index>( m_polygon_cell[p],
                                                                    m_polygon_vtx_ix[o] ) );
        }
    }
    std::sort( cell_corners.begin(), cell_corners.end() );
    typename std::vector< boost::tuple<Index,Index> >::iterator it = std::unique( cell_corners.begin(), cell_corners.end() );
    cell_corners.resize( std::distance( cell_corners.begin(), it ) );

    m_cell_corners.resize( cell_corners.size() );
    size_t cell=0;
    for(size_t i=0; i<m_cell_corners.size(); i++ ) {
        m_cell_corners[i] = cell_corners[i].get<1>();
        if( cell_corners[i].get<0>() != cell ) {
            for(Index c=cell; c<cell_corners[i].get<0>(); c++ ) {
                m_cell_offset[c+1] = i;
            }
            cell = cell_corners[i].get<0>();
        }
    }
    for(Index c=cell+1; c<m_cell_offset.size(); c++ ) {
        m_cell_offset[c] = m_cell_corners.size();
    }
    PerfTimer stop;
    LOGGER_DEBUG( log, "Determining unique set of corners for each cell... done (" << ((1000.0)*PerfTimer::delta( start, stop)) << "ms)" );

    LOGGER_DEBUG( log,
                  m_vertices.size() << " vertices, " <<
                  m_normals.size() << " normals, " <<
                  m_polygon_vtx_ix.size() << " indices, " <<
                  m_polygon_cell.size() << " polygons, " <<
                  (m_cell_offset.size()-1) << " cells, " <<
                  m_max_corners << " is max corners."
                  );
}

} // of namespace bridge
