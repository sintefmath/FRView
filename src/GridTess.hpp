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
#include <glm/glm.hpp>
#include <vector>
#include <boost/utility.hpp>

class GridField;
class CellSelector;
class GridTessBridge;
class GridTessSubset;

/** GPU representation of a polyhedral mesh.
 *
 * This representation includes all (triangulated) interfaces between cells. It
 * isn't rendered directly, \ref GridTessSurf represents a renderable subset of
 * all these triangles.
 *
 * It contains the following structures:
 * - A set of vertex positions, as XYZW coordinates (W=1 only to make them
 *   4-component).
 * - A set of independently indexed normal vectors, as 4-component vectors (W=1
 *   only to make them 4-component).
 * - A set of cells, where we have:
 *   - The global index
 *   - The index of the eight vertices spanning the cell (this representation is
 *     likely to change in order to accommodate cells of more than eight
 *     vertices).
 * - A set of triangles that forms the interfaces between cells. A triangle has
 *   exactly one or two cells adjacent (i.e., faults have been tessellated to
 *   accommodate this property). A triangle is described by:
 *   - Three vertex indices.
 *   - Three normal vector indices.
 *   - Two cell indices, one for each side of the triangle:
 *     - Bits 0..27 encode the cell index.
 *     - Bits 28,29,30 encode if edge [v0,v1], [v1,v2] and/or [v2,v0] should
 *       produce a outline (the outline consists only of edges corresponding to
 *       original cell edges. Edges produced by fault-splitting or triangulation
 *       do not have their bits set). Thus, the outline is depending on which
 *       side of the triangle that is rendered.
 *     - Bit 31 encode if the triangle is part of a fault surface (true) or not
 *       (false).
 * - (expermiental) A set of edges used for explicit edge rendering:
 *   - Two vertex indices.
 *   - Four cell indices.
 *
 */
class GridTess : public boost::noncopyable
{
public:
    GridTess();

    /** Check the topology (Euler number and friends) for each cell individually.
     *
     * Mainly used for tracking down bugs.
     */
    void
    checkTopology() const;

    // -------------------------------------------------------------------------
    /** @{ */
    /** Get the number of vertices in the grid. */
    GLsizei
    vertexCount() const { return m_vertices_num; }

    /** Get 4-component vertex positions through a vertex array object with positions at index 0. */
    GLuint
    vertexPositonsAsVertexArrayObject() const { return m_vertex_positions_vao;  }

    /** Get 4-component vertex positions through a GL_RGBA32F buffer texture. */
    GLuint
    vertexPositionsAsBufferTexture() const { return m_vertex_positions_tex; }

    /** Get 4-component vertex positions stored in host memory. */
    const std::vector<float>&
    vertexPositionsInHostMemory() const { return m_vertex_positions_host; }

    /** @} */
    // -------------------------------------------------------------------------
    /** @{ */
    /** Get 4-component normal vectors through a GL_RGBA32F buffer texture. */
    GLuint
    normalVectorsAsBufferTexture() const { return m_normal_vectors_tex; }

    /** Get 4-component normal vectors stored in host memory. */
    const std::vector<float>&
    normalVectorsInHostMemory() const { return m_normal_vectors_host; }
    /** @} */
    // -------------------------------------------------------------------------
    /** @{ */

    /** Get the number of cells in the grid. */
    GLsizei
    cellCount() const { return m_cell_global_index_host.size(); }

    /** Get global cell index through GL_R32UI buffer texture. */
    GLuint
    cellGlobalIndexTexture() const { return m_cell_global_index_tex; }

    /** Get the vertex indices spanning a cell through a GL_RGBA32UI buffer texture, two uvec4 per cell. */
    GLuint
    cellCornerTexture() const { return m_cell_vertex_indices_tex; }

    /** Get global cell index stored in host memory. */
    const std::vector<GLuint>&
    cellGlobalIndicesInHostMemory() const { return m_cell_global_index_host; }

