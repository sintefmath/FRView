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
#include <vector>
#include <boost/utility.hpp>
#include "render/ManagedGL.hpp"
#include "render/TextRenderer.hpp"

namespace render {
    namespace wells {

class Representation : public boost::noncopyable
{
public:
    Representation();

    void
    clear();

    void
    addWellHead( const std::string& well_name,
                 const float*       well_head_position);
    
    
    void
    addSegments( const std::vector<float>& positions,
                 const std::vector<float>& colors );

    bool
    empty() const { return m_indices.empty(); }
    
    /**
     * Sideeffects:
     * - VertexArrayObject binding
     * - GL_ARRAY_BUFFER and GL_ELEMENT_ARRAY_BUFFER bindings.
     */
    void
    upload();

    TextRenderer&
    wellHeads() { return m_well_heads; }

    GLVertexArrayObject&
    attribs() { return m_attribs_vao; }

    GLBuffer&
    indices() { return m_indices_buf; }

    GLsizei
    indexCount() { return m_indices.size(); }    
    
protected:
    bool                    m_do_upload;

    GLVertexArrayObject     m_attribs_vao;

    /**
     * Layout:
     * - position, 3 x float
     * - tangent, 3 x float
     * - normal, 3 x float
     * - color, 3 x float
     */
    GLBuffer                m_attribs_buf;
    std::vector<GLfloat>    m_attribs;

    GLBuffer                m_indices_buf;
    std::vector<GLuint>     m_indices;
    
    TextRenderer            m_well_heads;
};
    
    } // of namespace wells
} // of namespace render
