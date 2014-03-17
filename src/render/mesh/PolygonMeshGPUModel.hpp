#pragma once
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

#include <GL/glew.h>
#include <boost/shared_ptr.hpp>
#include "render/ManagedGL.hpp"
#include "render/mesh/AbstractMeshGPUModel.hpp"
#include "render/mesh/CellSetInterface.hpp"
#include "render/mesh/BoundingBoxInterface.hpp"
#include "render/mesh/VertexPositionInterface.hpp"
#include "render/mesh/NormalVectorInterface.hpp"

namespace bridge {
    class PolygonMeshBridge;
} // of namespace bridge

namespace render {
    namespace mesh {

class PolygonMeshGPUModel
        : virtual public AbstractMeshGPUModel,
          virtual public BoundingBoxInterface
          // virtual public CellSetInterface
          // virtual public VertexPositionInterface
          // virtual public NormalVectorInterface
{
public:
    PolygonMeshGPUModel();
    
    ~PolygonMeshGPUModel();
    
    /** Reset contents with data from bridge. */
    void
    update( boost::shared_ptr<const bridge::PolygonMeshBridge> mesh_bridge );

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
    std::vector<GLuint>     m_cell_vertex_indices_host;     ///< Host copy of cell vertex indices.
    GLBuffer                m_cell_vertex_indices_buf;      ///< Buffer object with cell vertex indices.
    GLTexture               m_cell_vertex_indices_tex;      ///< Texture sampling \ref m_cell_vertex_indices_buf.
    /** @} */

    void
    reset();
    
    bool
    updateVertices( boost::shared_ptr<const bridge::PolygonMeshBridge> mesh_bridge );

    bool
    updateBoundingBox( boost::shared_ptr<const bridge::PolygonMeshBridge> mesh_bridge );

    bool
    updateNormals( boost::shared_ptr<const bridge::PolygonMeshBridge> mesh_bridge );

    bool
    updateCells( boost::shared_ptr<const bridge::PolygonMeshBridge> mesh_bridge );
    
};
    
    } // of namespace mesh
} // of namespace render