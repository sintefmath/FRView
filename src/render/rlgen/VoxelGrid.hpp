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
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include "render/ManagedGL.hpp"
#include "job/SourceItem.hpp"

namespace render {
    namespace mesh {
        class CellSetInterface;
    }
    
    namespace subset {
        class Representation;
    }
    namespace rlgen {
        class Splats;

class GridVoxelization : public boost::noncopyable
{
public:
    GridVoxelization();

    /** Get the 3D texture containing the voxel set. */
    GLTexture&
    voxelTexture() { return m_voxels_tex; }

    /** Get the 3D texture containing the voxel set. */
    const GLTexture&
    voxelTexture() const { return m_voxels_tex; }

    /** Get voxel set dimension.
     *
     * \param dim  Pointer to an three-element int array where dimension will be stored.
     */
    void
    dimension( GLsizei* dim ) const;

protected:
    GLsizei     m_resolution;
    GLTexture   m_voxels_tex;
};

    } // of namespace rlgen
} // of namespace render
