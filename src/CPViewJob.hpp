/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once
#include <list>
#include <jobobserver/Observer.hpp>
#include <jobobserver/OpenGLJob.hpp>
#include <policylib/StateListener.hpp>



template<typename REAL> class Project;

class ClipPlane;
class GridTess;
class GridTessSubset;
class GridTessSurf;
class GridTessSurfBuilder;
class GridField;
class GridTessSurfRenderer;
class AllSelector;
class FieldSelector;
class IndexSelector;
class PlaneSelector;
class HalfPlaneSelector;
class TextRenderer;

class CPViewJob
        : public jobobserver::OpenGLJob,
          public policylib::StateListener
{
public:
    CPViewJob( const std::list<std::string>& files );

    ~CPViewJob();

    void stateElementModified( policylib::StateElement *stateElement );

    bool init();
    bool renderFrame( const std::string&  session,
                      const std::string&  key,
                      unsigned int        fbo,
                      const size_t        width,
                      const size_t        height );

    bool getRenderList( size_t &result_size,
                        char *result_buffer,
                        const size_t result_buffer_size,
                        const std::string &session,
                        const std::string &key,
                        const std::string &timestamp );
private:
    Project<float>*         m_project;
    ClipPlane*              m_clip_plane;
    TextRenderer*           m_text_renderer;
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
    bool                    m_load_geometry;
    bool                    m_load_color_field;
    bool                    m_has_color_field;
    bool                    m_do_update_subset;
    bool                    m_care_about_updates;
    bool                    m_render_clip_plane;
    unsigned int            m_solution_index;
    unsigned int            m_report_step_index;

    bool
    handleButtonClick( policylib::StateElement *stateElement );

    void
    triggerRedraw( const std::string& viewer_key );

};
