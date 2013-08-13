#pragma once
#include <hpmc.h>
#include <vector>

class GridVoxelization;
class GridField;

class VoxelSurface
{
public:
    VoxelSurface();

    ~VoxelSurface();

    const std::vector<GLfloat>&
    surfaceInHostMem() const
    { return m_surface_host; }

    void
    build( const GridVoxelization* voxels, const GridField* field );

protected:
    GLsizei                 m_volume_dim[3];
    HPMCConstants*          m_hpmc_c;
    HPMCIsoSurface*         m_hpmc_hp;
    HPMCIsoSurfaceRenderer* m_hpmc_th;

    GLuint                  m_extraction_program;
    GLuint                  m_extraction_loc_scale;
    GLuint                  m_extraction_loc_shift;

    GLsizei                 m_surface_alloc;    ///< Number of vertices currently allocated in \ref m_surface_buf.
    GLuint                  m_surface_buf;
    std::vector<GLfloat>    m_surface_host;
};
