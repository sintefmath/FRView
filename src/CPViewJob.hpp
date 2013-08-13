/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once
#include <memory>
#include <list>
#include <glm/glm.hpp>
#include <tinia/jobcontroller/Controller.hpp>
#include <tinia/jobcontroller/OpenGLJob.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/renderlist/DataBase.hpp>
#include "TimerQuery.hpp"
#include "PerfTimer.hpp"

template<typename REAL> class Project;

class ClipPlane;
class GridTess;
class GridTessSubset;
class GridTessSurf;
class GridTessSurfBuilder;
class GridField;
class GridCubeRenderer;
class GridTessSurfRenderer;
class AllSelector;
class FieldSelector;
class IndexSelector;
class PlaneSelector;
class HalfPlaneSelector;
class TextRenderer;
class WellRenderer;
class CoordSysRenderer;
class GridVoxelization;
class VoxelSurface;
class ASyncReader;

class CPViewJob
        : public tinia::jobcontroller::OpenGLJob,
          public tinia::model::StateListener
{
public:
    CPViewJob( const std::list<std::string>& files );

    ~CPViewJob();

    void stateElementModified( tinia::model::StateElement *stateElement );

    bool init();

    bool
    initGL();

    bool renderFrame( const std::string&  session,
                      const std::string&  key,
                      unsigned int        fbo,
                      const size_t        width,
                      const size_t        height );

    const tinia::renderlist::DataBase*
    getRenderList( const std::string& session, const std::string& key );

    void
    initRenderList();

    void
    updateRenderList( );


private:
    std::shared_ptr<ASyncReader>        m_async_reader;
    std::shared_ptr<Project<float> >    m_project;
    bool                    m_cares_about_renderlists;
    bool                    m_policies_changed;
    bool                    m_renderlist_rethink;
    tinia::renderlist::DataBase m_renderlist_db;
    TextRenderer*           m_well_labels;
    ClipPlane*              m_clip_plane;
    GridTess*               m_grid_tess;
    GridTessSubset*         m_grid_tess_subset;
    GridTessSurf*           m_faults_surface;
    bool                    m_faults_surface_tainted;
    GridTessSurf*           m_subset_surface;
    bool                    m_subset_surface_tainted;
    GridTessSurf*           m_boundary_surface;
    bool                    m_boundary_surface_tainted;
    GridTessSurfBuilder*    m_grid_tess_surf_builder;
    GridField*              m_grid_field;
    GridTessSurfRenderer*   m_tess_renderer;
    AllSelector*            m_all_selector;
    FieldSelector*          m_field_selector;
    IndexSelector*          m_index_selector;
    PlaneSelector*          m_plane_selector;
    HalfPlaneSelector*      m_half_plane_selector;
    GridCubeRenderer*       m_grid_cube_renderer;
    WellRenderer*           m_well_renderer;
    CoordSysRenderer*       m_coordsys_renderer;
    GridVoxelization*       m_grid_voxelizer;
    VoxelSurface*           m_voxel_surface;

    PerfTimer               m_profile_update_timer;
    TimerQuery              m_profile_proxy_gen;
    TimerQuery              m_profile_surface_gen;
    TimerQuery              m_profile_surface_render;

    bool                    m_load_geometry;
    bool                    m_load_color_field;
    bool                    m_has_color_field;
    bool                    m_do_update_subset;
    bool                    m_do_update_renderlist;
    bool                    m_care_about_updates;
    bool                    m_render_clip_plane;
    unsigned int            m_solution_index;
    unsigned int            m_report_step_index;



    float                   m_proxy_box_min[3];
    float                   m_proxy_box_max[3];
    float                   m_proxy_transform[16];

    glm::mat4               m_bbox_to_world;
    glm::mat4               m_bbox_from_world;

    glm::mat4               m_local_to_world;
    glm::mat4               m_local_from_world;
    glm::mat4               m_proxy_to_world;
    glm::mat4               m_proxy_from_world;


    /** Fetches data, if needed. */
    void
    fetchData();

    /** Performs the GPGPU passes, if needed. */
    void
    doCompute();

    void
    updateModelMatrices();

    void
    updateProxyMatrices();

    void
    render( const float* projection,
            const float* modelview,
            unsigned int        fbo,
            const size_t        width,
            const size_t        height );

#ifdef DEBUG_GRAPHICS
    std::vector<float>      m_debug_points;
#endif

    bool
    handleButtonClick( tinia::model::StateElement *stateElement );

    void
    triggerRedraw( const std::string& viewer_key );

};
