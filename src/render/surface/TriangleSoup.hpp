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

#pragma once

#include <GL/glew.h>
#include <boost/utility.hpp>
#include "render/ManagedGL.hpp"

namespace render {
namespace surface {

class TriangleSoup : public boost::noncopyable
{
public:
    
    TriangleSoup();
    
    bool
    setTriangleAndEdgeVertexCount( const GLsizei triangle_vertices, const GLsizei edge_vertices );
    
    GLsizei
    triangleVertexCount() const { return m_triangle_vtx_count; }

    GLsizei
    edgeVertexCount() const { return m_edge_vtx_count; }

    GLint
    triangleVertexAttributesAsVertexArrayObject() const
    { return m_triangle_attributes_vao.get(); }

    GLint
    edgeVertexAttributesAsVertexArrayObject() const
    { return m_edge_attributes_vao.get(); }

    GLuint
    vertexAttributesTransformFeedback()
    { return m_attributes_xfb.get(); }

    GLuint
    triangleVertexCountQuery() { return m_triangle_vertex_count_qry.get(); }

    GLuint
    edgeVertexCountQuery() { return m_edge_vertex_count_qry.get(); }

protected:
    GLsizei                     m_triangle_vtx_count; ///< Actual number of triangles in surface.
    GLsizei                     m_triangle_vtx_alloc; ///< Number of triangles allocated.

    GLsizei                     m_edge_vtx_count; ///< Actual number of triangles in surface.
    GLsizei                     m_edge_vtx_alloc; ///< Number of triangles allocated.

    /** Buffer holding vertex attribute data
     *
     * vec3 color
     * vec3 normal vector
     * vec3 position
     *
     */
    GLBuffer                    m_triangle_attributes;

    GLBuffer                    m_edge_attributes;

    /** Track number of vertices produced. */
    GLTransformFeedback         m_attributes_xfb;

    /** Vertex array object with attribute data, for drawing. */
    GLVertexArrayObject         m_triangle_attributes_vao;

    GLVertexArrayObject         m_edge_attributes_vao;

    GLQuery                     m_triangle_vertex_count_qry;
    GLQuery                     m_edge_vertex_count_qry;
};


} // namespace surfacae
} // namespace render
