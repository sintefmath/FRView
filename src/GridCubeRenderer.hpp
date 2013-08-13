/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once
#include <GL/glew.h>
#include <boost/utility.hpp>

class TextRenderer;


class GridCubeRenderer : public boost::noncopyable
{
public:
    GridCubeRenderer();

    ~GridCubeRenderer();

    void
    setUnitCubeToObjectTransform( const GLfloat* transform );

    void
    render( const GLfloat* projection,
            const GLfloat* modelview );

protected:
    GLuint                  m_cube_vao;
    GLuint                  m_cube_pos_buf;
    GLuint                  m_cube_nrm_buf;

    GLuint                  m_grid_prog;

    TextRenderer*           m_text_renderer;

};
