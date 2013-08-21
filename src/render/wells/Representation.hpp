/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2013 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once

#include <GL/glew.h>
#include <vector>
#include <boost/utility.hpp>
#include "render/ManagedGL.hpp"

namespace render {
    namespace wells {

class Representation : public boost::noncopyable
{
public:
    Representation();

    void
    clear();

    void
    addSegments( const std::vector<float>& positions,
                 const std::vector<float>& colors );

    /**
     * Sideeffects:
     * - VertexArrayObject binding
     * - GL_ARRAY_BUFFER and GL_ELEMENT_ARRAY_BUFFER bindings.
     */
    void
    upload();
    
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
    
    
};
    
    } // of namespace wells
} // of namespace render
