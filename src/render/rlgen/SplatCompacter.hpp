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
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include "render/ManagedGL.hpp"

namespace render {
    namespace mesh {
        class CellSetInterface;
        class VertexPositionInterface;
    }
    namespace subset {
        class Representation;
    }

    namespace rlgen {

    class Splats;
    
/** Converts a cell set to a compact set of slice-aligned splats. */
class SplatCompacter
        : public boost::noncopyable
{
public:
    SplatCompacter();
    
    ~SplatCompacter();
    
    void
    process( boost::shared_ptr<Splats>                               splats,
             boost::shared_ptr<const mesh::VertexPositionInterface>  vertices,
             boost::shared_ptr<const mesh::CellSetInterface>         cells,
             boost::shared_ptr<const subset::Representation>         subset,
             const GLfloat*                                          world_from_local );
    
protected:
    GLProgram   m_compacter;
    GLint       m_local_to_world_loc;
};


    } // of namespace rlgen
} // of namespace render
