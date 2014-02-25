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
#include "utils/GLSLTools.hpp"
#include "render/screen/OrderIndependentTransparency.hpp"

namespace render {
    namespace screen {
        namespace glsl {
            extern const std::string OrderIndependentTransparency_vs;
            extern const std::string OrderIndependentTransparency_fs;
        }
        static const std::string package = "render.screen.OrderIndependentTransparency";

OrderIndependentTransparency::OrderIndependentTransparency( const GLsizei width, const GLsizei height )
    : FragmentList( width, height )
{
    Logger log = getLogger( package + ".constructor" );
    
    static const GLfloat quad[ 4*4 ] = {
         1.f, -1.f, 0.f, 1.f,
         1.f,  1.f, 0.f, 1.f,
        -1.f, -1.f, 0.f, 1.f,
        -1.f,  1.f, 0.f, 1.f
    };
    glBindVertexArray( m_fsq_vao.get() );
    glBindBuffer( GL_ARRAY_BUFFER, m_fsq_buf.get() );
    glBufferData( GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 0, NULL );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );
    
    GLuint vs = utils::compileShader( log, glsl::OrderIndependentTransparency_vs, GL_VERTEX_SHADER );
    GLuint fs = utils::compileShader( log, glsl::OrderIndependentTransparency_fs, GL_FRAGMENT_SHADER );
    
    glAttachShader( m_fsq_prog.get(), vs );
    glAttachShader( m_fsq_prog.get(), fs );

    utils::linkProgram( log, m_fsq_prog.get() );
    
    glDeleteShader( vs );
    glDeleteShader( fs );
}
    
OrderIndependentTransparency::~OrderIndependentTransparency()
{
}



void
OrderIndependentTransparency::processFragments( GLuint fbo,
                                                const GLsizei width,
                                                const GLsizei height )
{
    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glDisable( GL_DEPTH_TEST );
    //glEnable( GL_BLEND );
    glDepthMask( GL_FALSE );
    glBlendFunc( GL_ONE, GL_ONE );
    
    glUseProgram( m_fsq_prog.get() );
    glBindImageTexture( 0, m_fragment_rgba_tex.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F );
    glBindImageTexture( 1, m_fragment_node_tex.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32I );
    glBindImageTexture( 2, m_fragment_head_tex.get(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I );

    glBindVertexArray( m_fsq_vao.get() );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );    

    glBindVertexArray( 0 );
    glUseProgram( 0 ); 

    glEnable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );
    glDepthMask( GL_TRUE );
}


    } // of namespace screen
} // of namespace render
