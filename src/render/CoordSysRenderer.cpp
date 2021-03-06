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

#include "utils/GLSLTools.hpp"
#include "CoordSysRenderer.hpp"

namespace resources {
    extern const std::string coordsys_vs;
    extern const std::string coordsys_fs;
}

namespace render {

// 8x8 X
static const GLubyte x_bits[] = { 0xc3, 0xc3, 0xc3, 0x66, 0x3c, 0x66, 0xc3, 0xc3 };
// 8x8 Y
static const GLubyte y_bits[] = { 0x18, 0x18, 0x18, 0x18, 0x3c, 0x66, 0xc3, 0xc3 };
// 8x8 Z
static const GLubyte z_bits[] = { 0xff, 0xc3, 0x63, 0x30, 0x18, 0x0c, 0xc6, 0xff };

static GLfloat coordsys_vbo[20*3*6] = {
    // X-axis
    1.f, 0.f, 0.f,    0.05f,-0.05f, 0.05f,
    1.f, 0.f, 0.f,    1.00f,-0.05f, 0.05f,
    1.f, 0.f, 0.f,    1.00f, 0.05f, 0.05f,
    1.f, 0.f, 0.f,    0.05f, 0.05f, 0.05f,
    1.f, 0.f, 0.f,    0.05f, 0.05f, 0.05f,
    1.f, 0.f, 0.f,    1.00f, 0.05f, 0.05f,
    1.f, 0.f, 0.f,    1.00f, 0.05f,-0.05f,
    1.f, 0.f, 0.f,    0.05f, 0.05f,-0.05f,
    1.f, 0.f, 0.f,    0.05f, 0.05f,-0.05f,
    1.f, 0.f, 0.f,    1.00f, 0.05f,-0.05f,
    1.f, 0.f, 0.f,    1.00f,-0.05f,-0.05f,
    1.f, 0.f, 0.f,   -0.05f,-0.05f,-0.05f,
    1.f, 0.f, 0.f,   -0.05f,-0.05f,-0.05f,
    1.f, 0.f, 0.f,    1.00f,-0.05f,-0.05f,
    1.f, 0.f, 0.f,    1.00f,-0.05f, 0.05f,
    1.f, 0.f, 0.f,    0.05f,-0.05f, 0.05f,
    1.f, 0.f, 0.f,    1.00f,-0.05f, 0.05f,
    1.f, 0.f, 0.f,    1.00f,-0.05f,-0.05f,
    1.f, 0.f, 0.f,    1.00f, 0.05f,-0.05f,
    1.f, 0.f, 0.f,    1.00f, 0.05f, 0.05f,
    // Y-axis
    0.f, 1.f, 0.f,   -0.05f,-0.05f,-0.05f,
    0.f, 1.f, 0.f,   -0.05f, 1.00f,-0.05f,
    0.f, 1.f, 0.f,    0.05f, 1.00f,-0.05f,
    0.f, 1.f, 0.f,    0.05f, 0.05f,-0.05f,
    0.f, 1.f, 0.f,    0.05f, 0.05f,-0.05f,
    0.f, 1.f, 0.f,    0.05f, 1.00f,-0.05f,
    0.f, 1.f, 0.f,    0.05f, 1.00f, 0.05f,
    0.f, 1.f, 0.f,    0.05f, 0.05f, 0.05f,
    0.f, 1.f, 0.f,    0.05f, 0.05f, 0.05f,
    0.f, 1.f, 0.f,    0.05f, 1.00f, 0.05f,
    0.f, 1.f, 0.f,   -0.05f, 1.00f, 0.05f,
    0.f, 1.f, 0.f,   -0.05f, 0.05f, 0.05f,
    0.f, 1.f, 0.f,   -0.05f, 0.05f, 0.05f,
    0.f, 1.f, 0.f,   -0.05f, 1.00f, 0.05f,
    0.f, 1.f, 0.f,   -0.05f, 1.00f,-0.05f,
    0.f, 1.f, 0.f,   -0.05f,-0.05f,-0.05f,
    0.f, 1.f, 0.f,   -0.05f, 1.00f,-0.05f,
    0.f, 1.f, 0.f,   -0.05f, 1.00f, 0.05f,
    0.f, 1.f, 0.f,    0.05f, 1.00f, 0.05f,
    0.f, 1.f, 0.f,    0.05f, 1.00f,-0.05f,
    // Z-axis
    0.f, 0.f, 1.f,   -0.05f, 0.05f, 0.05f,
    0.f, 0.f, 1.f,   -0.05f, 0.05f, 1.00f,
    0.f, 0.f, 1.f,    0.05f, 0.05f, 1.00f,
    0.f, 0.f, 1.f,    0.05f, 0.05f, 0.05f,
    0.f, 0.f, 1.f,    0.05f, 0.05f, 0.05f,
    0.f, 0.f, 1.f,    0.05f, 0.05f, 1.00f,
    0.f, 0.f, 1.f,    0.05f,-0.05f, 1.00f,
    0.f, 0.f, 1.f,    0.05f,-0.05f, 0.05f,
    0.f, 0.f, 1.f,    0.05f,-0.05f, 0.05f,
    0.f, 0.f, 1.f,    0.05f,-0.05f, 1.00f,
    0.f, 0.f, 1.f,   -0.05f,-0.05f, 1.00f,
    0.f, 0.f, 1.f,   -0.05f,-0.05f,-0.05f,
    0.f, 0.f, 1.f,   -0.05f,-0.05f,-0.05f,
    0.f, 0.f, 1.f,   -0.05f,-0.05f, 1.00f,
    0.f, 0.f, 1.f,   -0.05f, 0.05f, 1.00f,
    0.f, 0.f, 1.f,   -0.05f, 0.05f, 0.05f,
    0.f, 0.f, 1.f,   -0.05f, 0.05f, 1.00f,
    0.f, 0.f, 1.f,   -0.05f,-0.05f, 1.00f,
    0.f, 0.f, 1.f,    0.05f,-0.05f, 1.00f,
    0.f, 0.f, 1.f,    0.05f, 0.05f, 1.00f,
};

CoordSysRenderer::CoordSysRenderer()
{
    Logger log = getLogger( "render.CoordSysRenderer" );
    GLfloat pos_x[3] = {1.1f, 0.f, 0.f };
    GLfloat pos_y[3] = {0.f, 1.1f, 0.f };
    GLfloat pos_z[3] = {0.f, 0.f, 1.1f };

    m_text_renderer.add( "X", TextRenderer::FONT_8X12, pos_x );
    m_text_renderer.add( "Y", TextRenderer::FONT_8X12, pos_y );
    m_text_renderer.add( "Z", TextRenderer::FONT_8X12, pos_z );

    glGenVertexArrays( 1, &m_coordsys_vao );
    glBindVertexArray( m_coordsys_vao );
    glGenBuffers( 1, &m_coordsys_vbo );
    glBindBuffer( GL_ARRAY_BUFFER, m_coordsys_vbo );
    glBufferData( GL_ARRAY_BUFFER,
                  sizeof( coordsys_vbo),
                  coordsys_vbo,
                  GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(GLfloat), reinterpret_cast<GLvoid*>( 3*sizeof(GLfloat) ) );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_TRUE, 6*sizeof(GLfloat), reinterpret_cast<GLvoid*>( 0*sizeof(GLfloat)) );
    glEnableVertexAttribArray(1);
    glBindBuffer( GL_ARRAY_BUFFER, 0);
    glBindVertexArray( 0 );

