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

#include "utils/Logger.hpp"
#include "render/GridTess.hpp"
#include "render/subset/Representation.hpp"
#include "render/surface/GridTessSurf.hpp"

namespace render {
    namespace  surface {

GridTessSurf::GridTessSurf()
    : m_count( 0u ),
      m_alloc( 0u ),
      m_cell_buffer( "GridTessSurf.m_cell_buffer" ),
      m_cell_texture( "GridTessSurf.m_cell_texture" ),
      m_cornerpoint_index_buffer( "GridTessSurf.m_cornerpoint_index_buffer" ),
      m_indices_count_qry( "GridTessSurf.m_indices_count_qry" ),
      m_indices_xfb( "GridTessSurf.m_indices_xfb" )
{
    setTriangleCount( 1024 );

    glBindTexture( GL_TEXTURE_BUFFER, m_cell_texture.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32UI, m_cell_buffer.get() );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );

    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, m_indices_xfb.get() );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_cell_buffer.get() );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_cornerpoint_index_buffer.get() );
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
}

GridTessSurf::~GridTessSurf()
{
}

bool
GridTessSurf::setTriangleCount( const GLsizei count )
{
    m_count = count;
    if( m_count < m_alloc ) {
        return false;
    }
    else {
        m_alloc = 1.1f*m_count;
        glBindBuffer( GL_TEXTURE_BUFFER, m_cell_buffer.get() );
        glBufferData( GL_TEXTURE_BUFFER,
                      sizeof(GLuint)*4*m_alloc,
                      NULL,
                      GL_DYNAMIC_COPY );
        glBindBuffer( GL_TEXTURE_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_cornerpoint_index_buffer.get() );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                      sizeof(GLuint)*3*m_alloc,
                      NULL,
                      GL_DYNAMIC_COPY );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        return true;
    }
}

void
GridTessSurf::populatePolygonBuffer( const GridTess*        tess,
                                     GLsizei                N,
                                     GLuint                 tri_cell_index,
                                     GLuint                 tri_indices_index )
{
    Logger log = getLogger( "GridTessSurf.populateTriangleBuffer" );



    glEnable( GL_RASTERIZER_DISCARD );
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, m_indices_xfb.get() );

    for(uint i=0; i<2; i++) {
        glBeginQuery( GL_PRIMITIVES_GENERATED, m_indices_count_qry.get() );
        glBeginTransformFeedback( GL_POINTS );
        glDrawArrays( GL_POINTS, 0, N );
        //glDrawArrays( GL_POINTS, 0, tess->polygonCount() );
        glEndTransformFeedback( );
        glEndQuery( GL_PRIMITIVES_GENERATED );

        // check if buffer was large enough
        GLuint result;
        glGetQueryObjectuiv( m_indices_count_qry.get(),
                             GL_QUERY_RESULT,
                             &result );
        if( setTriangleCount( result ) ) {
            LOGGER_DEBUG( log, "Buffer can hold " << m_alloc
                          << " triangles, but surface contains " << m_count << ", resizing." );
        }
        else {
            break;
        }
    }
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
    glDisable( GL_RASTERIZER_DISCARD );

    // We do this at last to avoid messing with the bound shader program.
    //glBindTexture( GL_TEXTURE_BUFFER, m_cell_texture );
    //glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32UI, m_cell_buffer );
    //glTexParameteri( GL_TEXTURE_BUFFER, GL_TEXTURE_BASE_LEVEL, 0 );
    //glTexParameteri( GL_TEXTURE_BUFFER, GL_TEXTURE_MAX_LEVEL, 0 );
    //glBindTexture( GL_TEXTURE_BUFFER, 0 );

}

void
GridTessSurf::populateTriangleBuffer( const GridTess*  tess,
                                      GLuint           tri_cell_index,
                                      GLuint           tri_indices_index )
{
/*
    Logger log = getLogger( "GridTessSurf.populateTriangleBuffer" );

    glEnable( GL_RASTERIZER_DISCARD );
    for(uint i=0; i<2; i++) {
        // populate buffer
        glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, tri_cell_index, m_cell_buffer );
        glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, tri_indices_index, m_cornerpoint_index_buffer );

        glBeginQuery( GL_PRIMITIVES_GENERATED, m_indices_count_qry );
        glBeginTransformFeedback( GL_POINTS );
        glDrawArrays( GL_POINTS, 0, tess->triangleCount() );
        glEndTransformFeedback( );
        glEndQuery( GL_PRIMITIVES_GENERATED );

        // check if buffer was large enough
        GLuint result;
        glGetQueryObjectuiv( m_indices_count_qry,
                             GL_QUERY_RESULT,
                             &result );
        m_count = result;
        if( m_count <= m_alloc ) {
            break;
        }
        else {
            LOGGER_DEBUG( log, "Buffer can hold " << m_alloc
                          << " triangles, but surface contains " << m_count << ", resizing." );
            setTriangleCount( m_count );
        }
    }
    glDisable( GL_RASTERIZER_DISCARD );

    // We do this at last to avoid messing with the bound shader program.
    glBindTexture( GL_TEXTURE_BUFFER, m_cell_texture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32UI, m_cell_buffer );
    //glTexParameteri( GL_TEXTURE_BUFFER, GL_TEXTURE_BASE_LEVEL, 0 );
    //glTexParameteri( GL_TEXTURE_BUFFER, GL_TEXTURE_MAX_LEVEL, 0 );

    glBindTexture( GL_TEXTURE_BUFFER, 0 );
*/
}

    } // of namespace surface
} // of namespace render
