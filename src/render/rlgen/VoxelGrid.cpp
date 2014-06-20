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


#include "render/rlgen/VoxelGrid.hpp"

namespace {
    const std::string package = "render.GridVoxelization";
}

namespace render {
    namespace rlgen {

GridVoxelization::GridVoxelization( GLsizei resolution )
    : m_resolution( resolution )
{
    glBindTexture( GL_TEXTURE_3D, m_voxels_tex.get() );
    
    //GL_RGB5_A1 might be a good fit since we only need 1 bit of alpha.
    glTexImage3D( GL_TEXTURE_3D, 0, GL_RGBA,
                  m_resolution, m_resolution, m_resolution, 0,
                  GL_RGBA , GL_FLOAT, NULL );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glBindTexture( GL_TEXTURE_3D, 0 );
}


bool
GridVoxelization::hasDimension( GLsizei resolution )
{
    return resolution == m_resolution;
}

void
GridVoxelization::dimension( GLsizei* dim ) const
{
    dim[0] = m_resolution;
    dim[1] = m_resolution;
    dim[2] = m_resolution;
}


    } // of namespace rlgen
} // of namespace render
