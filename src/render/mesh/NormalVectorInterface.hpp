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
#include <vector>

namespace render {
namespace mesh {

class NormalVectorInterface
{
public:
    virtual
    ~NormalVectorInterface();

    /** Get 4-component normal vectors through a GL_RGBA32F buffer texture. */
    virtual
    GLuint
    normalVectorsAsBufferTexture() const = 0;

    /** Get 4-component normal vectors stored in host memory. */
    virtual
    const std::vector<float>&
    normalVectorsInHostMemory() const = 0;
    
};


} // of namespace mesh
} // of namespace render