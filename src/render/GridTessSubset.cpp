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
#include <siut2/gl_utils/GLSLtools.hpp>

namespace render {


GridTessSubset::GridTessSubset()
    : m_cells_total( 0 ),
      m_subset_buffer( "GridTessSubset.m_subset_buffer" ),
      m_subset_texture( "GridTessSubset.m_subset_texture" )
{
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_subset_buffer.get() );
    glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                  sizeof(GLuint)*1024,
                  NULL,
                  GL_STREAM_DRAW );
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );

    glBindTexture( GL_TEXTURE_BUFFER, m_subset_texture.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_subset_buffer.get() );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
}

GridTessSubset::~GridTessSubset()
{
}

void
GridTessSubset::populateBuffer( boost::shared_ptr<const GridTess> tess,
                                GLuint transform_feedback_index )
{
    if( tess->cellCount() == 0 ) {
        return;
    }
    if( m_cells_total != (GLsizei)tess->cellCount() ) {
        m_cells_total = tess->cellCount();
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_subset_buffer.get() );
        glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                      sizeof(GLuint)*((m_cells_total+31)/32),
                      NULL,
                      GL_STREAM_DRAW );
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );
    }

    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER,
                      transform_feedback_index,
                      m_subset_buffer.get() );
    glEnable( GL_RASTERIZER_DISCARD );
    glBeginTransformFeedback( GL_POINTS );
    glDrawArrays( GL_POINTS, 0, (m_cells_total+31)/32 );
    glEndTransformFeedback( );
    glDisable( GL_RASTERIZER_DISCARD );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0 );
}

} // of namespace render

