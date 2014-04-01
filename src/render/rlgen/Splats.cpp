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
#include "render/rlgen/Splats.hpp"
namespace {
const std::string pacakge = "render.rlgen.Splats";
}

namespace render {
namespace rlgen {


Splats::Splats()
    : m_alloc( 0 ),
      m_compacted( pacakge + ".m_compacted" ),
      m_query( pacakge + ".m_count" ),
      m_count( 0 ),
      m_update( true )
{
    resize( 100 );
}
    
Splats::~Splats()
{
}
    
bool
Splats::resize( GLsizei elements )
{
    GLsizei N = 1.05f*elements; // add 5% extra 
    if( m_alloc < N ) {
        m_alloc = N;
        glBindBuffer( GL_ARRAY_BUFFER, m_compacted.get() );
        glBufferData( GL_ARRAY_BUFFER,
                      (sizeof(GLfloat)*6+sizeof(GLuint))*m_alloc,
                      NULL,
                      GL_STREAM_DRAW );

        glBindVertexArray( m_attributes.get() );

        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6+sizeof(GLuint), NULL );
        glEnableVertexAttribArray( 0 );

        glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6+sizeof(GLuint), (GLvoid*)(sizeof(GLfloat)*3) );
        glEnableVertexAttribArray( 1 );

        glVertexAttribIPointer( 2, 1, GL_UNSIGNED_INT, sizeof(GLfloat)*6+sizeof(GLuint), (GLvoid*)(sizeof(GLfloat)*6) );
        glEnableVertexAttribArray( 2 );

        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindVertexArray( 0 );

        return true;
    }
    return false;
}
    
GLsizei
Splats::count()
{
    if( m_update ) {
        m_update = false;
        GLuint count;
        glGetQueryObjectuiv( m_query.get(), GL_QUERY_RESULT, &count );
        m_count = count;
    }
    return m_count;
}


} // of namespace rlgen
} // of namespace render
