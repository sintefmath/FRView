/* Copyright STIFTELSEN SINTEF 2013
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

#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "render/ManagedGL.hpp"
#include "render/mesh/AbstractMeshGPUModel.hpp"
#include "render/mesh/CellSetInterface.hpp"
#include "render/mesh/VertexPositionInterface.hpp"
#include "render/mesh/NormalVectorInterface.hpp"
#include "render/mesh/PolygonSetInterface.hpp"
#include "render/mesh/BoundingBoxInterface.hpp"

namespace bridge {
    class PolyhedralMeshBridge;
}

namespace render {
    class GridField;
    class CellSelector;
    namespace subset {
        class Representation;
    }
    namespace mesh {

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
class PolyhedralMeshGPUModel
        : virtual public AbstractMeshGPUModel,
          virtual public CellSetInterface,
          virtual public VertexPositionInterface,
          virtual public NormalVectorInterface,
          virtual public PolygonSetInterface,
          virtual public BoundingBoxInterface
{
public:
    PolyhedralMeshGPUModel();

    ~PolyhedralMeshGPUModel();
    

    /** Check the topology (Euler number and friends) for each cell individually.
     *
     * Mainly used for tracking down bugs.
     */
    void
    checkTopology() const;


    // -------------------------------------------------------------------------
    /** \name Implementation of VertexPositionInterface. */
    /** @{ */

    GLsizei
    vertexCount() const { return m_vertices_num; }

    GLuint
    vertexPositonsAsVertexArrayObject() const { return m_vertex_positions_vao.get();  }

    GLuint
    vertexPositionsAsBufferTexture() const { return m_vertex_positions_tex.get(); }

    const std::vector<float>&
    vertexPositionsInHostMemory() const { return m_vertex_positions_host; }
    
    /** @} */
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    /** \name Implementation of NormalVectorInterface. */
    /** @{ */

    GLuint
    normalVectorsAsBufferTexture() const { return m_normal_vectors_tex.get(); }

    const std::vector<float>&
    normalVectorsInHostMemory() const { return m_normal_vectors_host; }

    /** @} */
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    /** \name Implementation of CellSetInterface */
    /** @{ */

    GLsizei
    cellCount() const { return m_cell_global_index_host.size(); }

    GLuint
    cellGlobalIndexTexture() const { return m_cell_global_index_tex.get(); }

    GLuint
    cellCornerTexture() const { return m_cell_vertex_indices_tex.get(); }

    const std::vector<GLuint>&
    cellGlobalIndicesInHostMemory() const { return m_cell_global_index_host; }

    /** @} */
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    /** \name Implementation of BoundingBoxInterface. */
    /** @{ */

    const float*
    minBBox() const { return m_bb_min; }

    const float*
    maxBBox() const { return m_bb_max; }

    const float*
    scale() const { return m_scale; }

    const float*
    shift() const { return m_shift; }
    /** @} */
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    /** \name Implementation of PolygonSetInterface. */
    /** @{ */

    GLuint
    polygonCellCount() const
    { return 2; }
    
    GLuint
    polygonVertexArray() const
    { return m_polygon_vao.get(); }

    GLuint
    polygonVertexIndexTexture() const
    { return m_polygon_vtx_tex.get(); }

    GLuint
    polygonVertexIndexBuffer() const
    { return m_polygon_vtx_buf.get(); }

    GLuint
    polygonNormalIndexTexture() const
    { return m_polygon_nrm_tex.get(); }

    GLsizei
    polygonCount() const
    { return m_polygons_N; }

    GLsizei
    polygonMaxPolygonSize() const
    { return m_polygon_max_n; }

    GLsizei
    polygonTriangulatedCount() const
    { return m_triangles_N; }
    /** @} */
    // -------------------------------------------------------------------------

    /** Pull data from bridge. */
    void
    update( bridge::PolyhedralMeshBridge& bridge );

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
    GLBuffer                m_vertex_positions_buf;         ///< Buffer object with vertex positions.
    GLTexture               m_vertex_positions_tex;         ///< Texture sampling \ref m_vertex_positions_buf.
    GLVertexArrayObject     m_vertex_positions_vao;         ///< Vertex array object streaming location 0 from \ref m_vertex_positions_buf.
    /** @} */
    /** @{ */
    GLsizei                 m_normals_num;                  ///< Number of normal vectors in grid.
    std::vector<GLfloat>    m_normal_vectors_host;          ///< Host copy of normal vectors.
    GLBuffer                m_normal_vectors_buf;           ///< Buffer object with normal vectors.
    GLTexture               m_normal_vectors_tex;           ///< Texture sampling \ref m_normal_vectors_buf.
    /** @} */
    /** @{ */
    GLsizei                 m_cells_num;                    ///< Number of cells.
    std::vector<GLuint>     m_cell_global_index_host;       ///< Host copy of global cell indices.
    GLBuffer                m_cell_global_index_buf;        ///< Buffer object with global cell indices.
    GLTexture               m_cell_global_index_tex;        ///< Texture sampling \ref m_cell_global_index_buf.
    std::vector<GLuint>     m_cell_vertex_indices_host;  ///< Host copy of cell vertex indices.
    GLBuffer                m_cell_vertex_indices_buf;   ///< Buffer object with cell vertex indices.
    GLTexture               m_cell_vertex_indices_tex;   ///< Texture sampling \ref m_cell_vertex_indices_buf.
    /** @} */

    // -------------------------------------------------------------------------
    
    /** \name Per polygon info. */
    /** @{ */
    
    /** Number of triangles if all polygons were triangulated.
     *
     * I.e., each polygon contributes with N-2 triangles, where N is the number
     * of corners in that polygon. */
    GLsizei                 m_triangles_N;

    /** Number of polygons in set. */
    GLsizei                 m_polygons_N;

    /** Vertex array object with per polygon info.
     *
     * - Binding 0: \ref m_polygon_info_buf
     * - Binding 1: \ref m_polygon_offset_buf
     * - Binding 2: \ref m_polygon_offset_buf+1
     */
    GLVertexArrayObject     m_polygon_vao;

    /** Maximum number of corners in a polygon in the set. */
    GLsizei                 m_polygon_max_n;
    
    /** Per polygon info about adjacent cells and flags (2 uint's).
     *
     * Bits 0..29 inclusive encode cell, 0x3fffffffu means no cell. Bit 31 tags
     * polygon as part of a fault, and bit 30 is currently unused. Flags present
     * in both uints. */
    GLBuffer                m_polygon_info_buf;
    
    /** Per polygon offsets into \ref m_polygon_vtx_buf and \ref m_polygon_nrm_buf. */
    GLBuffer                m_polygon_offset_buf;

    /** Polygon vertex index. */
    GLBuffer                m_polygon_vtx_buf;
    
    /** Buffer texture accessing \ref m_polygon_vtx_buf. */
    GLTexture               m_polygon_vtx_tex;
    
    /** Polygon normal index with flags (1 uint).
     *
     * Bits 0..29 inclusive encode the normal index, bit 30 encode whether there
     * should be rendered an edge on side 1, and bit 31 encodes whether there
     * should be rendered an edge on side 0. */
    GLBuffer                m_polygon_nrm_buf;
    
    /** Buffer texture accessing \ref m_polygon_nrm_buf. */
    GLTexture               m_polygon_nrm_tex;
    /** @} */


    /** Pull vertex data from bridge. */
    void
    updateVertices( bridge::PolyhedralMeshBridge& bridge );

    /** Pull normal vectors from bridge. */
    void
    updateNormals( bridge::PolyhedralMeshBridge& bridge );

    /** Calculate bounding box from bridge vertex positions. */
    void
    updateBoundingBox( bridge::PolyhedralMeshBridge& bridge );

    /** Pull cell data from bridge. */
    void
    updateCells( bridge::PolyhedralMeshBridge& bridge );


    void
    updatePolygons( bridge::PolyhedralMeshBridge& bridge );

};

    } // of namespace mesh
} // of namespace render
