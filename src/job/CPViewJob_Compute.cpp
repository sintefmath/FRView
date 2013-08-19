#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "CPViewJob.hpp"
#include "render/GridTess.hpp"
#include "Project.hpp"
#include "utils/Logger.hpp"
#include "render/GridTessSubset.hpp"
#include "render/GridTessSurf.hpp"
#include "render/GridTessSurfBuilder.hpp"
#include "render/GridTessSurfRenderer.hpp"
#include "render/GridField.hpp"
#include "render/CellSelector.hpp"
#include "render/ClipPlane.hpp"

void
CPViewJob::doCompute()
{
    Logger log = getLogger( "CPViewJob.doCompute" );
    if( !m_has_pipeline ) {
        return;
    }

    bool profile;
    m_model->getElementValue( "profile" , profile );

    if( m_do_update_subset  ) {
        m_do_update_subset = false;
        try {
            bool flip_faces;
//            bool geometric_edges = false;
//            std::string edge_render_mode;
//            m_model->getElementValue( "edge_render_mode", edge_render_mode );
//            if( edge_render_mode == "Geometric" ) {
//                geometric_edges = true;
//            }

            m_model->getElementValue( "tess_flip_orientation", flip_faces );

            std::string subset;
            m_model->getElementValue( "surface_subset", subset );
            if( subset == "subset_index" ) {
                if( m_project ) {
                    int min_i, min_j, min_k, max_i, max_j, max_k;
                    m_model->getElementValue( "index_range_select_min_i", min_i );
                    m_model->getElementValue( "index_range_select_min_j", min_j );
                    m_model->getElementValue( "index_range_select_min_k", min_k );
                    m_model->getElementValue( "index_range_select_max_i", max_i );
                    m_model->getElementValue( "index_range_select_max_j", max_j );
                    m_model->getElementValue( "index_range_select_max_k", max_k );
                    m_index_selector->apply( m_grid_tess_subset,
                                             m_grid_tess,
                                             m_project->nx(), m_project->ny(), m_project->nz(),
                                             min_i, min_j, min_k,
                                             max_i, max_j, max_k );
                    m_render_clip_plane = false;
                }
            }
            else if( subset == "subset_field" ) {
                if( m_has_color_field ) {
                    double field_min, field_max;
                    m_model->getElementValue( "field_select_min", field_min );
                    m_model->getElementValue( "field_select_max", field_max );
                    m_field_selector->apply( m_grid_tess_subset,
                                             m_grid_tess,
                                             m_grid_field, field_min, field_max );
                }
                else {
                    m_all_selector->apply( m_grid_tess_subset,
                                           m_grid_tess );
                }
                m_render_clip_plane = false;
            }
            else if( subset == "subset_plane" ) {
                m_plane_selector->apply( m_grid_tess_subset,
                                         m_grid_tess,
                                         glm::value_ptr( m_clip_plane->plane()*m_local_to_world ) );
                m_render_clip_plane = true;
            }
            else if( subset == "subset_halfplane" ) {
                m_half_plane_selector->apply( m_grid_tess_subset,
                                              m_grid_tess,
                                              glm::value_ptr( m_clip_plane->plane()*m_local_to_world ) );
                m_render_clip_plane = true;
            }
            else {
                m_all_selector->apply( m_grid_tess_subset, m_grid_tess );
                m_render_clip_plane = false;
            }

            if( m_under_the_hood.profilingEnabled() ) {
                m_under_the_hood.surfaceGenerateTimer().beginQuery();
            }

            m_subset_surface->setTriangleCount( 0 );
            m_boundary_surface->setTriangleCount( 0 );
            m_faults_surface->setTriangleCount( 0 );
            boost::shared_ptr<render::GridTessSurf> subset_surf;
            if( m_visibility_mask & models::Appearance::VISIBILITY_MASK_SUBSET ) {
                subset_surf = m_subset_surface;
            }
            boost::shared_ptr<render::GridTessSurf> boundary_surf;
            if( m_visibility_mask & models::Appearance::VISIBILITY_MASK_BOUNDARY ) {
                boundary_surf = m_boundary_surface;
            }
            boost::shared_ptr<render::GridTessSurf> faults_surf;
            if( m_visibility_mask & models::Appearance::VISIBILITY_MASK_FAULTS ) {
                faults_surf = m_faults_surface;
            }

            m_grid_tess_surf_builder->buildSurfaces( subset_surf,
                                                     boundary_surf,
                                                     faults_surf,
                                                     m_grid_tess_subset,
                                                     m_grid_tess,
                                                     flip_faces );
            if( m_under_the_hood.profilingEnabled() ) {
                m_under_the_hood.surfaceGenerateTimer().endQuery();
            }
        }
        catch( std::runtime_error& e ) {
            LOGGER_ERROR( log, "While extracting subset: " << e.what() );
        }
        m_model->updateElement<int>( "renderlist", m_renderlist_db.bump() );
        if( m_renderlist_state == RENDERLIST_SENT ) {
            m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
        }
        //m_renderlist_rethink = true;
    }
//    else if( m_do_update_renderlist ) {
//        m_renderlist_state = RENDERLIST_CHANGED_NOTIFY_CLIENTS;
//        //m_renderlist_rethink = true;
//    }
    //m_do_update_renderlist = false;
}