    GLuint vs = utils::compileShader( log, resources::coordsys_vs, GL_VERTEX_SHADER );
    GLuint fs = utils::compileShader( log, resources::coordsys_fs, GL_FRAGMENT_SHADER );

    m_coordsys_program = glCreateProgram();
    glAttachShader( m_coordsys_program, vs );
    glAttachShader( m_coordsys_program, fs );
    utils::linkProgram( log, m_coordsys_program );
    glDeleteShader( vs );
    glDeleteShader( fs );
}

CoordSysRenderer::~CoordSysRenderer()
{
    glDeleteVertexArrays( 1, &m_coordsys_vao );
    glDeleteBuffers( 1, &m_coordsys_vbo );
    glDeleteProgram( m_coordsys_program );
}

void
CoordSysRenderer::render( const GLfloat* modelview, GLsizei width, GLsizei height )
{
    GLfloat mv[16] = { 0.7f*modelview[0], 0.7f*modelview[1], 0.1f*modelview[2], 0.f,
                       0.7f*modelview[4], 0.7f*modelview[5], 0.1f*modelview[6], 0.f,
                       0.7f*modelview[8], 0.7f*modelview[9], 0.1f*modelview[10], 0.f,
                       0.f, 0.f, -0.5f, 1.f };

/*
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glMatrixMode( GL_MODELVIEW );
    glLoadMatrixf( mv );
//    glDisable( GL_DEPTH_TEST );
*/
    glUseProgram( m_coordsys_program );
    glUniformMatrix4fv( glGetUniformLocation( m_coordsys_program, "mv" ), 1, GL_FALSE, mv );
    glBindVertexArray( m_coordsys_vao );
    glDrawArrays( GL_QUADS, 0, 20*3*3 );
    glBindVertexArray( 0 );

    m_text_renderer.render( width, height, mv );
}

} // of namespace render
