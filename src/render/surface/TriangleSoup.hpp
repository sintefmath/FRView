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
    setTriangleCount(const GLsizei triangles );
    
    GLsizei
    triangleCount() const { return m_triangle_count; }

    GLint
    vertexAttributesAsVertexArrayObject() const
    { return m_attributes_vao.get(); }

protected:
    GLsizei                     m_triangle_count; ///< Actual number of triangles in surface.
    GLsizei                     m_triangle_alloc; ///< Number of triangles allocated.

    /** Buffer holding vertex attribute data
     *
     * vec3 color
     * vec3 normal vector
     * vec3 position
     *
     */
    GLBuffer                    m_attributes;

    /** Track number of vertices produced. */
    GLQuery                     m_attributes_qry;
    GLTransformFeedback         m_attributes_xfb;

    /** Vertex array object with attribute data, for drawing. */
    GLVertexArrayObject         m_attributes_vao;


};


} // namespace surfacae
} // namespace render
