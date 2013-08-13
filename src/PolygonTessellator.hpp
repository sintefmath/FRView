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

template<typename Triangulation>
class PolygonTessellator
{
public:
    typedef float                                SrcReal;
    typedef typename Triangulation::Real         Real;
    typedef typename Triangulation::Index        Index;
    typedef typename Triangulation::Real4        Real4;
    typedef typename Triangulation::Orientation  Orientation;
    typedef typename Triangulation::Segment      Segment;
    typedef typename Triangulation::Interface    Interface;
    static const Orientation ORIENTATION_I = Triangulation::ORIENTATION_I;
    static const Orientation ORIENTATION_J = Triangulation::ORIENTATION_J;
    static const Orientation ORIENTATION_K = Triangulation::ORIENTATION_K;



    PolygonTessellator( Triangulation& triangulation );


    void
    addPolygon( const Interface  interface,
                const Segment*   segments,
                const Index      N );

    /** Convert all polygon loops into triangles.
      *
      * Converts all polygon loops into trianges, storing the triangles in the
      * attached triangulation.
      */

    void
    triangulate();

    void
    process();


    inline void reserveVertices( Index N ) { m_triangulation.reserveVertices(N); }
    inline void reserveEdges( Index N ) { m_triangulation.reserveEdges(N); }
    inline void reserveTriangles( Index N ) { m_triangulation.reserveTriangles( N ); }
    inline void reserveCells( Index N ) { m_triangulation.reserveCells(N); }
    inline Index vertices() const { return m_triangulation.vertices(); }
    Index addVertex( const Real4 pos ) { return m_triangulation.addVertex( pos ); }
    void addEdge( const Index ix0, const Index ix1, const Index cell_a, const Index cell_b, const Index cell_c, const Index cell_d ) { m_triangulation.addEdge( ix0, ix1, cell_a, cell_b, cell_c, cell_d ); }
    Index cellCount() const { return m_triangulation.cellCount(); }
    inline void setCellCount( const Index N ) { m_triangulation.setCellCount( N ); }
    inline const Real4& vertex( const Index ix ) const { return m_triangulation.vertex( ix ); }
    void boundingBox( Real4& minimum, Real4& maximum ) const { m_triangulation.boundingBox( minimum, maximum ); }


    inline void
    setCell( const Index index,
             const Index global_index,
             const Index vertex_0,
             const Index vertex_1,
             const Index vertex_2,
             const Index vertex_3,
             const Index vertex_4,
             const Index vertex_5,
             const Index vertex_6,
             const Index vertex_7 )
    {
        m_triangulation.setCell( index,
                                 global_index,
                                 vertex_0,
                                 vertex_1,
                                 vertex_2,
                                 vertex_3,
                                 vertex_4,
                                 vertex_5,
                                 vertex_6,
                                 vertex_7 );
    }






protected:
    struct Polygon
    {
        Interface       m_interface;
        Index           m_offset_0;
        Index           m_offset_1;
    };

    Triangulation&          m_triangulation;
    std::vector<Polygon>    m_polygon_info;
    std::vector<Segment>    m_polygon_vertices;

};
