#include <cmath>
#include <stdlib.h>
#include "CellSelector.hpp"
#include "GridTess.hpp"
#include "GridField.hpp"
#include "GridTessBridge.hpp"
#include "Logger.hpp"
#include <siut/gl_utils/GLSLtools.hpp>
#include <siut/io_utils/snarf.hpp>

GridTess::GridTess()
{
    m_vertices.m_N = 0;
    m_triangles.m_index_count = 0;

    glGenVertexArrays( 1, &m_vertices.m_vao );
    glGenTextures( 1, &m_vertices.m_texture );
    glGenBuffers( 1, &m_vertices.m_vbo );
    glGenBuffers( 1, &m_tri_info.m_buffer );
    glGenTextures( 1, &m_tri_info.m_texture );
    glGenBuffers( 1, &m_triangles.m_ibo );
    glGenBuffers( 1, &m_cell_index.m_buffer );
    glGenTextures( 1, &m_cell_index.m_texture );
    glGenBuffers( 1, &m_cell_corner.m_buffer );
    glGenTextures( 1, &m_cell_corner.m_texture );

    glGenBuffers( 1, &m_cell_subset_buffer );
    glGenTextures( 1, &m_cell_subset_texture );

    glGenVertexArrays( 1, &m_triangle_vao );
    glGenBuffers( 1, &m_triangle_compact_cell_buffer );
    glGenBuffers( 1, &m_triangle_compact_indices_buffer );
    glGenTextures( 1, &m_triangle_compact_cell_texture );
    glGenQueries( 1, &m_triangle_compact_query );


    m_triangle_compact_prog = glCreateProgram();
    m_triangle_compact_vs = siut::gl_utils::compileShader(
                siut::io_utils::snarfFile( "shaders/triangle_compact_vs.glsl" ),
                GL_VERTEX_SHADER,
                true );
    glAttachShader( m_triangle_compact_prog, m_triangle_compact_vs );
    m_triangle_compact_gs = siut::gl_utils::compileShader(
                siut::io_utils::snarfFile( "shaders/triangle_compact_gs.glsl" ),
                GL_GEOMETRY_SHADER,
                true );
    glAttachShader( m_triangle_compact_prog, m_triangle_compact_gs );
    const char* triangle_varyings[2] = {
        "cell",
        "indices"
    };
    glTransformFeedbackVaryings( m_triangle_compact_prog, 2, triangle_varyings, GL_SEPARATE_ATTRIBS );
    siut::gl_utils::linkProgram( m_triangle_compact_prog );
    glUseProgram( m_triangle_compact_prog );
    glUniform1i( glGetUniformLocation( m_triangle_compact_prog, "cell_subset" ), 0 );
    glUseProgram( 0 );

}



void
GridTess::triangleCompactionPass()
{
    glBindVertexArray( m_triangle_vao );
    glUseProgram( m_triangle_compact_prog );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, m_cell_subset_texture );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_triangle_compact_cell_buffer );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_triangle_compact_indices_buffer );
    glEnable( GL_RASTERIZER_DISCARD );
    glBeginQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_triangle_compact_query );
    glBeginTransformFeedback( GL_POINTS );
    glDrawArrays( GL_POINTS, 0, m_triangles.m_index_count/3 );
    glEndTransformFeedback( );
    glEndQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN );
    glDisable( GL_RASTERIZER_DISCARD );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    CHECK_GL;

    // retrieve edge and triangle counts
    GLuint result;
    glGetQueryObjectuiv( m_triangle_compact_query,
                         GL_QUERY_RESULT,
                         &result );
    m_triangle_compact_count = result;

    Logger log = getLogger( "GridTess.triangleCompactionPass" );
    LOGGER_DEBUG( log, "count = " << result );

//    LOGGER_DEBUG( log, "complexity = " << (m_triangle_compact_count/pow(cells, (2.0/3.0) ) ) );

}


void
GridTess::update( unsigned int cells )
{
    triangleCompactionPass();
    do {
        if( m_triangle_compact_count < m_triangle_compact_allocated ) {
            break;
        }
        m_triangle_compact_allocated = std::min( m_triangles.m_index_count,
                                                 (GLsizei)(0.05f*m_triangles.m_index_count + m_triangle_compact_allocated) );
        allocTriangleCompactBuffers();
        triangleCompactionPass();
    }
    while( m_triangle_compact_count <  m_triangles.m_index_count );

    CHECK_GL;

}


GridTessBridge::GridTessBridge(GridTess &owner)
    : m_owner( owner )
{
}

GridTessBridge::~GridTessBridge()
{
    m_owner.import( *this );
}
void
GridTess::allocTriangleCompactBuffers()
{
    Logger log = getLogger( "GridTess.allocTriangleCompactBuffers" );



    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_triangle_compact_indices_buffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                  sizeof(GLuint)*3*m_triangle_compact_allocated,
                  NULL,
                  GL_STREAM_DRAW );

    LOGGER_DEBUG( log, "vertex index size = "
                  << ( (sizeof(GLuint)*3*m_triangle_compact_allocated + 1023)/1024 ) << " kb" );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    glBindBuffer( GL_TEXTURE_BUFFER, m_triangle_compact_cell_buffer );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GLuint)*m_triangle_compact_allocated,
                  NULL,
                  GL_STREAM_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );

    LOGGER_DEBUG( log, "cell index size = "
                  << ( (sizeof(GLuint)*m_triangle_compact_allocated + 1023)/1024 ) << " kb" );
    glBindTexture( GL_TEXTURE_BUFFER, m_triangle_compact_cell_texture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_triangle_compact_cell_buffer );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );

}