    /** @} */
    // -------------------------------------------------------------------------
    /** @{ */

    /** Get the number of triangles in the grid. */
    GLsizei
    triangleCount() const { return m_triangles_num; }

    /** Vertex array object with triangle indices.
     *
     * - location 0: Abutting cell indices (uvec2).
     * - location 1: Triangle corner corner-point indices (uvec3).
     * - location 2: Triangle normal vector indices (uvec3).
     */
    GLuint
    triangleIndexTupleVertexArray() const { return m_triangle_index_tuple_vao; }

    /** Get the triangle cell indices through a GL_RG32UI buffer texture, one uvec2 per triangle. */
    GLuint
    triangleCellIndicesAsBufferTexture() const { return m_triangle_cell_indices_tex; }

    GLuint
    triangleVertexIndexBuffer() const { return m_triangle_vertex_indices_buf; }

    const std::vector<GLuint>&
    triangleVertexIndexInHostMemory() const { return m_triangle_vertex_indices_host; }

    const std::vector<GLuint>&
    triangleNormalIndexInHostMemory() const { return m_triangle_normal_indices_host; }

    const std::vector<GLuint>&
    triangleCellIndexInHostMemory() const { return m_triangle_cell_indices_host; }
    /** @} */
    // -------------------------------------------------------------------------
    /** @{ */
    /** Get the minimum corner of the axis-aligned bounding box of corner-points. */
    const float*
    minBBox() const { return m_bb_min; }

    /** Get the maximum corner of the axis-aligned bounding box of corner-points. */
    const float*
    maxBBox() const { return m_bb_max; }

    /** Get the scale of the XY-aspect-ratio preserving transform from actual positions to the unit cube. */
    const float*
    scale() const { return m_scale; }

    /** Get the shift of the XY-aspect-ratio preserving transform from actual positions to the unit cube. */
    const float*
    shift() const { return m_shift; }
    /** @} */
    // -------------------------------------------------------------------------
    /** @{ */
    GLuint
    edgeVertexIndexBuffer() const
    { return m_edge_vertex_indices_buf; }

    GLuint
    edgeIndexTupleVertexArray() const
    { return m_edge_index_tuple_vao; }

    GLsizei
    edgeCount() const
    { return m_edges_num; }
    /** @} */

    GLuint
    polygonVertexArray() const
    { return m_polygon_vao; }

    GLuint
    polygonVertexIndexTexture() const
    { return m_polygon_vtx_tex; }

    GLuint
    polygonNormalIndexTexture() const
    { return m_polygon_nrm_tex; }

    GLsizei
    polygonCount() const
    { return m_polygons_N; }

    GLsizei
    polygonMaxPolygonSize() const
    { return m_polygon_max_n; }

