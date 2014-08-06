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
#include <boost/utility.hpp>
#include "render/surface/TriangleSoup.hpp"

namespace render {
    class RenderItem;

namespace surface {

class TriangleSoupRenderer : public boost::noncopyable
{
public:
    TriangleSoupRenderer( const std::string& defines,
                          const std::string& fragment_source );
    
    void
    draw( const GLfloat*                    modelview,
          const GLfloat*                    projection,
          const GLsizei                     width,
          const GLsizei                     height,
          const std::vector<RenderItem>&    render_items );

    GLProgram&
    program() { return m_main; }
    
protected:
    GLProgram   m_main;
    GLint       m_loc_mvp;
    GLint       m_loc_mv;
    GLint       m_loc_nm;
    
};


} // namespace surfacae
} // namespace render
