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
#include "render/manager/OrderIndependentTransparency.hpp"

namespace render {
    namespace manager {
        namespace glsl {
            extern const std::string OrderIndependentTransparency_vs;
            extern const std::string OrderIndependentTransparency_fs;
        }
        static const std::string package = "render.screen.OrderIndependentTransparency";

OrderIndependentTransparency::OrderIndependentTransparency( const models::RenderConfig& appearance,
                                                            const GLsizei width,
                                                            const GLsizei height )
    : FragmentList( appearance, width, height )
{
    Logger log = getLogger( package + ".constructor" );
    
    
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
    glEnable( GL_BLEND );
    glDepthMask( GL_FALSE );
    glBlendFunc( GL_ONE, GL_SRC_ALPHA );
    
    glUseProgram( m_fsq_prog.get() );
    glBindImageTexture( 0, m_fragment_rgba_tex.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F );
    glBindImageTexture( 1, m_fragment_node_tex.get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32I );
    glBindImageTexture( 2, m_fragment_head_tex.get(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I );

    glBindBufferBase( GL_UNIFORM_BUFFER, 0, m_status_ubo.get() );
    
    glBindVertexArray( m_fsq_vao.get() );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );    

    glBindBufferBase( GL_UNIFORM_BUFFER, 0, 0 );
    glBindVertexArray( 0 );
    glUseProgram( 0 ); 

    glEnable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );
    glDepthMask( GL_TRUE );
}


    } // of namespace screen
} // of namespace render
