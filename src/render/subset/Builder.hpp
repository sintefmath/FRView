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
#include <memory>
#include <boost/shared_ptr.hpp>
#include <GL/glew.h>
#include <string>

namespace render {
    class GridField;
    namespace mesh {
        class AbstractMeshGPUModel;
        class CellSetInterface;
    }
    namespace subset {
        class Representation;

/** Base class for cell subset selectors.
 *
 * Since subclasses mostly differ in which shaders they invoke, they subclasses
 * are very thin. Common to them is applying a shader program with a vertex
 * shader that is captured, and this class provides compiling and initializing
 * such a program.
 *
 */
class Builder
{
protected:
    GLuint  m_program;  ///< Shader program with only a vertex shader and output set up for capture.

    Builder( const std::string& vs );

    virtual
    ~Builder();
};

    } // of namespace subset
} // of namespace render
