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
#include <list>
#include <boost/utility.hpp>
#include "Logger.hpp"
class GridTess;

class GridTessBridge : public boost::noncopyable
{
    friend class GridTess;
public:
    /** Basic floating-point type for triangulation. */
    typedef float Real;

    /** Basic index type for triangulation. */
    typedef unsigned int Index;

    /** Helper struct used for polygon tessellation.
      *
      * Everything is currently encoded as an uint32, where the two upper bits
      * are used to signal the presence of a cell boundary, used for line
      * drawing. Maximum vertex index is thus restricted to 2^30-1.
      */
    struct Segment {
        inline Segment() {}
        inline Segment( Index normal, Index vertex, Index flags )
            : m_normal( normal ), m_value( vertex | (flags<<30u) )
        {}

        //inline Segment( Index vertex ) : m_value( vertex ) {}
        //inline Segment( Index vertex, bool edge_a, bool edge_b )
        //    : m_value( vertex | (edge_a?(1u<<30u):0) | (edge_b?(1u<<31u):0) )
        //{}

        inline void clearEdges() { m_value = m_value & (~(3u<<30u)); }
        inline void setEdges(bool edge_a, bool edge_b ) { m_value = (m_value & (~(3u<<30u))) | (edge_a?(1u<<30u):0) | (edge_b?(1u<<31u):0); }
        inline Index normal() const { return m_normal; }
        inline Index vertex() const { return m_value & (~(3u<<30u)); }
        inline bool edgeA() const { return (m_value & (1u<<30u)) != 0u; }
        inline bool edgeB() const { return (m_value & (1u<<31u)) != 0u; }
        unsigned int    m_normal;
        unsigned int    m_value;
    };


    /** Type for describing points in R^3. */
    struct Real4 {
        inline Real4() {}
        inline Real4( const Real x, const Real y, const Real z, const Real w=1.f ) { v[0]=x; v[1]=y; v[2]=z; v[3]=w; }
        inline const Real& x() const { return v[0]; }
        inline const Real& y() const { return v[1]; }
        inline const Real& z() const { return v[2]; }
        inline const Real& w() const { return v[3]; }
        inline Real& x() { return v[0]; }
        inline Real& y() { return v[1]; }
        inline Real& z() { return v[2]; }
        inline Real& w() { return v[3]; }
        Real v[4];
    } __attribute__((aligned(16)));

    /** Enum to keep track of which logical face direction a polygon has. */
    enum Orientation {
        ORIENTATION_I = 0, ///< Polygon oriented in logical JK-plane.
        ORIENTATION_J = 1, ///< Polygon oriented in logical IK-plane.
        ORIENTATION_K = 2  ///< Polygon oriented in logical IJ-plane.
    };

    /** Describes the interface between two cells that a triangle is part of. */
    struct Interface
    {
        inline Interface() {}
        inline Interface( const Index cell_a,
                          const Index cell_b,
                          const Orientation orientation,
                          const bool fault = false )
        {   m_value[0] = cell_a | (fault?(1u<<31u):0 );
            m_value[1] = cell_b;
        }
        unsigned int    m_value[2] __attribute__((aligned(8)));
    };


    GridTessBridge( GridTess& owner );

    ~GridTessBridge();

    void
    reserveVertices( Index N );

    void
    reserveEdges( Index N );

    void
    reserveTriangles( Index N );

    void
    reserveCells( Index N );

    Index
    vertices() const;

    Index
    addVertex( const Real4 pos );

    Index
    addNormal( const Real4 dir );

    void
    addEdge( const Index ix0, const Index ix1,
             const Index cell_a, const Index cell_b,
             const Index cell_c, const Index cell_d )
    { }


    Index
    cellCount() const;

    void
    setCellCount( const Index N );


    void
    setCell( const Index index,
             const Index global_index,
             const Index vertex_0,
             const Index vertex_1,
             const Index vertex_2,
             const Index vertex_3,
             const Index vertex_4,
             const Index vertex_5,
             const Index vertex_6,
             const Index vertex_7 );

    void
    addTriangle( const Interface interface,
                 const Segment s0, const Segment s1, const Segment s2 );


    const Real4&
    vertex( const Index ix ) const
    { return m_vertices[ix]; }

    void
    boundingBox( Real4& minimum, Real4& maximum ) const;


    void
    process();

protected:
    GridTess&                   m_owner;
    std::vector<Real4>          m_vertices;
    std::vector<Real4>          m_normals;
    std::vector<unsigned int>   m_cell_index;
    std::vector<unsigned int>   m_cell_corner;

    static const Index          m_chunk_size = 10*1024;
    Index                       m_tri_N;
    Index                       m_tri_chunk_N;
    Index*                      m_tri_nrm_ix;
    Index*                      m_tri_info;
    Index*                      m_tri_vtx;
    std::list<Index*>           m_tri_nrm_ix_chunks;
    std::list<Index*>           m_tri_info_chunks;
    std::list<Index*>           m_tri_vtx_chunks;

    void
    allocTriangleChunks();

};
