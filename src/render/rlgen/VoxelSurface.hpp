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
#include <hpmc.h>
#include <vector>

namespace render {
    class GridVoxelization;
    class GridField;
    
    namespace rlgen {

class VoxelSurface
{
public:
    VoxelSurface();

    ~VoxelSurface();

    const std::vector<GLfloat>&
    surfaceInHostMem() const
    { return m_surface_host; }

    void
    build( boost::shared_ptr<const GridVoxelization> voxels,
           boost::shared_ptr<const GridField> field );

protected:
    GLsizei                 m_volume_dim[3];
    HPMCConstants*          m_hpmc_c;
    HPMCHistoPyramid*       m_hpmc_hp;
    HPMCTraversalHandle*    m_hpmc_th;

    GLuint                  m_extraction_program;
    GLuint                  m_extraction_loc_scale;
    GLuint                  m_extraction_loc_shift;

    GLsizei                 m_surface_alloc;    ///< Number of vertices currently allocated in \ref m_surface_buf.
    GLuint                  m_surface_buf;
    std::vector<GLfloat>    m_surface_host;
};

    } // of namespace rlgen
} // of namespace render
