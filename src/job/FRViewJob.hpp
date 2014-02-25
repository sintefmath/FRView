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
#include <list>
#include <glm/glm.hpp>
#include <tinia/jobcontroller/Controller.hpp>
#include <tinia/jobcontroller/OpenGLJob.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/renderlist/DataBase.hpp>
#include "utils/PerfTimer.hpp"
#include "models/Logic.hpp"
#include "models/Appearance.hpp"
#include "models/GridStats.hpp"
#include "models/File.hpp"
#include "models/UnderTheHood.hpp"
#include "render/TimerQuery.hpp"

namespace dataset {
    template<typename REAL> class Project;
} // of namespace dataset

class ASyncReader;
namespace render {
    namespace subset {
        class Representation;
        class BuilderSelectAll;
        class BuilderSelectByFieldValue;
        class BuilderSelectByIndex;
        class BuilderSelectOnPlane;
        class BuilderSelectInsideHalfplane;
    }

    class ClipPlane;
    class GridTess;
    class GridField;
    class GridCubeRenderer;
    class TextRenderer;
    class CoordSysRenderer;
    namespace wells {
        class Representation;
        class WellRenderer;
    }
    namespace surface {
        class GridTessSurf;
        class GridTessSurfBuilder;
    }
    namespace rlgen {
        class GridVoxelization;
        class VoxelSurface;
    }
    namespace screen {
        class Transparency;
    }
} // of namespace render

class FRViewJob
        : public tinia::jobcontroller::OpenGLJob,
          public tinia::model::StateListener,
          public models::Logic
{
public:
    FRViewJob( const std::list<std::string>& files );

    ~FRViewJob();

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

    void
    doLogic();

    void
    loadFile( const std::string& filename,
              int refine_i,
              int refine_j,
              int refine_k,
              bool triangulate );

private:


    models::File                                    m_file;
    models::UnderTheHood                            m_under_the_hood;
    models::Appearance                              m_appearance;
    models::Appearance::VisibilityMask              m_visibility_mask;
    models::Appearance::Theme                       m_theme;
    models::GridStats                               m_grid_stats;
    float                                           m_zscale;
    /** True if job initGL has been called, and glew is set up. */
    bool                                            m_has_context;
    boost::shared_ptr<ASyncReader>                    m_async_reader;

    /** True if we want to enable OpenGL debug messages ourselves. */
    bool                                            m_enable_gl_debug;
    
    /** @{ */
    enum {
        PROJECT_NO_FILE,
        PROJECT_LOAD_GEOMETRY,
        PROJECT_LOAD_FIELD,
        PROJECT_UPDATE_SUBSET,
        PROJECT_OK
    }                                               m_project_state;
    boost::shared_ptr<dataset::Project<float> >                m_project;
    /** @} */

    /** @{ */
    /** True if observer has asked at least once for renderlists. */
    bool                                            m_renderlist_initialized;
    enum {
        /** Renderlist should be modified, trigger update from client. */
        RENDERLIST_CHANGED_NOTIFY_CLIENTS,
        /** Renderlist modification notified, do geometry creation. */
        RENDERLIST_CHANGED_CLIENTS_NOTIFIED,
        /** Client should have latest version of renderlist. */
        RENDERLIST_SENT
    }                                               m_renderlist_state;
    tinia::renderlist::DataBase                     m_renderlist_db;
    /** @} */

    /** @{ */
    /** True if all GPU objects has been built. */
    bool                                                            m_has_pipeline;
    boost::shared_ptr<render::ClipPlane>                            m_clip_plane;
    boost::shared_ptr<render::GridTess>                             m_grid_tess;
    boost::shared_ptr<render::subset::Representation>               m_grid_tess_subset;
    boost::shared_ptr<render::surface::GridTessSurf>                m_faults_surface;
    boost::shared_ptr<render::surface::GridTessSurf>                m_subset_surface;
    boost::shared_ptr<render::surface::GridTessSurf>                m_boundary_surface;
    boost::shared_ptr<render::surface::GridTessSurfBuilder>         m_grid_tess_surf_builder;
    boost::shared_ptr<render::GridField>                            m_grid_field;
    boost::shared_ptr<render::subset::BuilderSelectAll>             m_all_selector;
    boost::shared_ptr<render::subset::BuilderSelectByFieldValue>    m_field_selector;
    boost::shared_ptr<render::subset::BuilderSelectByIndex>         m_index_selector;
    boost::shared_ptr<render::subset::BuilderSelectOnPlane>         m_plane_selector;
    boost::shared_ptr<render::subset::BuilderSelectInsideHalfplane> m_half_plane_selector;
    boost::shared_ptr<render::GridCubeRenderer>                     m_grid_cube_renderer;
    boost::shared_ptr<render::wells::Representation>                m_wells;
    boost::shared_ptr<render::CoordSysRenderer>                     m_coordsys_renderer;
    boost::shared_ptr<render::rlgen::GridVoxelization>              m_grid_voxelizer;
    boost::shared_ptr<render::rlgen::VoxelSurface>                  m_voxel_surface;
    boost::shared_ptr<render::screen::Transparency>                 m_screen_manager;
    /** @} */


    bool                                            m_load_geometry;
    bool                                            m_load_color_field;
    bool                                            m_has_color_field;
    bool                                            m_do_update_subset;
    bool                                            m_care_about_updates;
    bool                                            m_render_clip_plane;
    unsigned int                                    m_solution_index;
    unsigned int                                    m_report_step_index;
    float                                           m_proxy_box_min[3];
    float                                           m_proxy_box_max[3];
    float                                           m_proxy_transform[16];
    glm::mat4                                       m_local_to_world;
    glm::mat4                                       m_local_from_world;
    glm::mat4                                       m_bbox_to_world;
    glm::mat4                                       m_bbox_from_world;
    glm::mat4                                       m_proxy_to_world;
    glm::mat4                                       m_proxy_from_world;


    void
    releasePipeline();

    bool
    setupPipeline();


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

    bool
    handleButtonClick( tinia::model::StateElement *stateElement );

    void
    triggerRedraw( const std::string& viewer_key );

};
