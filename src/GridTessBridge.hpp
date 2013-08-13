#pragma once
#include <vector>
#include <boost/utility.hpp>

class GridTess;

class GridTessBridge : public boost::noncopyable
{
    friend class GridTess;
public:
    typedef float REAL;
    enum Orientation {
        ORIENTATION_I = 0, // in JK-plane
        ORIENTATION_J = 1, // in IK-plane
        ORIENTATION_K = 2  // in IJ-plane
    };

    GridTessBridge( GridTess& owner );

    ~GridTessBridge();

    void
    reserveVertices( unsigned int N )
    {
        m_vertices.reserve( 4*N );
    }

    void
    reserveEdges( unsigned int N )
    {
    }

    void
    reserveTriangles( unsigned int N )
    {
        m_triangle_info.reserve( 3*N ); // ??
        m_triangles.reserve( 3*N );
    }

    void
    reserveCells( unsigned int N )
    {
        m_cell_index.resize( N );
        m_cell_corner.resize( 8*N );
    }


    void
    addVertex( const float x, const float y, const float z, const float w = 1.f )
    {
        m_vertices.push_back( x );
        m_vertices.push_back( y );
        m_vertices.push_back( z );
        m_vertices.push_back( w );
    }

    void
    addEdge( const unsigned int ix0, const unsigned int ix1,
             const unsigned int cell_a, const unsigned cell_b,
             const unsigned int cell_c, const unsigned cell_d )
    {
    }

    unsigned int
    vertexCount() const
    {
        return m_vertices.size()/4;
    }

    unsigned int
    cellCount() const
    {
        return m_cell_index.size();
    }

    void
    setCellCount( unsigned int N )
    {
        m_cell_index.resize( N );
        m_cell_corner.resize( 8*N );
    }

    void
    addCell( const unsigned int global_index,
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
    setCell( const unsigned int index,
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
    addTriangle( const Orientation orientation,
                 const unsigned int cell_a, const unsigned cell_b,
                 const unsigned int ix0, const unsigned ix1, const unsigned ix2 )
    {
        const unsigned int flags = (unsigned int)(orientation)<<29u;
        m_triangle_info.push_back( flags | cell_a );
        m_triangle_info.push_back( flags | cell_b );
        m_triangles.push_back( ix0 );
        m_triangles.push_back( ix1 );
        m_triangles.push_back( ix2 );
    }


    const REAL&
    vertexX( const unsigned int index ) const {
        return m_vertices[ 4*index + 0 ];
    }

    const REAL&
    vertexY( const unsigned int index ) const {
        return m_vertices[ 4*index + 1 ];
    }

    const REAL&
    vertexZ( const unsigned int index ) const {
        return m_vertices[ 4*index + 2 ];
    }

protected:
    GridTess&                  m_owner;
    std::vector<REAL>          m_vertices;
    std::vector<unsigned int>  m_triangles;
    std::vector<unsigned int>  m_triangle_info;
    std::vector<unsigned int>  m_cell_index;
    std::vector<unsigned int>  m_cell_corner;

};
