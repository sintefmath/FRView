/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once
#include <memory>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

namespace render {
    class GridTess;
    class GridTessSubset;

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

    /** Populate the voxel set. */
    void
    build( boost::shared_ptr<const GridTess> tess,
           boost::shared_ptr<const GridTessSubset> subset,
           const GLfloat* local_from_world );

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

} // of namespace render
