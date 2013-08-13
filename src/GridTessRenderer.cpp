#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <siut/gl_utils/GLSLtools.hpp>
#include <siut/io_utils/snarf.hpp>
#include "Logger.hpp"
#include "GridTess.hpp"
#include "GridField.hpp"
#include "GridTessRenderer.hpp"

using siut::gl_utils::compileShader;
using siut::gl_utils::linkProgram;
using siut::io_utils::snarfFile;

GridTessRenderer::GridTessRenderer()
    : m_magic( false )
{

    glGenFramebuffers( 1, &m_gbuffer.m_fbo );
    glGenTextures( 1, &m_gbuffer.m_cell_id_texture );
    glGenTextures( 1, &m_gbuffer.m_depth_texture );
    glGenTextures( 1, &m_gbuffer.m_normal_texture );
    m_gbuffer.m_width = ~0u;
    m_gbuffer.m_heigth = ~0u;


    // create GPGPU quad
    static const GLfloat quad[ 4*4 ] = {
         1.f, -1.f, 0.f, 1.f,
         1.f,  1.f, 0.f, 1.f,
        -1.f, -1.f, 0.f, 1.f,
        -1.f,  1.f, 0.f, 1.f
    };
    glGenVertexArrays( 1, &m_gpgpu_quad.m_vertex_array );
    glBindVertexArray( m_gpgpu_quad.m_vertex_array );
    glGenBuffers( 1, &m_gpgpu_quad.m_vertex_buffer );
    glBindBuffer( GL_ARRAY_BUFFER, m_gpgpu_quad.m_vertex_buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 0, NULL );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );


    {
        m_screen_solid.m_prog = glCreateProgram();
        m_screen_solid.m_vs = compileShader( snarfFile( "shaders/screen_common_vs.glsl" ),
                                             GL_VERTEX_SHADER, true );
        m_screen_solid.m_fs = compileShader( snarfFile( "shaders/screen_colorize_fs.glsl" ),
                                             GL_FRAGMENT_SHADER, true );
        glAttachShader( m_screen_solid.m_prog, m_screen_solid.m_vs );
        glAttachShader( m_screen_solid.m_prog, m_screen_solid.m_fs );
        linkProgram( m_screen_solid.m_prog );

        glUseProgram( m_screen_solid.m_prog );
        glUniform1i( glGetUniformLocation( m_screen_solid.m_prog, "cell_index"), 0 );
        glUniform1i( glGetUniformLocation( m_screen_solid.m_prog, "depth"), 1 );
        glUniform1i( glGetUniformLocation( m_screen_solid.m_prog, "normal"), 2 );
        m_screen_solid.m_loc_wireframe = glGetUniformLocation( m_screen_solid.m_prog, "wireframe" );
        glUseProgram( 0 );
    }

    {
        m_screen_field.m_prog = glCreateProgram();
        m_screen_field.m_vs = compileShader( snarfFile( "shaders/screen_common_vs.glsl" ),
                                             GL_VERTEX_SHADER, true );
        m_screen_field.m_fs = compileShader(  "#define FIELD\n" +
                                              snarfFile( "shaders/screen_colorize_fs.glsl" ),
                                              GL_FRAGMENT_SHADER, true );
        glAttachShader( m_screen_field.m_prog, m_screen_field.m_vs );
        glAttachShader( m_screen_field.m_prog, m_screen_field.m_fs );
        linkProgram( m_screen_field.m_prog );

        glUseProgram( m_screen_field.m_prog );
        glUniform1i( glGetUniformLocation( m_screen_field.m_prog, "cell_index"), 0 );
        glUniform1i( glGetUniformLocation( m_screen_field.m_prog, "depth"), 1 );
        glUniform1i( glGetUniformLocation( m_screen_field.m_prog, "normal"), 2 );
        glUniform1i( glGetUniformLocation( m_screen_field.m_prog, "field"), 3 );
        m_screen_field.m_loc_wireframe = glGetUniformLocation( m_screen_field.m_prog, "wireframe" );
        m_screen_field.m_loc_linear_map = glGetUniformLocation( m_screen_field.m_prog, "linear_map" );
        glUseProgram( 0 );
    }

    {
        m_cell_gbuffer_shader.m_prog = glCreateProgram();
        m_cell_gbuffer_shader.m_vs = siut::gl_utils::compileShader( siut::io_utils::snarfFile( "shaders/gbuffer_vs.glsl" ),
                                                                  GL_VERTEX_SHADER,
                                                                  true );
        m_cell_gbuffer_shader.m_gs = siut::gl_utils::compileShader( siut::io_utils::snarfFile( "shaders/gbuffer_gs.glsl" ),
                                                                  GL_GEOMETRY_SHADER,
                                                                  true );
        m_cell_gbuffer_shader.m_fs = siut::gl_utils::compileShader( siut::io_utils::snarfFile( "shaders/gbuffer_fs.glsl" ),
                                                                  GL_FRAGMENT_SHADER,
                                                                  true );
        glAttachShader( m_cell_gbuffer_shader.m_prog, m_cell_gbuffer_shader.m_vs );
        glAttachShader( m_cell_gbuffer_shader.m_prog, m_cell_gbuffer_shader.m_gs );
        glAttachShader( m_cell_gbuffer_shader.m_prog, m_cell_gbuffer_shader.m_fs );
        siut::gl_utils::linkProgram( m_cell_gbuffer_shader.m_prog );

        m_cell_gbuffer_shader.m_loc_mvp = glGetUniformLocation( m_cell_gbuffer_shader.m_prog, "MVP" );
        m_cell_gbuffer_shader.m_loc_nm = glGetUniformLocation( m_cell_gbuffer_shader.m_prog, "NM" );

        glUseProgram( m_cell_gbuffer_shader.m_prog );
        GLint loc = glGetUniformLocation( m_cell_gbuffer_shader.m_prog, "tri_info" );
        glUniform1i( loc, 0 );
        loc = glGetUniformLocation( m_cell_gbuffer_shader.m_prog, "cell_subset" );
        glUniform1i( loc, 1 );
        glUseProgram( 0 );
    }

    {
        m_cell_gbuffer_compact_shader.m_prog = glCreateProgram();
        m_cell_gbuffer_compact_shader.m_vs = siut::gl_utils::compileShader( "#define COMPACT\n" +
                                                                          siut::io_utils::snarfFile( "shaders/gbuffer_vs.glsl" ),
                                                                          GL_VERTEX_SHADER,
                                                                          true );
        m_cell_gbuffer_compact_shader.m_gs = siut::gl_utils::compileShader( "#define COMPACT\n" +
                                                                          siut::io_utils::snarfFile( "shaders/gbuffer_gs.glsl" ),
                                                                          GL_GEOMETRY_SHADER,
                                                                          true );
        m_cell_gbuffer_compact_shader.m_fs = siut::gl_utils::compileShader(  "#define COMPACT\n" +
                                                                           siut::io_utils::snarfFile( "shaders/gbuffer_fs.glsl" ),
                                                                           GL_FRAGMENT_SHADER,
                                                                           true );
        glAttachShader( m_cell_gbuffer_compact_shader.m_prog, m_cell_gbuffer_compact_shader.m_vs );
        glAttachShader( m_cell_gbuffer_compact_shader.m_prog, m_cell_gbuffer_compact_shader.m_gs );
        glAttachShader( m_cell_gbuffer_compact_shader.m_prog, m_cell_gbuffer_compact_shader.m_fs );
        siut::gl_utils::linkProgram( m_cell_gbuffer_compact_shader.m_prog );

        m_cell_gbuffer_compact_shader.m_loc_mvp = glGetUniformLocation( m_cell_gbuffer_compact_shader.m_prog, "MVP" );
        m_cell_gbuffer_compact_shader.m_loc_nm = glGetUniformLocation( m_cell_gbuffer_compact_shader.m_prog, "NM" );

        glUseProgram( m_cell_gbuffer_compact_shader.m_prog );
        GLint loc = glGetUniformLocation( m_cell_gbuffer_compact_shader.m_prog, "cells" );
        glUniform1i( loc, 0 );
        glUseProgram( 0 );
    }

    CHECK_GL;
}

