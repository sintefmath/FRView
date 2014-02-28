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

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "render/wells/Representation.hpp"
#include "render/screen/Transparency.hpp"


namespace render {
    namespace screen {


Transparency::~Transparency()
{
}


void
Transparency::renderMiscellaneous( const GLsizei                       width,
                                   const GLsizei                       height,
                                   const GLfloat*                      local_to_world,
                                   const GLfloat*                      modelview,
                                   const GLfloat*                      projection,
                                   const std::vector<RenderItem>&      items )
{
    glm::mat4 MVP =
            glm::make_mat4( projection ) *
            glm::make_mat4( modelview ) *
            glm::make_mat4( local_to_world );
            
            

    for( size_t i=0; i<items.size(); i++ ) {
        if( items[i].m_renderer == RenderItem::RENDERER_WELL ) {
            m_well_renderer.render( width,
                                    height,
                                    projection,
                                    modelview,
                                    local_to_world,
                                    items[i].m_well );
            items[i].m_well->wellHeads().render( width,
                                                    height,
                                                    glm::value_ptr( MVP ) );
        }
    }
    
}

void
Transparency::renderOverlay( const GLsizei                       width,
                             const GLsizei                       height,
                             const GLfloat*                      local_to_world,
                             const GLfloat*                      modelview,
                             const GLfloat*                      projection,
                             const std::vector<RenderItem>&      items )
{
    

}


    } // of namespace screen
} // of namespace render
