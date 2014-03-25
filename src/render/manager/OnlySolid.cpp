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
#include "render/manager/OnlySolid.hpp"


namespace render {
    namespace manager {
        namespace glsl {
            extern const std::string OnlySolid_geo_fs;
        }


TransparencyNone::TransparencyNone( const models::Appearance &appearance,
                                    const GLsizei width,
                                    const GLsizei height )
    : AbstractBase( appearance, width, height ),
      m_surface_renderer( defines(), glsl::OnlySolid_geo_fs )
{}
    
TransparencyNone::~TransparencyNone()
{
}


void
TransparencyNone::render(GLuint                              fbo,
                          const GLsizei                       width,
                          const GLsizei                       height,
                          const GLfloat*                      local_to_world,
                          const GLfloat*                      modelview,
                          const GLfloat*                      projection,
                          const std::vector<RenderItem>&      items )
{
    if( (m_width != width) || (m_height != height) ) {
        m_width = width;
        m_height = height;
    }
    glm::mat4 M = glm::make_mat4( modelview ) * glm::make_mat4( local_to_world );
    
    glViewport( 0, 0, m_width, m_height );

    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );
    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );
    
    m_surface_renderer.draw( glm::value_ptr( M ), projection,
                             m_width, m_height,
                             m_color_map, items );
    renderMiscellaneous( width, height,
                         local_to_world, modelview, projection,
                         items );
    renderOverlay( width, height,
                   local_to_world, modelview, projection,
                   items );   
}

    } // of namespace screen
} // of namespace render
