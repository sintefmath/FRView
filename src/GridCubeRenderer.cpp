/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <siut2/gl_utils/GLSLtools.hpp>
#include "GridCubeRenderer.hpp"
#include "TextRenderer.hpp"

using siut2::gl_utils::compileShader;
using siut2::gl_utils::linkProgram;
static const std::string package = "GridCubeRenderer";

namespace resources {
    extern const std::string grid_cube_v;
    extern const std::string grid_cube_f;
}


GridCubeRenderer::GridCubeRenderer()
{
    static const GLfloat cube_pos[12*12] = {
        -0.1,  1.1, -0.1, 0,    1.1,  1.1, -0.1, 0,    1.1, -0.1, -0.1, 0,
         1.1, -0.1, -0.1, 0,   -0.1, -0.1, -0.1, 0,   -0.1,  1.1, -0.1, 0,
        -0.1,  1.1,  1.1, 0,    1.1, -0.1,  1.1, 0,    1.1,  1.1,  1.1, 0,
         1.1, -0.1,  1.1, 0,   -0.1,  1.1,  1.1, 0,   -0.1, -0.1,  1.1, 0,
         1.1,  1.1, -0.1, 1,    1.1,  1.1,  1.1, 1,    1.1, -0.1,  1.1, 1,
         1.1, -0.1,  1.1, 1,    1.1, -0.1, -0.1, 1,    1.1,  1.1, -0.1, 1,
        -0.1,  1.1, -0.1, 1,   -0.1, -0.1,  1.1, 1,   -0.1,  1.1,  1.1, 1,
        -0.1, -0.1,  1.1, 1,   -0.1,  1.1, -0.1, 1,   -0.1, -0.1, -0.1, 1,
        -0.1,  1.1, -0.1, 2,   -0.1,  1.1,  1.1, 2,    1.1,  1.1,  1.1, 2,
         1.1,  1.1,  1.1, 2,    1.1,  1.1, -0.1, 2,   -0.1,  1.1, -0.1, 2,
        -0.1, -0.1, -0.1, 2,    1.1, -0.1,  1.1, 2,   -0.1, -0.1,  1.1, 2,
         1.1, -0.1,  1.1, 2,   -0.1, -0.1, -0.1, 2,    1.1, -0.1, -0.1, 2
    };


    glGenVertexArrays( 1, &m_cube_vao );
    glBindVertexArray( m_cube_vao );
    glGenBuffers( 1, &m_cube_pos_buf );
    glBindBuffer( GL_ARRAY_BUFFER, m_cube_pos_buf );
    glBufferData( GL_ARRAY_BUFFER, sizeof(cube_pos), cube_pos, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 0, NULL );
    glEnableVertexAttribArray( 0 );
    glBindVertexArray( 0 );
    CHECK_GL;

    GLuint grid_v = compileShader( resources::grid_cube_v, GL_VERTEX_SHADER );
    GLuint grid_f = compileShader( resources::grid_cube_f, GL_FRAGMENT_SHADER );
    m_grid_prog = glCreateProgram();
    glAttachShader( m_grid_prog, grid_v );
    glAttachShader( m_grid_prog, grid_f );
    linkProgram( m_grid_prog );
    glDeleteShader( grid_v );
    glDeleteShader( grid_f );



}

GridCubeRenderer::~GridCubeRenderer()
{
    glDeleteVertexArrays( 1, &m_cube_vao );
    glDeleteBuffers( 1, &m_cube_pos_buf );
    glDeleteProgram( m_grid_prog );
}

void
GridCubeRenderer::setUnitCubeToObjectTransform( const GLfloat* transform )
{
    GLfloat sx = 5.f/transform[0];
    GLfloat sy = 5.f/transform[5];
    GLfloat sz = 5.f/transform[10];


    glProgramUniformMatrix4fv( m_grid_prog,
                               glGetUniformLocation( m_grid_prog, "unit_to_object" ),
                               1, GL_FALSE, transform );

    glProgramUniform3f( m_grid_prog,
                        glGetUniformLocation( m_grid_prog, "scale" ),
                        sx, sy, sz );
}


void
GridCubeRenderer::render( const GLfloat* projection,
                          const GLfloat* modelview )
{

    glm::mat4 p = glm::mat4( projection[0],  projection[1],  projection[2],  projection[3],
                             projection[4],  projection[5],  projection[6],  projection[7],
                             projection[8],  projection[9],  projection[10], projection[11],
                             projection[12], projection[13], projection[14], projection[15] );

    glm::mat4 mv = glm::mat4( modelview[0], modelview[1],  modelview[2],  modelview[3],
                              modelview[4], modelview[5],  modelview[6],  modelview[7],
                              modelview[8], modelview[9],  modelview[10], modelview[11],
                              modelview[12],modelview[13], modelview[14], modelview[15] );
    glm::mat4 mvp = p * mv;
    glUseProgram( m_grid_prog );
    glUniformMatrix4fv( glGetUniformLocation( m_grid_prog, "mvp" ),
                        1, GL_FALSE, glm::value_ptr( mvp ) );

    glEnable( GL_CULL_FACE );
    glCullFace( GL_FRONT );
    glDisable( GL_DEPTH_TEST );
    glBindVertexArray( m_cube_vao );
    glDrawArrays( GL_TRIANGLES, 0, 12*3 );
    glDisable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    glEnable( GL_DEPTH_TEST );
    glBindVertexArray( 0 );

    glUseProgram( 0 );

    //    m_text_renderer->render( width, height, glm::value_ptr( p*mv ) );

}

