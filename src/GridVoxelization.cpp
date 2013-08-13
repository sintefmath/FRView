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
#include <glm/gtx/string_cast.hpp>

#include <siut2/gl_utils/GLSLtools.hpp>
#include "Logger.hpp"
#include "GridTess.hpp"
#include "GridTessSubset.hpp"
#include "GridVoxelization.hpp"


namespace resources {
    extern const std::string grid_voxelizer_compact_vs;
    extern const std::string grid_voxelizer_compact_gs;
    extern const std::string grid_voxelizer_vs;
    extern const std::string grid_voxelizer_fs;
    extern const std::string grid_voxelizer_gs;
}

GridVoxelization::GridVoxelization()
    : m_resolution(16)
{
    glGenTextures( 1, &m_voxels_tex );
    glBindTexture( GL_TEXTURE_3D, m_voxels_tex );
    glTexImage3D( GL_TEXTURE_3D, 0, GL_R32UI,
                  m_resolution, m_resolution, m_resolution, 0,
                  GL_RED_INTEGER , GL_INT, NULL );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glBindTexture( GL_TEXTURE_3D, 0 );

    m_compacted_alloc = 1000;
    glGenBuffers( 1, &m_compacted_buf );
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_compacted_buf );
    glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                  (sizeof(GLfloat)*6+sizeof(GLuint))*m_compacted_alloc,
                  NULL,
                  GL_STREAM_DRAW );
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );

    glGenVertexArrays( 1, &m_compacted_vao );
    glBindVertexArray( m_compacted_vao );
    glBindBuffer( GL_ARRAY_BUFFER, m_compacted_buf );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6+sizeof(GLuint), NULL );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6+sizeof(GLuint), (GLvoid*)(sizeof(GLfloat)*3) );
    glEnableVertexAttribArray( 1 );
    glVertexAttribIPointer( 2, 1, GL_UNSIGNED_INT, sizeof(GLfloat)*6+sizeof(GLuint), (GLvoid*)(sizeof(GLfloat)*6) );
    glEnableVertexAttribArray( 2 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );


    glGenQueries( 1, &m_compacted_count_query );

    glGenFramebuffers( 1, &m_slice_fbo );

    m_voxelizer_program = glCreateProgram();

    GLuint vs = siut2::gl_utils::compileShader( resources::grid_voxelizer_vs,
                                                GL_VERTEX_SHADER, true );
    glAttachShader( m_voxelizer_program, vs );
    glDeleteShader( vs );

    GLuint gs = siut2::gl_utils::compileShader( resources::grid_voxelizer_gs,
                                                GL_GEOMETRY_SHADER, true );
    glAttachShader( m_voxelizer_program, gs );
    glDeleteShader( gs );

    GLuint fs = siut2::gl_utils::compileShader( resources::grid_voxelizer_fs,
                                                GL_FRAGMENT_SHADER, true );
    glAttachShader( m_voxelizer_program, fs );
    glDeleteShader( fs );

    siut2::gl_utils::linkProgram( m_voxelizer_program );

    m_voxelizer_slice_loc = glGetUniformLocation( m_voxelizer_program, "slice" );

    GLuint sh;

    const char* compact_feedback[3] = {
        "bbmin",
        "bbmax",
        "cellid"
    };

    m_compact_program = glCreateProgram();
    sh = siut2::gl_utils::compileShader( resources::grid_voxelizer_compact_vs,
                                         GL_VERTEX_SHADER, true );
    glAttachShader( m_compact_program, sh );
    glDeleteShader( sh );
    sh = siut2::gl_utils::compileShader( resources::grid_voxelizer_compact_gs,
                                         GL_GEOMETRY_SHADER, true );
    glAttachShader( m_compact_program, sh );
    glDeleteShader( sh );

    glTransformFeedbackVaryings( m_compact_program,
                                 3, compact_feedback,
                                 GL_INTERLEAVED_ATTRIBS );
    siut2::gl_utils::linkProgram( m_compact_program );
    m_compact_local_to_world_loc = glGetUniformLocation( m_compact_program, "local_to_world" );


    CHECK_GL;
}

GridVoxelization::~GridVoxelization()
{
    glDeleteTextures( 1, &m_voxels_tex );
    glDeleteFramebuffers( 1, &m_slice_fbo );
}


void
GridVoxelization::dimension( GLsizei* dim ) const
{
    dim[0] = m_resolution;
    dim[1] = m_resolution;
    dim[2] = m_resolution;
}

void
GridVoxelization::build( const GridTess* tess, const GridTessSubset* subset, const GLfloat* world_from_local )
{
    Logger log = getLogger( "GridVoxelization.build" );

    // transform to unit cube and discard unselected cells
    if( m_compacted_alloc < tess->cellCount() ) {
        m_compacted_alloc = 1.01*tess->cellCount();
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_compacted_buf );
        glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                      (sizeof(GLfloat)*6+sizeof(GLuint))*m_compacted_alloc,
                      NULL,
                      GL_STREAM_DRAW );
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );
    }

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, subset->subsetAsTexture() );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->vertexPositionsAsBufferTexture() );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->cellCornerTexture() );

    glUseProgram( m_compact_program );
    glUniformMatrix4fv( m_compact_local_to_world_loc, 1, GL_FALSE, world_from_local );

    glEnable( GL_RASTERIZER_DISCARD );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_compacted_buf );
    glBeginQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_compacted_count_query );
    glBeginTransformFeedback( GL_POINTS );
    glDrawArrays( GL_POINTS, 0, tess->cellCount() );
    glEndTransformFeedback();
    glEndQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN );
    glDisable( GL_RASTERIZER_DISCARD );

    GLuint compacted_count;
    glGetQueryObjectuiv( m_compacted_count_query,
                         GL_QUERY_RESULT,
                         &compacted_count );

    // populate slices
    glBindVertexArray( m_compacted_vao );
    glBindFramebuffer( GL_FRAMEBUFFER, m_slice_fbo );

    // make sure that the outer rim of voxels are always empty
    glViewport( 1, 1, m_resolution-2, m_resolution-2 );
    glUseProgram( m_voxelizer_program );

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );
    glClearColor( 0.f, 0.f, 0.f, 0.f );
    for(GLsizei i=0; i<m_resolution; i++ ) {
        glFramebufferTexture3D( GL_FRAMEBUFFER,
                                GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_3D,
                                m_voxels_tex,
                                0,
                                i );
        CHECK_FBO;
        glClear( GL_COLOR_BUFFER_BIT );
        if( (0 < i) && (i+1<m_resolution) ) {
            glUniform2f( m_voxelizer_slice_loc, (i-1.f)/(m_resolution-2.f), (i+0.f)/(m_resolution-2.f) );
            glDrawArrays( GL_POINTS, 0, compacted_count );
        }
    }
    glBindVertexArray( 0 );
    CHECK_GL;

}
