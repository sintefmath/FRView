#pragma once
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

#include <memory>
#include <list>
#include <glm/glm.hpp>
#include <tinia/jobcontroller/Controller.hpp>
#include <tinia/jobcontroller/OpenGLJob.hpp>
#include <tinia/model/StateListener.hpp>
#include <tinia/renderlist/DataBase.hpp>
#include "utils/PerfTimer.hpp"
#include "models/Appearance.hpp"
#include "models/Logic.hpp"
#include "models/RenderConfig.hpp"
#include "models/GridStats.hpp"
#include "models/File.hpp"
#include "models/UnderTheHood.hpp"
#include "models/SourceSelector.hpp"
#include "models/SubsetSelector.hpp"
#include "render/TimerQuery.hpp"
#include "job/SourceItem.hpp"


class ASyncReader;
namespace render {
    namespace subset {
        class BuilderSelectAll;
        class BuilderSelectByFieldValue;
        class BuilderSelectByIndex;
        class BuilderSelectOnPlane;
        class BuilderSelectInsideHalfplane;
    }

    class GLTexture;
    class GridCubeRenderer;
    class TextRenderer;
    class CoordSysRenderer;
    namespace wells {
        class WellRenderer;
    }
    namespace surface {
        class GridTessSurfBuilder;
    }
    namespace rlgen {
        class SplatCompacter;
        class GridVoxelization;
        class VoxelSurface;
        class SplatRenderer;
    }
    namespace manager {
        class AbstractBase;
    }
} // of namespace render
namespace dataset {
    class AbstractDataSource;
}
namespace bridge {
    class AbstractMeshBridge;
}

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
    setSource( size_t index );

    void
    addSource( boost::shared_ptr<dataset::AbstractDataSource >        source,
               const std::string&                                     source_file,
               boost::shared_ptr<render::mesh::AbstractMeshGPUModel>  gpu_mesh );

    void
    cloneSource();

    void
    deleteSource();

    void
    deleteAllSources();

    void
    loadFile(const std::string& filename);

    
    // Returns true if there is a valid source item
    bool
    currentSourceItemValid() const;
    
    // might throw runtime_error if no item
    boost::shared_ptr<SourceItem>
    currentSourceItem();

    // might throw runtime_error if no item
    const boost::shared_ptr<SourceItem>
    currentSourceItem() const;
    
private:
    std::vector<boost::shared_ptr<SourceItem> >     m_source_items;
    size_t                                          m_current_item;


    models::SourceSelector                          m_source_selector;
    models::SubsetSelector                          m_subset_selector;
    models::Appearance                              m_appearance;
    models::UnderTheHood                            m_under_the_hood;
    models::RenderConfig                              m_renderconfig;
    //models::RenderConfig::VisibilityMask              m_visibility_mask;
    models::RenderConfig::Theme                       m_theme;
    models::GridStats                               m_grid_stats;
    float                                           m_zscale;
    /** True if job initGL has been called, and glew is set up. */
    bool                                            m_has_context;
    boost::shared_ptr<ASyncReader>                    m_async_reader;
    bool                                            m_check_async_reader;

    /** True if we want to enable OpenGL debug messages ourselves. */
    bool                                            m_enable_gl_debug;
    
    /** @{ */
    //boost::shared_ptr<dataset::AbstractDataSource >                m_project;
    /** @} */

    /** @{ */
    /** True if observer has asked at least once for renderlists. */
    bool                                            m_renderlist_initialized;

    /** True if e.g. clip plane has changed and needs to be updated. */
    bool                                            m_renderlist_update_revision;
    tinia::renderlist::DataBase                     m_renderlist_db;
    /** @} */

    /** @{ */
    /** True if all GPU objects haJob.handleFetchSource - Updated grid tesss been built. */

    
    bool                                                            m_has_pipeline;
    boost::shared_ptr<render::surface::GridTessSurfBuilder>         m_grid_tess_surf_builder;
    boost::shared_ptr<render::subset::BuilderSelectAll>             m_all_selector;
    boost::shared_ptr<render::subset::BuilderSelectByFieldValue>    m_field_selector;
    boost::shared_ptr<render::subset::BuilderSelectByIndex>         m_index_selector;
    boost::shared_ptr<render::subset::BuilderSelectOnPlane>         m_plane_selector;
    boost::shared_ptr<render::subset::BuilderSelectInsideHalfplane> m_half_plane_selector;
    boost::shared_ptr<render::GridCubeRenderer>                     m_grid_cube_renderer;
    boost::shared_ptr<render::CoordSysRenderer>                     m_coordsys_renderer;
    boost::shared_ptr<render::rlgen::SplatCompacter>                m_splat_compacter;
    boost::shared_ptr<render::rlgen::GridVoxelization>              m_voxel_grid;
    boost::shared_ptr<render::rlgen::SplatRenderer>                 m_splat_renderer;
    boost::shared_ptr<render::rlgen::VoxelSurface>                  m_voxel_surface;
    boost::shared_ptr<render::manager::AbstractBase>                 m_screen_manager;

    // Color map that can be used by items
    boost::shared_ptr<render::GLTexture>                            m_color_maps;
    
    /** @} */


    bool                                            m_care_about_updates;
    bool                                            m_render_clip_plane;
    bool                                            m_create_nonindexed_geometry;
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
    updateCurrentFieldData();

    void
    releasePipeline();

    bool
    setupPipeline();


    /** Fetches data, if needed. */
    void
    issueFieldFetch();

    void
    fetchData();
    
    void
    handleFetchSource();
    
    void
    handleFetchField();

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

    void
    triggerRedraw( const std::string& viewer_key );

};