void
GridTess::import( GridTessBridge& bridge )
{
    Logger log = getLogger( "GridTess.import" );

    if( bridge.m_vertices.empty() ) {
        return;
    }


    for(unsigned int i=0; i<3; i++) {
        m_bb_min[i] = m_bb_max[i] = bridge.m_vertices[i];
    }
    for(unsigned int j=0; j<bridge.m_vertices.size()/4; j++) {
        for(unsigned int i=0; i<3; i++) {
            m_bb_min[i] = std::min( m_bb_min[i], bridge.m_vertices[4*j+i] );
            m_bb_max[i] = std::max( m_bb_max[i], bridge.m_vertices[4*j+i] );
        }
    }

    LOGGER_DEBUG( log, "bbox = ["
                  << m_bb_min[0] << ", " << m_bb_min[1] << ", " << m_bb_min[2] << "] x ["
                  << m_bb_max[0] << ", " << m_bb_max[1] << ", " << m_bb_max[2] << "]." );

    for(unsigned int i=0; i<3; i++) {
        m_scale[i] = 1.f/(m_bb_max[i]-m_bb_min[i]);
    }
    float xy_scale = std::min( m_scale[0], m_scale[1] );
    m_scale[0] = xy_scale;
    m_scale[1] = xy_scale;

    for(unsigned int i=0; i<3; i++) {
        m_shift[i] = -0.5f*(m_bb_max[i]+m_bb_min[i]) + 0.5f/m_scale[i];
    }


    for(unsigned int j=0; j<bridge.m_vertices.size()/4; j++) {
        for(unsigned int i=0; i<3; i++) {
            bridge.m_vertices[4*j+i] = m_scale[i]*(bridge.m_vertices[4*j+i]+m_shift[i]  );
        }
        bridge.m_vertices[4*j+3] = 1.f;
    }

    m_vertices.m_N = bridge.m_vertices.size()/4;
    if( m_vertices.m_N > 0 ) {
        glBindVertexArray( m_vertices.m_vao );
        glBindBuffer( GL_ARRAY_BUFFER, m_vertices.m_vbo );
        glBufferData( GL_ARRAY_BUFFER,
                      sizeof(float)*bridge.m_vertices.size(),
                      bridge.m_vertices.data(),
                      GL_STATIC_DRAW );
        glVertexPointer( 4, GL_FLOAT, 0, NULL );
        glEnableClientState( GL_VERTEX_ARRAY );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindVertexArray( 0 );

        glBindTexture( GL_TEXTURE_BUFFER, m_vertices.m_texture );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_vertices.m_vbo );
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
    }

    m_cells = bridge.m_cell_index.size();
    if( m_cells > 0 ) {
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_cell_subset_buffer );
        glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                      sizeof(GLuint)*((m_cells+31)/32),
                      NULL,
                      GL_STREAM_DRAW );
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );

        glBindTexture( GL_TEXTURE_BUFFER, m_cell_subset_texture );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_cell_subset_buffer );
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
    }


    m_triangles.m_index_count = bridge.m_triangles.size();
    if( m_triangles.m_index_count > 0 ) {
        glBindBuffer( GL_TEXTURE_BUFFER, m_tri_info.m_buffer );
        glBufferData( GL_TEXTURE_BUFFER,
                      sizeof(unsigned int)*bridge.m_triangle_info.size(),
                      bridge.m_triangle_info.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_TEXTURE_BUFFER, m_tri_info.m_buffer );
        glBindTexture( GL_TEXTURE_BUFFER, m_tri_info.m_texture );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RG32UI, m_tri_info.m_buffer );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_triangles.m_ibo );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                      sizeof(int)*bridge.m_triangles.size(),
                      bridge.m_triangles.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );


        glBindVertexArray( m_triangle_vao );
        glBindBuffer( GL_ARRAY_BUFFER, m_tri_info.m_buffer );
        glVertexAttribIPointer( 0, 2, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 0 );
        glBindBuffer( GL_ARRAY_BUFFER, m_triangles.m_ibo );
        glVertexAttribIPointer( 1, 3, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 1 );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindVertexArray( 0 );


        m_triangle_compact_allocated = 10* pow( m_cells, (2.0/3.0) );
        LOGGER_DEBUG( log, "cells = " << m_cells );
        LOGGER_DEBUG( log, "comp  = " << m_triangle_compact_allocated );

        allocTriangleCompactBuffers();
    }
    else {
        m_triangle_compact_allocated = 0;
    }





    // steal the contents of the cell_index buffer
    m_cell_index.m_body.swap( bridge.m_cell_index );

    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_index.m_buffer );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(unsigned int)*m_cell_index.m_body.size(),
                  m_cell_index.m_body.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_index.m_buffer );
    glBindTexture( GL_TEXTURE_BUFFER, m_cell_index.m_texture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_cell_index.m_buffer );


    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_corner.m_buffer );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(unsigned int)*bridge.m_cell_corner.size(),
                  bridge.m_cell_corner.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_corner.m_buffer );
    glBindTexture( GL_TEXTURE_BUFFER, m_cell_corner.m_texture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32UI, m_cell_corner.m_buffer );


    glBindTexture( GL_TEXTURE_BUFFER, 0 );


    CHECK_GL;
}
