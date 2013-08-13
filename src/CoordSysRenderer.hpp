#pragma once
#include <GL/glew.h>
#include "TextRenderer.hpp"

class CoordSysRenderer
{
public:
    CoordSysRenderer();

    ~CoordSysRenderer();

    void
    render( const GLfloat* modelview, GLsizei width, GLsizei height );

protected:
    GLuint          m_coordsys_program;
    GLuint          m_coordsys_vbo;
    GLuint          m_coordsys_vao;
    TextRenderer    m_text_renderer;

};
