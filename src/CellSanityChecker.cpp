#include <algorithm>
#include "Logger.hpp"
#include "CellSanityChecker.hpp"

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
CellSanityChecker::checkTopology()
{
    Logger log = getLogger( "CellSanityChecker.checkTopology" );

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
