/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once
#include <vector>
#include <boost/utility.hpp>

#include "Logger.hpp"
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

    struct Segment {
        Segment() {}

        Segment( unsigned int v, unsigned int flags )
            : m_v(v), m_flags(flags)
        {}
        unsigned int m_v;
        unsigned int m_flags;   // bit 0: edge on side a, bit1: edge on side b.
    };

    GridTessBridge( GridTess& owner );

    ~GridTessBridge();

    void
    reserveVertices( unsigned int N );

    void
    reserveEdges( unsigned int N );

    void
    reserveTriangles( unsigned int N );

    void
    reserveCells( unsigned int N );


    void
    addVertex( const float x, const float y, const float z, const float w = 1.f );

    void
    addEdge( const unsigned int ix0, const unsigned int ix1,
             const unsigned int cell_a, const unsigned cell_b,
             const unsigned int cell_c, const unsigned cell_d );

    unsigned int
    vertexCount() const;

    unsigned int
    cellCount() const;

    void
    setCellCount( unsigned int N );

    void
    addCell( const unsigned int global_index,
             const unsigned int vertex_0,
             const unsigned int vertex_1,
             const unsigned int vertex_2,
             const unsigned int vertex_3,
             const unsigned int vertex_4,
             const unsigned int vertex_5,
             const unsigned int vertex_6,
             const unsigned int vertex_7 );

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
             const unsigned int vertex_7 );

    void
    addTriangle( const Orientation orientation,
                 const bool is_fault,
                 const unsigned int cell_a,
                 const bool edge_a_0_1,
                 const bool edge_a_1_2,
                 const bool edge_a_2_0,
                 const unsigned cell_b,
                 const bool edge_b_0_1,
                 const bool edge_b_1_2,
                 const bool edge_b_2_0,
                 const unsigned int ix0, const unsigned ix1, const unsigned ix2 );

    void
    addPolygon(  const Orientation,
                 const bool          is_fault,
                 const unsigned int  cell_a,
                 const unsigned int  cell_b,
                 const Segment*      segments,
                 const unsigned int  N,
                 const unsigned int  split_hint = ~0u );



    void
    addQuadrilateral( const Orientation orientation,
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
                      const unsigned int ix3 );

    void
    addPentagon( const Orientation orientation,
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
                      const unsigned int ix4 );

    void
    addHexagon( const Orientation orientation,
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
                const unsigned int ix5 );

    const REAL&
    vertexX( const unsigned int index ) const;

    const REAL&
    vertexY( const unsigned int index ) const;

    const REAL&
    vertexZ( const unsigned int index ) const;

    const size_t
    triangleCount() const;

    void
    triangle( unsigned int& cell_a,
              unsigned int& cell_b,
              unsigned int& v0, unsigned int& v1, unsigned int& v2,
              const unsigned int i );

protected:
    GridTess&                  m_owner;
    std::vector<REAL>          m_vertices;
    std::vector<unsigned int>  m_triangles;
    std::vector<unsigned int>  m_triangle_info;
    std::vector<unsigned int>  m_cell_index;
    std::vector<unsigned int>  m_cell_corner;

};
