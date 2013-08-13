/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once

#include <GL/glew.h>
#include <boost/utility.hpp>

class GridTess;
class GridTessSubset;


/** Contains indices for a surface of a subset of a corner-point grid tessellation. */
class GridTessSurf : public boost::noncopyable
{
public:
    GridTessSurf();

    ~GridTessSurf();

    /** Returns true if surface has explicit geometric edges. */
    bool
    hasGeometricEdges() const { return m_edges.m_enabled; }

    void
    clearEdgeBuffer();

    /** Returns the buffer name to edge cornerpoint index buffer. */
    GLuint
    edgeCornerpointIndexBuffer() const { return m_edges.m_cornerpoint_index_buffer; }

    /** Returns the number of edges in surface. */
    GLsizei
    edgeCount() const { return m_edges.m_count; }

    /** Returns the texture name to buffer texture of cell index and normal index. */
    GLuint
    triangleCellTexture() const { return m_triangles.m_cell_texture; }

    /** Returns the buffer name to triangle cornerpoint index buffer. */
    GLuint
    triangleCornerpointIndexBuffer() const { return m_triangles.m_cornerpoint_index_buffer; }

    /** Returns the number of triangles in surface. */
    GLsizei
    triangleCount() const { return m_triangles.m_count; }


    /** Populate edge buffers using currently set shader.
     *
     * Processes all edges in tessellation using currently set shader, storing
     * emitted edges into edge buffer using transform feedback.
     *
     * The caller typically uses a geometry shader to only pass on geometry
     * that is part of the surface.
     *
     * - Input (point)
     *   - End-points as corner-point indices (uvec2, location 0).
     *   - Abutting cell indices (uvec4, location 1).
     * - Output (point)
     *   - End-point as corner-point indices (uvec2,
     *     location edge_indices_index).
     *
     * \param edge_indices_index  Transform feedback binding point index.
     */
    void
    populateEdgeBuffer( const GridTess*  tess,
                        GLuint           edge_indices_index = 0u );

    /** Populate triangle buffers using currently set shader.
     *
     * Processes all triangles in tessellation using currently set shader,
     * and stores emitted triangles into triangle buffer using transform
     * feedback.
     *
     * The caller typically uses a geometry shader to only pass on geometry
     * that is part of the surface.
     *
     * - Input (point)
     *   - Abutting cell indices (uvec2, location 0).
     *   - Corner-point indices (uvec3, location 1).
     *   - Normal vector indices (uvec3, location 2).
     * - Output (point)
     *   - Cell index and normal vector indices (x and yzw of uvec4,
     *     location tri_cell_index).
     *   - Corner-point indices (uvec3, location tri_indices_index).
     *
     * In and output to shader pipeline are points.
     *
     * \param tri_cell_index     Transform feedback binding point index.
     * \param tri_indices_index  Transform feedback binding point index.
     */
    void
    populateTriangleBuffer( const GridTess*        tess,
                            GLuint                 tri_cell_index    = 0u,
                            GLuint                 tri_indices_index = 1u );

    void
    populatePolygonBuffer( const GridTess*        tess,
                           GLsizei                N,
                            GLuint                 tri_cell_index    = 0u,
                            GLuint                 tri_indices_index = 1u );

protected:
    /** GL objects that hold triangle representation of surface. */
    struct {
        GLuint      m_cell_buffer;              ///< Buffer with cell id + normal vector ix (uvec4).
        GLuint      m_cell_texture;             ///< Buffer texture for m_cell_buffer.
        GLuint      m_cornerpoint_index_buffer; ///< Corner indices for triangles (uvec3).
        GLuint      m_count_query;              ///< Query object used to count primitives written.
        GLsizei     m_count;                    ///< The actual number of triangles in buffers.
        GLsizei     m_alloc;                    ///< The number of triangles that the buffer may hold.
    }           m_triangles;

    /** GL objects that hold outline edges of surface. */
    struct {
        bool        m_enabled;                  ///< True if geometric edges are enabled.
        GLuint      m_cornerpoint_index_buffer; ///< Buffer object with end-point indices.
        GLuint      m_count_query;              ///< Query object used to count primitives written.
        GLsizei     m_count;                    ///< The actual number of edges in buffer.
        GLsizei     m_alloc;                    ///< The number of edges that the buffer may hold.
    }           m_edges;

    void
    resizeTriangleBuffer( const GLsizei count );

    void
    resizeEdgeBuffer( const GLsizei count );

};
