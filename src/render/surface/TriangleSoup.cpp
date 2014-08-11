/* Copyright STIFTELSEN SINTEF 2014
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
#include "render/surface/TriangleSoup.hpp"


namespace {
    const std::string package = "render.surface.TriangleSoup";
}

namespace render {
namespace surface {


TriangleSoup::TriangleSoup()
    : m_triangle_vtx_count(0),
      m_triangle_vtx_alloc(0),
      m_triangle_attributes( "TriangleSoup.m_attributes" ),
      m_attributes_xfb( "TriangleSoup.m_attributes_xfb" ),
      m_triangle_attributes_vao( "TriangleSoup.m_attributes_vao" )
{
    setTriangleAndEdgeVertexCount( 0, 0 );

    // --- set up vertex array object ------------------------------------------
    glBindBuffer( GL_ARRAY_BUFFER, m_triangle_attributes.get() );
    glBindVertexArray( m_triangle_attributes_vao.get() );

    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0*sizeof(float) );
    glEnableVertexAttribArray( 0 );

    glVertexAttribIPointer( 1, 1, GL_UNSIGNED_INT,   8*sizeof(float), (const GLvoid*)(3*sizeof(float)) );
    glEnableVertexAttribArray( 1 );

    glVertexAttribPointer( 2, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (const GLvoid*)(4*sizeof(float)) );
    glEnableVertexAttribArray( 2 );
 
    glBindBuffer( GL_ARRAY_BUFFER, m_edge_attributes.get() );
    glBindVertexArray( m_edge_attributes_vao.get() );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 0, 0 );
    glEnableVertexAttribArray( 0 );

    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    // --- set up transform feedback -------------------------------------------

    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, m_attributes_xfb.get() );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_triangle_attributes.get() );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_edge_attributes.get() );
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
}

bool
TriangleSoup::setTriangleAndEdgeVertexCount(const GLsizei triangle_vertices, const GLsizei edge_vertices )
{
    bool retval = false;

    Logger log = getLogger( package + ".setTriangleCount");
    m_triangle_vtx_count = triangle_vertices;

    if( m_triangle_vtx_count > m_triangle_vtx_alloc ) {
        m_triangle_vtx_alloc = std::max( 1024.f, 1.1f*m_triangle_vtx_count );
        GLsizei bytes = 10*sizeof(float)*m_triangle_vtx_alloc;

        LOGGER_DEBUG( log,
                      "Surface has " << m_triangle_vtx_count <<
                      " triangle vertices, resizing buffers to hold " << m_triangle_vtx_alloc <<
                      " triangle vertices (" << ((bytes/1024.f)/1024.f) << " MB).\n" );

        glBindBuffer( GL_ARRAY_BUFFER, m_triangle_attributes.get() );
        glBufferData( GL_ARRAY_BUFFER,
                      bytes,
                      NULL,
                      GL_DYNAMIC_COPY );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        retval = true;
    }

    m_edge_vtx_count = edge_vertices;
    if( m_edge_vtx_count > m_edge_vtx_alloc ) {
        m_edge_vtx_alloc = std::max( 1024.f, 1.1f*m_edge_vtx_count );
        GLsizei bytes = 4*sizeof(float)*m_edge_vtx_alloc;

        LOGGER_DEBUG( log,
                      "Surface has " << m_edge_vtx_count <<
                      " edge vertices, resizing buffers to hold " << m_edge_vtx_alloc <<
                      " edge vertices (" << ((bytes/1024.f)/1024.f) << " MB).\n" );

        glBindBuffer( GL_ARRAY_BUFFER, m_edge_attributes.get() );
        glBufferData( GL_ARRAY_BUFFER,
                      bytes,
                      NULL,
                      GL_DYNAMIC_COPY );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        retval = true;
    }

    return retval;
}


} // namespace surfacae
} // namespace render
