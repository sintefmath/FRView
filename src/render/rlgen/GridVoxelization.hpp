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

    ~GridVoxelization();

    /** Get the 3D texture containing the voxel set. */
    GLuint
    voxelTexture() const { return m_voxels_tex; }

    /** Get voxel set dimension.
     *
     * \param dim  Pointer to an three-element int array where dimension will be stored.
     */
    void
    dimension( GLsizei* dim ) const;

    void
    apply(std::list<boost::shared_ptr<SourceItem> > items );
    
    /** Populate the voxel set. */
    void
    build(boost::shared_ptr<const mesh::CellSetInterface> cell_set,
           boost::shared_ptr<const subset::Representation> cell_subset,
           const GLfloat*                                  world_from_local );

protected:
    GLsizei     m_resolution;
    GLuint      m_voxels_tex;
    GLuint      m_slice_fbo;


    GLuint      m_compacted_buf;
    GLsizei     m_compacted_alloc;
    GLuint      m_compacted_count_query;
    GLuint      m_compacted_vao;

    GLuint      m_voxelizer_program;
    GLint       m_voxelizer_slice_loc;

    GLuint      m_compact_program;
    GLint       m_compact_local_to_world_loc;
};

    } // of namespace rlgen
} // of namespace render
