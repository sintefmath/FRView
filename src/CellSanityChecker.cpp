#include <algorithm>
#include "Logger.hpp"
#include "CellSanityChecker.hpp"


CellSanityChecker::CellSanityChecker()
{
    m_polygon_offsets.push_back( 0 );
}

void
CellSanityChecker::addPolygon( unsigned int id,
                               const std::vector<unsigned int>& vertices )
{
    for( unsigned int i=0; i<vertices.size(); i++ ) {
        m_polygon_vertices.push_back( vertices[i] );
    }
    m_polygon_offsets.push_back( m_polygon_vertices.size() );
}

void
CellSanityChecker::addPolygonReverse( unsigned int id,
                                      const std::vector<unsigned int>& vertices )
{
    unsigned int l = vertices.size()-1;
    for( unsigned int i=0; i<vertices.size(); i++ ) {
        m_polygon_vertices.push_back( vertices[l-i] );
    }
    m_polygon_offsets.push_back( m_polygon_vertices.size() );
}


void
CellSanityChecker::addTriangle( unsigned int id,
                                unsigned int v0, unsigned int v1, unsigned int v2 )
{
    m_triangles.push_back( id );
    m_triangles.push_back( v0 );
    m_triangles.push_back( v1 );
    m_triangles.push_back( v2 );
}


struct Edge
{
    Edge( unsigned int a, unsigned int b )
    {
        if( a < b ) {
            m_flipped = false;
            m_a = a;
            m_b = b;
        }
        else {
            m_flipped = true;
            m_a = b;
            m_b = a;
        }
    }

    friend
    bool
    operator<( const Edge& a, const Edge& b )
    {
        if( a.m_a == b.m_a ) {
            return a.m_b < b.m_b;
        }
        return a.m_a < b.m_a;
    }

    bool            m_flipped;
    unsigned int    m_a;
    unsigned int    m_b;
};

bool
CellSanityChecker::checkPolygonTopology()
{
    Logger log = getLogger( "CellSanityChecker.checkPolygonTopology" );

    std::vector<Edge> edges;
    for( unsigned int i=1; i<m_polygon_offsets.size();  i++) {
        unsigned int a = m_polygon_offsets[i-1];
        unsigned int b = m_polygon_offsets[i];
        unsigned int N = b-a;
        for(unsigned int i=0; i<N; i++) {
            unsigned int v0 = m_polygon_vertices[ a+i ];
            unsigned int v1 = m_polygon_vertices[ a+( (i+1)%N ) ];
            if( v0 == v1 ) {
                LOGGER_ERROR( log, "Degenerate edge [" << v0 << ", " << v1 << "]" );
            }
            else {
                edges.push_back( Edge( v0, v1 ) );
            }
        }
    }

    std::sort( edges.begin(), edges.end() );

    unsigned int boundary_edges = 0;
    unsigned int manifold_edges = 0;
    unsigned int nonmanifold_edges = 0;

    for(unsigned int j=0; j<edges.size(); j++ ) {
        unsigned int i;
        for(i=j; i<edges.size()-1; i++) {
            if( edges[i].m_a != edges[i+1].m_a || edges[i].m_b != edges[i+1].m_b ) {
                break;
            }
        }
        int abutting = (i-j)+1;
        if( abutting == 1 ) {
            boundary_edges++;
        }
        else if( abutting == 2 ) {
            manifold_edges++;
        }
        else {
            LOGGER_ERROR( log, "non-manifold: " << edges[i].m_a << ", " << edges[i].m_b );
            nonmanifold_edges++;
        }
        j = i;
    }

    if( boundary_edges != 0 ) {
        LOGGER_ERROR( log, boundary_edges << " boundary edges." );
    }
    if( nonmanifold_edges != 0 ) {
        LOGGER_ERROR( log, nonmanifold_edges << " non-manifold edges." );
    }

    /*
    if( boundary_edges != 0 || nonmanifold_edges != 0 ) {
        for( unsigned int i=0; i<m_triangles.size()/4; i++ ) {
            LOGGER_DEBUG( log,
                          "T: " << m_triangles[4*i+1]
                          << ", " << m_triangles[4*i+2]
                          << ", " << m_triangles[4*i+3] );
        }
        for( unsigned int i=0; i<edges.size(); i++ ) {
            LOGGER_DEBUG( log,
                          "E: " << edges[i].m_a
                          << ", " << edges[i].m_b );
        }
    }
*/
    //    LOGGER_DEBUG( log, manifold_edges << " manifold edges" );


    return true;
}

bool
CellSanityChecker::checkTriangleTopology()
{
    Logger log = getLogger( "CellSanityChecker.checkTriangleTopology" );

    std::vector<Edge> edges;
    for( unsigned int i=0; i<m_triangles.size()/4; i++ ) {
        for( unsigned int k=0; k<3; k++ ) {
            unsigned int a = m_triangles[ 4*i+1+k];
            unsigned int b = m_triangles[ 4*i+1+((k+1)%3)];
            if( a == b ) {
                LOGGER_ERROR( log, "Degenerate edge: " << a << ", " << b );
            }
            else {
                edges.push_back( Edge( a, b ) );
            }
        }
    }

    std::sort( edges.begin(), edges.end() );


    unsigned int boundary_edges = 0;
    unsigned int manifold_edges = 0;
    unsigned int nonmanifold_edges = 0;

    for(unsigned int j=0; j<edges.size(); j++ ) {
        unsigned int i;
        for(i=j; i<edges.size()-1; i++) {
            if( edges[i].m_a != edges[i+1].m_a || edges[i].m_b != edges[i+1].m_b ) {
                break;
            }
        }
        int abutting = (i-j)+1;
        if( abutting == 1 ) {
            boundary_edges++;
        }
        else if( abutting == 2 ) {
            manifold_edges++;
        }
        else {
            LOGGER_ERROR( log, "non-manifold: " << edges[i].m_a << ", " << edges[i].m_b );
            nonmanifold_edges++;
        }
        j = i;
    }

    if( boundary_edges != 0 ) {
        LOGGER_ERROR( log, boundary_edges << " boundary edges." );
    }
    if( nonmanifold_edges != 0 ) {
        LOGGER_ERROR( log, nonmanifold_edges << " non-manifold edges." );
    }

    /*
    if( boundary_edges != 0 || nonmanifold_edges != 0 ) {
        for( unsigned int i=0; i<m_triangles.size()/4; i++ ) {
            LOGGER_DEBUG( log,
                          "T: " << m_triangles[4*i+1]
                          << ", " << m_triangles[4*i+2]
                          << ", " << m_triangles[4*i+3] );
        }
        for( unsigned int i=0; i<edges.size(); i++ ) {
            LOGGER_DEBUG( log,
                          "E: " << edges[i].m_a
                          << ", " << edges[i].m_b );
        }
    }
*/
    //    LOGGER_DEBUG( log, manifold_edges << " manifold edges" );


    return true;
}
