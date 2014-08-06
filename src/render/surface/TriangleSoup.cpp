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

#include "render/surface/TriangleSoup.hpp"

namespace render {
namespace surface {


TriangleSoup::TriangleSoup()
    : m_vertex_alloc(0),
      m_vertex_count(0),
      m_attributes( "TriangleSoup.m_attributes" ),
      m_attributes_qry( "TriangleSoup.m_attributes_qry" ),
      m_attributes_xfb( "TriangleSoup.m_attributes_xfb" ),
      m_attributes_vao( "TriangleSoup.m_attributes_vao" )
{
    resizeBuffersIfNeeded( 4 );

    static const GLfloat quad[ 4*9 ] = {
        1.f,  0.f, 0.f,   0.f, 1.f,  1.f,    1.f, -1.f, 0.f,
        1.f,  0.f, 0.f,   0.f, 1.f,  1.f,    1.f,  1.f, 0.f,
        1.f,  0.f, 0.f,   0.f, 1.f,  1.f,   -1.f, -1.f, 0.f,
        1.f,  0.f, 0.f,   0.f, 1.f,  1.f,   -1.f,  1.f, 0.f,
    };
    m_vertex_count = 4;
    
    glBindBuffer( GL_ARRAY_BUFFER, m_attributes.get() );
    glBufferData( GL_ARRAY_BUFFER,
                  4*9*sizeof(float),
                  quad,
                  GL_DYNAMIC_COPY );
   
    // --- set up vertex array object ------------------------------------------
    glBindVertexArray( m_attributes_vao.get() );

    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 9*sizeof(float), 0*sizeof(float) );
    glEnableVertexAttribArray( 0 );

    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 9*sizeof(float), (const GLvoid*)(3*sizeof(float)) );
    glEnableVertexAttribArray( 1 );

    glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, 9*sizeof(float), (const GLvoid*)(6*sizeof(float)) );
    glEnableVertexAttribArray( 2 );
 
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    // --- set up transform feedback -------------------------------------------
    /*
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, m_attributes_xfb.get() );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_cell_buffer.get() );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_vertex_ix_buf.get() );
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
   */
}

bool
TriangleSoup::resizeBuffersIfNeeded( const GLsizei vertices )
{
    if( vertices < m_vertex_alloc ) {
        m_vertex_alloc = 1.1f*vertices;
        glBindBuffer( GL_ARRAY_BUFFER, m_attributes.get() );
        glBufferData( GL_ARRAY_BUFFER,
                      9*sizeof(float)*m_vertex_alloc,
                      NULL,
                      GL_DYNAMIC_COPY );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        return true;
    }
    else {
        return false;
    }
    
    
    
    
}


} // namespace surfacae
} // namespace render
