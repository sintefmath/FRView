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


#include <boost/shared_ptr.hpp>
#include "models/Appearance.hpp"

// Forward decls of shared_ptr contents
namespace dataset {
    class AbstractDataSource;
}
namespace render {
    class GridField;
    class ClipPlane;

    class GLTexture;
    
    namespace mesh {
        class AbstractMeshGPUModel;
    }
    namespace subset {
        class Representation;
    }
    namespace surface {
        class GridTessSurf;
        class TriangleSoup;
    }
    namespace rlgen {
        class Splats;
    }
    namespace wells {
        class Representation;
    }
}
namespace models {
    class SubsetSelectorData;
    class AppearanceData;
}

struct SourceItem {
    SourceItem( boost::shared_ptr< dataset::AbstractDataSource >       source,
                const std::string&                                     source_file,
                boost::shared_ptr<render::mesh::AbstractMeshGPUModel>  gpu_mesh,
                boost::shared_ptr<render::GLTexture>                   color_map,
                const std::vector<boost::shared_ptr<SourceItem> >&     sources );


    boost::shared_ptr<dataset::AbstractDataSource>         m_source;
    std::string                                            m_source_file;
    boost::shared_ptr<render::ClipPlane>                   m_clip_plane;
    boost::shared_ptr<render::mesh::AbstractMeshGPUModel>  m_grid_tess;
    boost::shared_ptr<render::subset::Representation>      m_grid_tess_subset;
    boost::shared_ptr<render::surface::GridTessSurf>       m_faults_surface;
    boost::shared_ptr<render::surface::GridTessSurf>       m_subset_surface;
    boost::shared_ptr<render::surface::GridTessSurf>       m_boundary_surface;
    boost::shared_ptr<render::surface::TriangleSoup>       m_faults_surface_soup;
    boost::shared_ptr<render::surface::TriangleSoup>       m_subset_surface_soup;
    boost::shared_ptr<render::surface::TriangleSoup>       m_boundary_surface_soup;
    boost::shared_ptr<render::GridField>                   m_grid_field;
    boost::shared_ptr<render::wells::Representation>       m_wells;
    boost::shared_ptr<render::GLTexture>                   m_color_map;

    /** Compacted list of cell splat info used for render list creation. */
    boost::shared_ptr<render::rlgen::Splats>               m_splats;
    
    boost::shared_ptr<models::SubsetSelectorData>          m_subset_selector_data;
    boost::shared_ptr<models::AppearanceData>              m_appearance_data;
    
    
    std::string                                             m_name;

    /** Visibility mask used to generate geometry. */
    models::AppearanceData::VisibilityMask                  m_visibility_mask;
    bool                                            m_load_color_field;
    bool                                            m_do_update_subset;
    bool                                            m_do_update_renderlist;


    int                                                     m_field_num;
    int                                                     m_field_current;
    std::vector<std::string>                                m_field_names;
    int                                                     m_timestep_num;
    int                                                     m_timestep_current;

    
};
