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
#include <boost/utility.hpp>

namespace render {
    class TextRenderer;


class GridCubeRenderer : public boost::noncopyable
{
public:
    GridCubeRenderer();

    ~GridCubeRenderer();

    void
    setUnitCubeToObjectTransform( const GLfloat* transform );

    void
    render( const GLfloat* projection,
            const GLfloat* modelview );

protected:
    GLuint                  m_cube_vao;
    GLuint                  m_cube_pos_buf;
    GLuint                  m_cube_nrm_buf;

    GLuint                  m_grid_prog;

    TextRenderer*           m_text_renderer;

};

} // of namespace render
