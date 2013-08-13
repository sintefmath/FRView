#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "CPViewJob.hpp"
#include "GridTess.hpp"
#include "Project.hpp"
#include "Logger.hpp"
#include "GridTessSubset.hpp"
#include "GridTessSurf.hpp"
#include "GridTessSurfBuilder.hpp"
#include "GridTessSurfRenderer.hpp"
#include "GridField.hpp"
#include "CellSelector.hpp"
#include "ClipPlane.hpp"

void
CPViewJob::doCompute()
{
    Logger log = getLogger( "CPViewJob.doCompute" );

    GLenum error = glGetError();
    while( error != GL_NO_ERROR ) {
        LOGGER_ERROR( log, "Entered function with GL error: " << gluGetString(error) );
        error = glGetError();
    }

    if( m_do_update_subset ) {
        m_do_update_subset = false;
        try {
            bool flip_faces;

            m_policy->getElementValue( "tess_flip_orientation", flip_faces );

            std::string subset;
            m_policy->getElementValue( "surface_subset", subset );
            if( subset == "subset_index" ) {
                int min_i, min_j, min_k, max_i, max_j, max_k;
                m_policy->getElementValue( "index_range_select_min_i", min_i );
                m_policy->getElementValue( "index_range_select_min_j", min_j );
                m_policy->getElementValue( "index_range_select_min_k", min_k );
                m_policy->getElementValue( "index_range_select_max_i", max_i );
                m_policy->getElementValue( "index_range_select_max_j", max_j );
                m_policy->getElementValue( "index_range_select_max_k", max_k );
                m_index_selector->apply( m_grid_tess_subset,
                                         m_grid_tess,
                                         m_project->nx(), m_project->ny(), m_project->nz(),
                                         min_i, min_j, min_k,
                                         max_i, max_j, max_k );
                m_render_clip_plane = false;
            }
            else if( subset == "subset_field" ) {
                if( m_has_color_field ) {
                    double field_min, field_max;
                    m_policy->getElementValue( "field_select_min", field_min );
                    m_policy->getElementValue( "field_select_max", field_max );
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

            m_grid_tess_surf_builder->buildSubsetSurface( m_subset_surface,
                                                          m_grid_tess_subset,
                                                          m_grid_tess,
                                                          flip_faces );

            m_grid_tess_surf_builder->buildSubsetBoundarySurface( m_boundary_surface,
                                                                  m_grid_tess_subset,
                                                                  m_grid_tess,
                                                                  flip_faces );
            m_grid_tess_surf_builder->buildFaultSurface( m_faults_surface,
                                                         m_grid_tess,
                                                         flip_faces );
        }
        catch( std::runtime_error& e ) {
            LOGGER_ERROR( log, "While extracting subset: " << e.what() );
        }
        m_policy->updateElement<int>( "renderlist", m_renderlist_db.bump() );
        m_renderlist_rethink = true;
    }




}
