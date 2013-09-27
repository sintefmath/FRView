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
#include <vector>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include "render/ManagedGL.hpp"

namespace render {
    namespace wells {
        class Representation;

class WellRenderer : public boost::noncopyable
{
public:

    WellRenderer();

    ~WellRenderer();

    void
    render( GLsizei                             width,
            GLsizei                             height,
            const GLfloat*                      projection,
            const GLfloat*                      camera_from_world,
            const GLfloat*                      world_from_model,
            boost::shared_ptr<Representation>   wells );


protected:
    GLProgram   m_well_prog;
};

    } // of namespace wells
} // of namespace render
