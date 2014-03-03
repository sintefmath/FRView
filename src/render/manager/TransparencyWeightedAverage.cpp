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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "utils/Logger.hpp"
#include "utils/GLSLTools.hpp"
#include "render/manager/TransparencyWeightedAverage.hpp"


namespace render {

namespace manager {
        namespace glsl {
            extern const std::string TransparencyWeightedAverage_geo_fs;
            extern const std::string TransparencyWeightedAverage_vs;
            extern const std::string TransparencyWeightedAverage_fs;
        }
        static const std::string package = "render.screen.TransparencyWeightedAverage";

TransparencyWeightedAverage::TransparencyWeightedAverage( const models::Appearance& appearance,
                                                          const GLsizei width,
                                                          const GLsizei height )
    : AbstractBase( appearance, width, height ),
      m_surface_renderer( defines(), glsl::TransparencyWeightedAverage_geo_fs ),
      m_fbo_solid( package + ".m_fbo_solid" ),
      m_fbo_weighted_average_transparent( package + ".wgt_avg_transparent" ),
      m_solid_color_tex( package + ".m_solid_color" ),
      m_transparent_color_tex( package + ".m_transparent_color" ),
      m_transparent_complexity_tex( package + ".m_transparent_complexity" ),
      m_depth_tex( package + ".m_depth" ),
      m_fsq_vao( package + ".m_fsq_vao" ),
      m_fsq_buf( package + ".m_fsq_buf" ),
      m_merge_passes( package + ".m_merge_passes" )
{
    m_surface_renderer_solid_pass = glGetUniformLocation( m_surface_renderer.program().get(),
                                                          "solid_pass" );
    
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

    resize( width, height );
    buildShaders();
}

    
TransparencyWeightedAverage::~TransparencyWeightedAverage()
{
}

void
TransparencyWeightedAverage::buildShaders()
{
    Logger log = getLogger( package + ".buildShaders" );
    
    GLuint screen_common_vs = utils::compileShader( log, glsl::TransparencyWeightedAverage_vs, GL_VERTEX_SHADER );
    GLuint avg_alpha_fs = utils::compileShader( log, glsl::TransparencyWeightedAverage_fs, GL_FRAGMENT_SHADER );
    glAttachShader( m_merge_passes.get(), screen_common_vs );
    glAttachShader( m_merge_passes.get(), avg_alpha_fs );
    utils::linkProgram( log, m_merge_passes.get() );
    glDeleteShader( screen_common_vs );
    glDeleteShader( avg_alpha_fs );
}


void
TransparencyWeightedAverage::resize( const GLsizei width, const GLsizei height )
{
    Logger log = getLogger( package + ".resize" );
    if( (width > 0) && (height>0) ) {
        m_width = width;
        m_height = height;
    }
    else {
        m_width = 1;
        m_height = 1;
    }
    
    
    GLenum drawbuffers[2] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1
    };

    glBindTexture( GL_TEXTURE_2D, m_depth_tex.get() );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_DEPTH_COMPONENT, m_width, m_height, 0,
                  GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture( GL_TEXTURE_2D, m_solid_color_tex.get() );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_RGBA, m_width, m_height, 0,
                  GL_RGBA, GL_FLOAT, NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture( GL_TEXTURE_2D, m_transparent_color_tex.get() );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_RGBA16F, m_width, m_height, 0,
                  GL_RGBA, GL_FLOAT, NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture( GL_TEXTURE_2D, m_transparent_complexity_tex.get() );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_R16F, m_width, m_height, 0,
                  GL_RED, GL_FLOAT, NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture( GL_TEXTURE_2D, 0 );

    glBindFramebuffer( GL_FRAMEBUFFER, m_fbo_solid.get() );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, m_solid_color_tex.get(), 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_2D, m_depth_tex.get(), 0 );
    glDrawBuffers( 1, drawbuffers );
//    glDrawBuffer( GL_COLOR_ATTACHMENT0 );
    utils::checkFBO( log );

    /*
    glBindFramebuffer( GL_FRAMEBUFFER, m_fbo_weighted_sum_transparent );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, m_transparent_color_tex, 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_2D, m_depth_tex, 0 );
    utils::checkFBO( log );
*/

    glBindFramebuffer( GL_FRAMEBUFFER, m_fbo_weighted_average_transparent.get() );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, m_transparent_color_tex.get(), 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                            GL_TEXTURE_2D, m_transparent_complexity_tex.get(), 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_2D, m_depth_tex.get(), 0 );
    glDrawBuffers( 2, drawbuffers );
//    glDrawBuffer( GL_COLOR_ATTACHMENT0 );
    utils::checkFBO( log );    
}


void
TransparencyWeightedAverage::render( GLuint                              fbo,
                                     const GLsizei                       width,
                                     const GLsizei                       height,
                                     const GLfloat*                      local_to_world,
                                     const GLfloat*                      modelview,
                                     const GLfloat*                      projection,
                                     boost::shared_ptr<const GridTess>   tess,
                                     boost::shared_ptr<const GridField>  field,
                                     const std::vector<RenderItem>&      items )
{
    if( (m_width != width) || (m_height != height) ) {
        resize( width, height );
    }
    glm::mat4 M = glm::make_mat4( modelview ) * glm::make_mat4( local_to_world );
    
    glViewport( 0, 0, m_width, m_height );

    // Weighted average forumla, Bavoil & Myers, 'Order Independent
    // transparency with dual depth peeling', NVIDIA whitepaper.

    glBindFramebuffer( GL_READ_FRAMEBUFFER, fbo );
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_fbo_solid.get() );
    glBlitFramebuffer( 0, 0, m_width, m_height,
                       0, 0, m_width, m_height,
                       GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                       GL_NEAREST );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );
    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );

    glProgramUniform1i( m_surface_renderer.program().get(),
                        m_surface_renderer_solid_pass,
                        GL_TRUE );
    m_surface_renderer.draw( glm::value_ptr( M ),
                             projection,
                             m_width,
                             m_height,
                             tess, field, m_color_map, items );
    renderMiscellaneous( width, height,
                         local_to_world, modelview, projection,
                         items );
    
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_fbo_weighted_average_transparent.get() );
    glClearColor( 0.f, 0.f, 0.f, 0.f );
    glClear( GL_COLOR_BUFFER_BIT );
    glDepthMask( GL_FALSE );
    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );  // alpha * color done in geometry shader

    glProgramUniform1i( m_surface_renderer.program().get(),
                        m_surface_renderer_solid_pass,
                        GL_FALSE );
    m_surface_renderer.draw( glm::value_ptr( M ),
                             projection,
                             m_width,
                             m_height,
                             tess, field, m_color_map, items );

    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );

    // alpha average pass
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, fbo );
    glUseProgram( m_merge_passes.get() );
    glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_2D, m_solid_color_tex.get() );
    glActiveTexture( GL_TEXTURE1 ); glBindTexture( GL_TEXTURE_2D, m_transparent_color_tex.get() );
    glActiveTexture( GL_TEXTURE2 ); glBindTexture( GL_TEXTURE_2D, m_transparent_complexity_tex.get() );

    glBindVertexArray( m_fsq_vao.get() );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );    

    glBindVertexArray( 0 );
    glUseProgram( 0 );
    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glActiveTexture( GL_TEXTURE2 ); glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE1 ); glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_2D, 0 );
    glEnable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );
    glDepthMask( GL_TRUE );

    renderOverlay( width, height,
                   local_to_world, modelview, projection,
                   items );
}

    } // of namespace screen
} // of namespace render
