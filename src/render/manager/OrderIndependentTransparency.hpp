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
#include "render/manager/FragmentList.hpp"

namespace render {
    namespace manager {

class OrderIndependentTransparency : public FragmentList
{
public:
    OrderIndependentTransparency( const models::RenderConfig& appearance,
                                  const GLsizei width,
                                  const GLsizei height );
    
    ~OrderIndependentTransparency();

/*    void
    render( GLuint                              fbo,
            const GLsizei                       width,
            const GLsizei                       height,
            const GLfloat*                      local_to_world,
            const GLfloat*                      modelview,
            const GLfloat*                      projection,
            boost::shared_ptr<const GridTess>   tess,
            boost::shared_ptr<const GridField>  field,
            const std::vector<RenderItem>&      items );    */

    
protected:
    GLProgram           m_fsq_prog;

    void
    processFragments( GLuint        fbo,
                      const GLsizei width,
                      const GLsizei height );

};
    
    } // of namespace screen
} // of namespace render