    /** Pull data from bridge. */
    void
    update( GridTessBridge& bridge );

protected:
    /** @{ */
    float                   m_bb_min[3];                    ///< AABB min corner.
    float                   m_bb_max[3];                    ///< AABB max corner.
    float                   m_scale[3];                     ///< Scale part of XY-aspect-preserving map to unit cube.
    float                   m_shift[3];                     ///< Shift part of XY-aspect-preserving map to unit cube.
    /** @} */
    /** @{ */
    GLsizei                 m_vertices_num;                 ///< Number of vertices in grid.
    std::vector<GLfloat>    m_vertex_positions_host;        ///< Host copy of vertex positions.
    GLuint                  m_vertex_positions_buf;         ///< Buffer object with vertex positions.
    GLuint                  m_vertex_positions_tex;         ///< Texture sampling \ref m_vertex_positions_buf.
    GLuint                  m_vertex_positions_vao;         ///< Vertex array object streaming location 0 from \ref m_vertex_positions_buf.
    /** @} */
    /** @{ */
    GLsizei                 m_normals_num;                  ///< Number of normal vectors in grid.
    std::vector<GLfloat>    m_normal_vectors_host;          ///< Host copy of normal vectors.
    GLuint                  m_normal_vectors_buf;           ///< Buffer object with normal vectors.
    GLuint                  m_normal_vectors_tex;           ///< Texture sampling \ref m_normal_vectors_buf.
    /** @} */
    /** @{ */
    GLsizei                 m_cells_num;                    ///< Number of cells.
    std::vector<GLuint>     m_cell_global_index_host;       ///< Host copy of global cell indices.
    GLuint                  m_cell_global_index_buf;        ///< Buffer object with global cell indices.
    GLuint                  m_cell_global_index_tex;        ///< Texture sampling \ref m_cell_global_index_buf.
    std::vector<GLuint>     m_cell_vertex_indices_host;     ///< Host copy of cell vertex indices.
    GLuint                  m_cell_vertex_indices_buf;      ///< Buffer object with cell vertex indices.
    GLuint                  m_cell_vertex_indices_tex;      ///< Texture sampling \ref m_cell_vertex_indices_buf.
    /** @} */
    /** @{ */
    GLsizei                 m_triangles_num;                ///< Number of triangles.
    GLuint                  m_triangle_index_tuple_vao;     ///< Vertex array object with cell, vertex and normal indices as inputs 0 through 2.
    GLuint                  m_triangle_vertex_indices_buf;  ///< Buffer object with triangle vertex indices (3 per triangle).
    std::vector<GLuint>     m_triangle_vertex_indices_host; ///< Host copy of \ref m_triangle_vertex_indices_buf.
    GLuint                  m_triangle_normal_indices_buf;  ///< Buffer object with triangle normal vector indices (3 per triangle).
    std::vector<GLuint>     m_triangle_normal_indices_host; ///< Host copy of \ref m_triangle_normal_indices_buf.
    GLuint                  m_triangle_normal_indices_tex;  ///< Texture sampling \ref m_triangle_normal_indices_buf.
    GLuint                  m_triangle_cell_indices_buf;    ///< Buffer object with tagged triangle cell indices (2 per triangle).
    std::vector<GLuint>     m_triangle_cell_indices_host;   ///< Host copy of \ref m_triangle_cell_indices_buf.
    GLuint                  m_triangle_cell_indices_tex;    ///< Texture sampling \ref m_triangle_cell_indices_buf.
    /** @} */
    /** @{ */
    GLsizei                 m_edges_num;                    ///< Number of edges.
    GLuint                  m_edge_index_tuple_vao;         ///< Vertex array object with vertex (uvec2) and cell index data (uvec4) as inputs 0 and 1.
    GLuint                  m_edge_vertex_indices_buf;      ///< Buffer object with uint data (2 per edge).
    GLuint                  m_edge_cell_indices_buf;        ///< Buffer object with edge cell indices (4 per edge).
    GLuint                  m_edge_cell_indices_tex;        ///< Texture sampling \ref m_edge_cell_indices_buf.
    /** @} */

    GLuint                  m_polygon_vao;
    GLsizei                 m_polygons_N;
    GLsizei                 m_polygon_max_n;
    GLuint                  m_polygon_info_buf;
    GLuint                  m_polygon_offset_buf;
    GLuint                  m_polygon_vtx_buf;
    GLuint                  m_polygon_vtx_tex;
    GLuint                  m_polygon_nrm_buf;
    GLuint                  m_polygon_nrm_tex;


    /** Pull vertex data from bridge. */
    void
    updateVertices( GridTessBridge& bridge );

    /** Pull normal vectors from bridge. */
    void
    updateNormals( GridTessBridge& bridge );

    /** Calculate bounding box from bridge vertex positions. */
    void
    updateBoundingBox( GridTessBridge& bridge );

    /** Pull cell data from bridge. */
    void
    updateCells( GridTessBridge& bridge );

    /** Pull triangle data from bridge. */
    void
    updateTriangles( GridTessBridge& bridge );

    void
    updatePolygons( GridTessBridge& bridge );

    /** Pull edge data from bridge. */
    void
    updateEdges( GridTessBridge& bridge );
};
