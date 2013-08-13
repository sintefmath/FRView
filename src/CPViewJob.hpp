/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once
#include <glm/glm.hpp>
#include <list>
#include <tinia/jobobserver/Observer.hpp>
#include <tinia/jobobserver/OpenGLJob.hpp>
#include <tinia/policy/StateListener.hpp>
#include <tinia/renderlist/DataBase.hpp>


template<typename REAL> class Project;

class ClipPlane;
class GridTess;
class GridTessSubset;
class GridTessSurf;
class GridTessSurfBuilder;
class GridField;
class GridCubeRenderer;
class GridTessSurfRenderer;
class GridTessSurfBBoxFinder;
class AllSelector;
class FieldSelector;
class IndexSelector;
class PlaneSelector;
class HalfPlaneSelector;
class TextRenderer;
class WellRenderer;

class CPViewJob
        : public tinia::jobobserver::OpenGLJob,
          public tinia::policy::StateListener
{
public:
    CPViewJob( const std::list<std::string>& files );

    ~CPViewJob();

    void stateElementModified( tinia::policy::StateElement *stateElement );

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
    Project<float>*         m_project;
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
    GridTessSurfBBoxFinder* m_bbox_finder;
    AllSelector*            m_all_selector;
    FieldSelector*          m_field_selector;
    IndexSelector*          m_index_selector;
    PlaneSelector*          m_plane_selector;
    HalfPlaneSelector*      m_half_plane_selector;
    GridCubeRenderer*       m_grid_cube_renderer;
    WellRenderer*           m_well_renderer;
    bool                    m_load_geometry;
    bool                    m_load_color_field;
    bool                    m_has_color_field;
    bool                    m_do_update_subset;
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
    handleButtonClick( tinia::policy::StateElement *stateElement );

    void
    triggerRedraw( const std::string& viewer_key );

};
