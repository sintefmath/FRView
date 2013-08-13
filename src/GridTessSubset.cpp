/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include "GridTess.hpp"
#include "GridTessSubset.hpp"
#include <siut2/gl_utils/GLSLtools.hpp>


GridTessSubset::GridTessSubset()
    : m_cells_total( 0 ),
      m_primitive_count_query( 0 ),
      m_subset_buffer( 0 ),
      m_subset_texture( 0 )
{
    glGenBuffers( 1, &m_subset_buffer );
    glGenTextures( 1, &m_subset_texture );
    glGenQueries( 1, &m_primitive_count_query );

    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_subset_buffer );
    glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                  sizeof(GLuint)*1024,
                  NULL,
                  GL_STREAM_DRAW );
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );


    glBindTexture( GL_TEXTURE_BUFFER, m_subset_texture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_subset_buffer );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );


    CHECK_GL;
}

GridTessSubset::~GridTessSubset()
{
    if( m_subset_buffer != 0 ) {
        glDeleteBuffers( 1, &m_subset_buffer );
    }
    if( m_subset_texture != 0 ) {
        glDeleteTextures( 1, &m_subset_texture );
    }
    if( m_primitive_count_query != 0 ) {
        glDeleteQueries( 1, &m_primitive_count_query );
    }
    CHECK_GL;
}

void
GridTessSubset::populateBuffer( const GridTess* tess, GLuint transform_feedback_index )
{
    CHECK_GL;
    if( tess->cellCount() == 0 ) {
        return;
    }
    if( m_cells_total != (GLsizei)tess->cellCount() ) {
        m_cells_total = tess->cellCount();
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_subset_buffer );
        glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                      sizeof(GLuint)*((m_cells_total+31)/32),
                      NULL,
                      GL_STREAM_DRAW );
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );
    }

    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER,
                      transform_feedback_index,
                      m_subset_buffer );
    glEnable( GL_RASTERIZER_DISCARD );
    glBeginTransformFeedback( GL_POINTS );
    glDrawArrays( GL_POINTS, 0, (m_cells_total+31)/32 );
    glEndTransformFeedback( );
    glDisable( GL_RASTERIZER_DISCARD );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0 );


    CHECK_GL;
}

