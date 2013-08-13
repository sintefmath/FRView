/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include "Logger.hpp"
#include "GridTess.hpp"
#include "GridTessSubset.hpp"
#include "GridTessSurf.hpp"

GridTessSurf::GridTessSurf()
    : m_render_vao( 0 ),
      m_tri_cell_buffer( 0u ),
      m_tri_cell_texture( 0u ),
      m_tri_indices_buffer( 0u ),
      m_tri_count_query( 0u ),
      m_tri_count( 0u ),
      m_tri_alloc( 0u )
{
    glGenVertexArrays( 1, &m_render_vao );
    glGenBuffers( 1, &m_tri_cell_buffer );
    glGenTextures( 1, &m_tri_cell_texture );
    glGenBuffers( 1, &m_tri_indices_buffer );
    glGenQueries( 1, &m_tri_count_query );
}

GridTessSurf::~GridTessSurf()
{
    if( m_render_vao != 0 ) {
        glDeleteVertexArrays( 1, &m_render_vao );
    }
    if( m_tri_cell_buffer != 0 ) {
        glDeleteBuffers( 1, &m_tri_cell_buffer );
    }
    if( m_tri_cell_texture != 0 ) {
        glDeleteTextures( 1, &m_tri_cell_texture );
    }
    if( m_tri_indices_buffer != 0 ) {
        glDeleteBuffers( 1, &m_tri_indices_buffer );
    }
    if( m_tri_count_query != 0 ) {
        glDeleteQueries( 1, &m_tri_count_query );
    }
}

/*
void
GridTessSurf::populateBuffer( const GridTessSubset*  subset,
                              GLuint                 tri_cell_index,
                              GLuint                 tri_indices_index )
{
    populateBuffer( &(subset->tessellation()),
                    tri_cell_index,
                    tri_indices_index );
}
*/

void
GridTessSurf::populateBuffer( const GridTess*  tess,
                              GLuint           tri_cell_index,
                              GLuint           tri_indices_index )
{
    Logger log = getLogger( "GridTessSurf.populateBuffer" );

    m_tri_count = 0;
    for(int i=0; i<100; i++) {

        // alloc more space ?
        if( m_tri_alloc == 0 || m_tri_count >= m_tri_alloc ) {
            m_tri_alloc = std::min( tess->triangleCount(),
                                    m_tri_alloc + std::max( (GLsizei)1000u, (GLsizei)(0.01f*tess->triangleCount())) );
            LOGGER_DEBUG( log, "Allocated space for " << m_tri_alloc << " triangles" );
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_tri_cell_buffer );
            glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                          sizeof(GLuint)*4*m_tri_alloc,
                          NULL,
                          GL_STREAM_DRAW );

            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_tri_indices_buffer );
            glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                          sizeof(GLuint)*3*m_tri_alloc,
                          NULL,
                          GL_STREAM_DRAW );
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );


        }

        // populate buffer
        glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, tri_cell_index, m_tri_cell_buffer );
        glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, tri_indices_index, m_tri_indices_buffer );

        glBeginQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_tri_count_query );

        glEnable( GL_RASTERIZER_DISCARD );
        glBeginTransformFeedback( GL_POINTS );

        glDrawArrays( GL_POINTS, 0, tess->triangleCount() );

        glEndTransformFeedback( );
        glDisable( GL_RASTERIZER_DISCARD );

        glEndQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN );

        // check if buffer was large enough
        GLuint result;
        glGetQueryObjectuiv( m_tri_count_query,
                             GL_QUERY_RESULT,
                             &result );
        m_tri_count = result;
        if( m_tri_count < m_tri_alloc ) {
            break;
        }
    }

    if( !(m_tri_count < m_tri_alloc) ) {
        LOGGER_ERROR( log, "Failed to extract surface in 100 iterations" );
    }

    // We do this at last to avoid messing with the bound shader program.
    glBindTexture( GL_TEXTURE_BUFFER, m_tri_cell_texture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32UI, m_tri_cell_buffer );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
}