GridTessRenderer::~GridTessRenderer()
{
}

void
GridTessRenderer::magic()
{
    m_magic = !m_magic;
    std::cerr << "magic = " << m_magic << "\n";
}

void
GridTessRenderer::resizeGBuffer( const GLsizei width, const GLsizei height )
{
    glBindTexture( GL_TEXTURE_2D, m_gbuffer.m_cell_id_texture );
    glTexImage2D( GL_TEXTURE_2D,
                  0,
                  GL_R32UI,
                  width,
                  height,
                  0,
                  GL_RED_INTEGER,
                  GL_UNSIGNED_INT,
                  NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture( GL_TEXTURE_2D, m_gbuffer.m_depth_texture );
    glTexImage2D( GL_TEXTURE_2D,
                  0,
                  GL_DEPTH_COMPONENT,
                  width,
                  height,
                  0,
                  GL_DEPTH_COMPONENT,
                  GL_FLOAT,
                  NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture( GL_TEXTURE_2D, 0);

    glBindTexture( GL_TEXTURE_2D, m_gbuffer.m_normal_texture );
    glTexImage2D( GL_TEXTURE_2D,
                  0,
                  GL_RGB,
                  width,
                  height,
                  0,
                  GL_RGB,
                  GL_FLOAT,
                  NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture( GL_TEXTURE_2D, 0);

    glBindFramebuffer( GL_FRAMEBUFFER, m_gbuffer.m_fbo );
    glFramebufferTexture2D( GL_FRAMEBUFFER,
                            GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D,
                            m_gbuffer.m_cell_id_texture,
                            0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER,
                            GL_COLOR_ATTACHMENT1,
                            GL_TEXTURE_2D,
                            m_gbuffer.m_normal_texture,
                            0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER,
                            GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_2D,
                            m_gbuffer.m_depth_texture,
                            0 );
    GLenum drawbuffers[2] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1
    };
    glDrawBuffers( 2, drawbuffers );
    CHECK_FBO;


    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    CHECK_GL;
}

void
GridTessRenderer::populateGBuffer( const GLfloat*   modelview,
                                   const GLfloat*   projection,
                                   const GridTess&  tess )
{
    glm::mat4 M(modelview[0], modelview[1], modelview[ 2], modelview[3],
                modelview[4], modelview[5], modelview[ 6], modelview[7],
                modelview[8], modelview[9], modelview[10], modelview[11],
                modelview[12], modelview[13], modelview[14], modelview[15] );
    glm::mat4 P(projection[0], projection[1], projection[ 2], projection[3],
                projection[4], projection[5], projection[ 6], projection[7],
                projection[8], projection[9], projection[10], projection[11],
                projection[12], projection[13], projection[14], projection[15] );
    glm::mat4 MVP = P*M;

    glm::mat4 NM = glm::transpose( glm::inverse( M ) );
    const GLfloat* nm4 = glm::value_ptr( NM );
    const GLfloat nm3[9] = { nm4[0], nm4[1], nm4[2],
                             nm4[4], nm4[5], nm4[6],
                             nm4[8], nm4[9], nm4[10] };


    if( m_magic ) {
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_BUFFER, tess.triangleCompactCellTexture() );

        glUseProgram( m_cell_gbuffer_compact_shader.m_prog );
        glUniformMatrix4fv( m_cell_gbuffer_compact_shader.m_loc_mvp, 1, GL_FALSE, glm::value_ptr( MVP ) );
        glUniformMatrix3fv( m_cell_gbuffer_compact_shader.m_loc_nm, 1, GL_FALSE, nm3 );

        glBindVertexArray( tess.vertexVertexArrayObject() );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, tess.triangleCompactIndexBuffer() );
        glDrawElements( GL_TRIANGLES, 3*tess.triangleCompactCount(), GL_UNSIGNED_INT, NULL );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        glBindVertexArray( 0 );
        glUseProgram( 0 );

        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
    }
    else {
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_BUFFER, tess.triangleInfoTexture() );
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_BUFFER, tess.cellSubsetTexture() );

        glUseProgram( m_cell_gbuffer_shader.m_prog );
        glUniformMatrix4fv( m_cell_gbuffer_shader.m_loc_mvp, 1, GL_FALSE, glm::value_ptr( MVP ) );
        glUniformMatrix3fv( m_cell_gbuffer_shader.m_loc_nm, 1, GL_FALSE, nm3 );

        glBindVertexArray( tess.vertexVertexArrayObject() );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, tess.triangleIndexBuffer() );
        glDrawElements( GL_TRIANGLES, 3*tess.triangleCount(), GL_UNSIGNED_INT, NULL );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        glBindVertexArray( 0 );
        glUseProgram( 0 );

        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
    }
    CHECK_GL;

}


