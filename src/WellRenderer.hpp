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
#include <vector>
#include <boost/utility.hpp>

class WellRenderer : public boost::noncopyable
{
public:

    WellRenderer();

    ~WellRenderer();

    void
    clear();

    void
    addSegments( const std::vector<float>& positions,
                 const std::vector<float>& colors );

    void
    render( GLsizei           width,
            GLsizei           height,
            const GLfloat*    projection,
            const GLfloat*    camera_from_world,
            const GLfloat*    world_from_model );


protected:
    GLuint                  m_well_prog;
    bool                    m_do_upload;
    GLuint                  m_attribs_vao;
    GLuint                  m_attribs_buf;
    GLuint                  m_indices_buf;
    std::vector<GLfloat>    m_attribs;
    std::vector<GLuint>     m_indices;
};
