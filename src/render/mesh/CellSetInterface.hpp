#pragma once
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
#include <GL/glew.h>
#include <string>
#include <vector>
#include <boost/utility.hpp>

namespace render {
namespace mesh {

class CellSetInterface
{
public:
    virtual
    ~CellSetInterface();
    
    /** Get the number of cells in the grid. */
    virtual
    GLsizei
    cellCount() const = 0;
    
    /** Get global cell index through GL_R32UI buffer texture. */
    virtual
    GLuint
    cellGlobalIndexTexture() const = 0;
    
    /** Get the vertex indices spanning a cell through a GL_RGBA32UI buffer texture, two uvec4 per cell. */
    virtual
    GLuint
    cellCornerTexture() const = 0;
    
    /** Get global cell index stored in host memory. */
    virtual
    const std::vector<GLuint>&
    cellGlobalIndicesInHostMemory() const = 0;
    
};


} // of namespace mesh
} // of namespace render