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
    : m_triangle_count(0),
      m_triangle_alloc(0),
      m_attributes( "TriangleSoup.m_attributes" ),
      m_attributes_qry( "TriangleSoup.m_attributes_qry" ),
      m_attributes_xfb( "TriangleSoup.m_attributes_xfb" ),
      m_attributes_vao( "TriangleSoup.m_attributes_vao" )
{
    setTriangleCount( 0 );
/*
    static const GLfloat quad[ 4*10 ] = {
        1.f,  0.f, 0.f, 0.5f,   0.f, 1.f,  1.f,    1.f, -1.f, 0.f,
        1.f,  1.f, 0.f,  0.5f,  0.f, 1.f,  1.f,    1.f,  1.f, 0.f,
        0.f,  1.f, 0.f, 0.5f,   0.f, 1.f,  1.f,   -1.f, -1.f, 0.f,
        1.f,  1.f, 0.f, 0.5f,   0.f, 1.f,  1.f,   -1.f,  1.f, 0.f,
    };
    
    glBufferData( GL_ARRAY_BUFFER,
                  4*9*sizeof(float),
                  quad,
                  GL_DYNAMIC_COPY );
  */
    // --- set up vertex array object ------------------------------------------
    glBindBuffer( GL_ARRAY_BUFFER, m_attributes.get() );
    glBindVertexArray( m_attributes_vao.get() );

    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 10*sizeof(float), 0*sizeof(float) );
    glEnableVertexAttribArray( 0 );

    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 10*sizeof(float), (const GLvoid*)(4*sizeof(float)) );
    glEnableVertexAttribArray( 1 );

    glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, 10*sizeof(float), (const GLvoid*)(7*sizeof(float)) );
    glEnableVertexAttribArray( 2 );
 
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    // --- set up transform feedback -------------------------------------------

    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, m_attributes_xfb.get() );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_attributes.get() );
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
}

bool
TriangleSoup::setTriangleCount( const GLsizei triangles )
{
    Logger log = getLogger( package + ".setTriangleCount");
    m_triangle_count = triangles;
    if( m_triangle_count < m_triangle_alloc ) {
        return false;
    }
    else {
        m_triangle_alloc = std::max( 1024.f, 1.1f*m_triangle_count );
        GLsizei bytes = 10*3*sizeof(float)*m_triangle_alloc;

        LOGGER_DEBUG( log,
                      "Surface has " << m_triangle_count <<
                      " triangles, resizing buffers to hold " << m_triangle_alloc <<
                      " triangles (" << ((bytes/1024.f)/1024.f) << " MB).\n" );

        glBindBuffer( GL_ARRAY_BUFFER, m_attributes.get() );
        glBufferData( GL_ARRAY_BUFFER,
                      bytes,
                      NULL,
                      GL_DYNAMIC_COPY );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        return true;
    }
}


} // namespace surfacae
} // namespace render
