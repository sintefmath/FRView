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

#pragma once
#include <GL/glew.h>
#include "render/ManagedGL.hpp"
#include "render/surface/Renderer.hpp"
#include "render/screen/Transparency.hpp"

namespace render {
    namespace screen {

class TransparencyWeightedAverage : public Transparency
{
public:
    TransparencyWeightedAverage( const GLsizei width, const GLsizei height );
   
    ~TransparencyWeightedAverage();
    
    void
    render( GLuint                              fbo,
            const GLsizei                       width,
            const GLsizei                       height,
            const GLfloat*                      local_to_world,
            const GLfloat*                      modelview,
            const GLfloat*                      projection,
            boost::shared_ptr<const GridTess>   tess,
            boost::shared_ptr<const GridField>  field,
            const std::vector<RenderItem>&      items );
    
protected:
    surface::Renderer   m_surface_renderer;
    GLint               m_surface_renderer_solid_pass;
    
    GLFramebuffer       m_fbo_solid;
    GLFramebuffer       m_fbo_weighted_average_transparent;
    
    GLTexture           m_solid_color_tex;
    GLTexture           m_transparent_color_tex;
    GLTexture           m_transparent_complexity_tex;
    GLTexture           m_depth_tex;

    GLVertexArrayObject m_fsq_vao;
    GLBuffer            m_fsq_buf;

    GLProgram           m_merge_passes;

    void
    buildShaders();
    
    void
    resize( const GLsizei width, const GLsizei height );

};
    
    
    } // of namespace screen
} // of namespace render