void
GridTessRenderer::renderCells( GLuint           fbo,
                               const GLsizei    width,
                               const GLsizei    height,
                               const GLfloat*   modelview,
                               const GLfloat*   projection,
                               const GridTess&  tess,
                               const bool       wireframe )
{
    if( width == 0 || height == 0 ) {
        return;
    }

    // populate gbuffer
    resizeGBuffer( width, height );
    glBindFramebuffer( GL_FRAMEBUFFER, m_gbuffer.m_fbo );
    glViewport( 0, 0, width, height );
    glClearColorIuiEXT( 0, 0, 0, 0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    populateGBuffer( modelview, projection, tess );

    // colorize
    glDisable( GL_CULL_FACE );
    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glUseProgram( m_screen_solid.m_prog );
    glUniform1i( m_screen_solid.m_loc_wireframe, wireframe );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, m_gbuffer.m_cell_id_texture );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, m_gbuffer.m_depth_texture );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_2D, m_gbuffer.m_normal_texture );

    glBindVertexArray( m_gpgpu_quad.m_vertex_array );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    glBindVertexArray( 0 );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, 0 );
}

void
GridTessRenderer::renderCells( GLuint            fbo,
                               const GLsizei     width,
                               const GLsizei     height,
                               const GLfloat*    modelview,
                               const GLfloat*    projection,
                               const GridTess&   tess,
                               const GridField&  field,
                               const bool        wireframe )
{
    if( width == 0 || height == 0 ) {
        return;
    }



    // populate gbuffer
    resizeGBuffer( width, height );
    glBindFramebuffer( GL_FRAMEBUFFER, m_gbuffer.m_fbo );
    glViewport( 0, 0, width, height );
    glClearColorIuiEXT( 0, 0, 0, 0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    populateGBuffer( modelview, projection, tess );


    // colorize
    glDisable( GL_CULL_FACE );
    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glUseProgram( m_screen_field.m_prog );
    glUniform1i( m_screen_field.m_loc_wireframe, wireframe );
    glUniform2f( m_screen_field.m_loc_linear_map,
                 field.minValue(),
                 1.f/(field.maxValue()-field.minValue() ) );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, m_gbuffer.m_cell_id_texture );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, m_gbuffer.m_depth_texture );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_2D, m_gbuffer.m_normal_texture );
    glActiveTexture( GL_TEXTURE3 );
    glBindTexture( GL_TEXTURE_BUFFER, field.texture() );

    glBindVertexArray( m_gpgpu_quad.m_vertex_array );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    glBindVertexArray( 0 );

    glActiveTexture( GL_TEXTURE3 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, 0 );
}



