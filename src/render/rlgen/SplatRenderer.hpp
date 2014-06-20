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
#include <list>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

#include "job/SourceItem.hpp"
#include "render/ManagedGL.hpp"

namespace render {

namespace rlgen {
    class GridVoxelization;

    
/** Renders splat descriptions into the layers of a voxel field, creating a voxelization. */
class SplatRenderer
        : public boost::noncopyable
{
public:
    SplatRenderer();
    
    void
    apply( boost::shared_ptr<GridVoxelization>               target,
           const std::list<boost::shared_ptr<SourceItem> >&  items );
    
protected:
    GLFramebuffer   m_slice_fbo;
    GLProgram       m_program;
    GLint           m_loc_slice;
    GLint           m_loc_field_remap;
    GLint           m_loc_use_field;
    GLint           m_loc_log_map;
    GLint           m_loc_surface_color;
};
            
        
    } // of namespace rlgen
} // of namespace render
