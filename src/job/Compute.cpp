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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "dataset/CornerpointGrid.hpp"
#include "job/FRViewJob.hpp"
#include "utils/Logger.hpp"
#include "render/mesh/AbstractMeshGPUModel.hpp"
#include "render/mesh/CellSetInterface.hpp"
#include "render/mesh/PolyhedralMeshGPUModel.hpp"
#include "render/subset/Representation.hpp"
#include "render/GridField.hpp"
#include "render/subset/BuilderSelectAll.hpp"
#include "render/subset/BuilderSelectByFieldValue.hpp"
#include "render/subset/BuilderSelectByIndex.hpp"
#include "render/subset/BuilderSelectInsideHalfplane.hpp"
#include "render/subset/BuilderSelectOnPlane.hpp"
#include "render/ClipPlane.hpp"
#include "render/surface/GridTessSurf.hpp"
#include "render/surface/GridTessSurfBuilder.hpp"

void
FRViewJob::doCompute()
{
    Logger log = getLogger( "FRViewJob.doCompute" );
    if( !m_has_pipeline ) {
        if( m_has_context ) {
            if(!setupPipeline()) {
                return;
            }
        }
        else {
            LOGGER_DEBUG( log, "OpenGL context isn't initialized yet." );
        }
    }

    bool profile;
    m_model->getElementValue( "profile" , profile );

    for( size_t i=0; i<m_source_items.size(); i++ ) {
        boost::shared_ptr<SourceItem> source_item = m_source_items[i];
        if( source_item->m_do_update_subset ) {
            LOGGER_DEBUG( log, "Updating subset geometry of " << source_item->m_source->name() );
            source_item->m_do_update_subset = false;
            // we will probably change one or more surfaces, so renderlist must
            // be updated.
            try {
                LOGGER_DEBUG( log, "updating item " << i << " of " << m_source_items.size() << ": " << source_item->m_source->name() );
                
                bool flip_faces = false;
                if( source_item->m_appearance_data ) {
                    flip_faces = source_item->m_appearance_data->flipOrientation();
                }
                
                //            bool geometric_edges = false;
                //            std::string edge_render_mode;
                //            m_model->getElementValue( "edge_render_mode", edge_render_mode );
                //            if( edge_render_mode == "Geometric" ) {
                //                geometric_edges = true;
                //            }
                
                std::string subset;
                m_model->getElementValue( "surface_subset", subset );
                
                boost::shared_ptr<const render::mesh::CellSetInterface> cell_set
                        = boost::dynamic_pointer_cast<const render::mesh::CellSetInterface>( source_item->m_grid_tess );
                if( cell_set ) {
                    if( subset == "subset_index" ) {
                        
                        boost::shared_ptr< dataset::CellLayoutInterface > cell_layout =
                                boost::dynamic_pointer_cast< dataset::CellLayoutInterface >( source_item->m_source );
                        if( cell_layout ) {
                            int min_i, min_j, min_k, max_i, max_j, max_k;
                            m_model->getElementValue( "index_range_select_min_i", min_i );
                            m_model->getElementValue( "index_range_select_min_j", min_j );
                            m_model->getElementValue( "index_range_select_min_k", min_k );
                            m_model->getElementValue( "index_range_select_max_i", max_i );
                            m_model->getElementValue( "index_range_select_max_j", max_j );
                            m_model->getElementValue( "index_range_select_max_k", max_k );
                            m_index_selector->apply( source_item->m_grid_tess_subset,
                                                     cell_set,
                                                     cell_layout->maxIndex(0),
                                                     cell_layout->maxIndex(1),
                                                     cell_layout->maxIndex(2),
                                                     //nx(), cell_layout->ny(), cell_layout->nz(),
                                                     min_i, min_j, min_k,
                                                     max_i, max_j, max_k );
                            m_render_clip_plane = false;
                        }
                    }
                    else if( subset == "subset_field" ) {
                        if( source_item->m_grid_field ) {
                            double field_min, field_max;
                            m_model->getElementValue( "field_select_min", field_min );
                            m_model->getElementValue( "field_select_max", field_max );
                            m_field_selector->apply( source_item->m_grid_tess_subset,
                                                     cell_set,
                                                     source_item->m_grid_field,
                                                     field_min,
                                                     field_max );
                        }
                        else {
                            m_all_selector->apply( source_item->m_grid_tess_subset,
                                                   cell_set );
                        }
                        m_render_clip_plane = false;
                    }
                    else if( subset == "subset_plane" ) {
                        m_plane_selector->apply( source_item->m_grid_tess_subset,
                                                 cell_set,
                                                 glm::value_ptr( source_item->m_clip_plane->plane()*m_local_to_world ) );
                        m_render_clip_plane = true;
                    }
                    else if( subset == "subset_halfplane" ) {
                        m_half_plane_selector->apply( source_item->m_grid_tess_subset,
                                                      cell_set,
                                                      glm::value_ptr( source_item->m_clip_plane->plane()*m_local_to_world ) );
                        m_render_clip_plane = true;
                    }
                    else {
                        m_all_selector->apply( source_item->m_grid_tess_subset, cell_set );
                        m_render_clip_plane = false;
                    }
                }
                // -----------------------------------------------------------------
                
                // --- Extract surface ---------------------------------------------
                if( m_under_the_hood.profilingEnabled() ) {
                    m_under_the_hood.surfaceGenerateTimer().beginQuery();
                }
                
                source_item->m_subset_surface->setTriangleCount( 0 );
                source_item->m_boundary_surface->setTriangleCount( 0 );
                source_item->m_faults_surface->setTriangleCount( 0 );
                boost::shared_ptr<render::surface::GridTessSurf> subset_surf;
                if( m_visibility_mask & models::RenderConfig::VISIBILITY_MASK_SUBSET ) {
                    subset_surf = source_item->m_subset_surface;
                }
                boost::shared_ptr<render::surface::GridTessSurf> boundary_surf;
                if( m_visibility_mask & models::RenderConfig::VISIBILITY_MASK_BOUNDARY ) {
                    boundary_surf = source_item->m_boundary_surface;
                }
                boost::shared_ptr<render::surface::GridTessSurf> faults_surf;
                if( m_visibility_mask & models::RenderConfig::VISIBILITY_MASK_FAULTS ) {
                    faults_surf = source_item->m_faults_surface;
                }
                
                m_grid_tess_surf_builder->buildSurfaces( subset_surf,
                                                         boundary_surf,
                                                         faults_surf,
                                                         source_item->m_grid_tess_subset,
                                                         source_item->m_grid_tess,
                                                         flip_faces );
                if( m_under_the_hood.profilingEnabled() ) {
                    m_under_the_hood.surfaceGenerateTimer().endQuery();
                }
                // -----------------------------------------------------------------
                LOGGER_DEBUG( log, "...done" );
            }
            catch( std::runtime_error& e ) {
                LOGGER_ERROR( log, "While extracting subset: " << e.what() );
            }
            m_model->updateElement<int>( "renderlist", m_renderlist_db.bump() );
        }
    }
}
