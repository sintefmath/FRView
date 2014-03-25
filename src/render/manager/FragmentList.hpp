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

#pragma once
#include <GL/glew.h>
#include "render/surface/Renderer.hpp"
#include "render/manager/AbstractBase.hpp"

namespace render {
    namespace manager {

class FragmentList : public AbstractBase
{
public:
    FragmentList( const models::Appearance& appearance,
                  const GLsizei width,
                  const GLsizei height );
   
    ~FragmentList();

    virtual
    void
    render( GLuint                              fbo,
            const GLsizei                       width,
            const GLsizei                       height,
            const GLfloat*                      local_to_world,
            const GLfloat*                      modelview,
            const GLfloat*                      projection,
            const std::vector<RenderItem>&      items );    



protected:
    void
    resizeFragmentBuffers( );
    
    void
    resizeScreenSizedBuffers();
    
    virtual
    void
    processFragments( GLuint        fbo,
                      const GLsizei width,
                      const GLsizei height ) = 0;
    
    
    surface::Renderer   m_surface_renderer_solid;
    surface::Renderer   m_surface_renderer_transparent;
    GLint               m_texbuf_max_texels;
    GLint               m_fragment_alloc;   ///< Number of fragments allocated.
    GLint               m_fragment_alloc_loc;
    GLBuffer            m_status_ubo;
    GLBuffer            m_fragment_counter_buf; ///< Counts the fragments.
    GLBuffer            m_fragment_counter_readback_buf;
    GLBuffer            m_fragment_rgba_buf;    //
    GLTexture           m_fragment_rgba_tex;
    GLBuffer            m_fragment_node_buf;    // list next pointers RG32I
    GLTexture           m_fragment_node_tex;
    GLTexture           m_fragment_head_tex;    // width x height list headers, R32I
    GLFramebuffer       m_fragment_head_fbo;

};
    
    
    } // of namespace screen
} // of namespace render
