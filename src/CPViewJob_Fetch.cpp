#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "CPViewJob.hpp"
#include "Project.hpp"
#include "GridTess.hpp"
#include "GridField.hpp"
#include "Logger.hpp"
#include "GridTessBridge.hpp"
#include "TextRenderer.hpp"
#include "WellRenderer.hpp"

void
CPViewJob::fetchData()
{
    Logger log = getLogger( "CPViewJob.doCompute" );
    GLenum error = glGetError();
    while( error != GL_NO_ERROR ) {
        LOGGER_ERROR( log, "Entered function with GL error: " << gluGetString(error) );
        error = glGetError();
    }

    if( m_load_geometry ) {
        m_load_geometry = false;
        m_do_update_subset = true;
        m_load_color_field = true;
        m_faults_surface_tainted = true;
        try {
            GridTessBridge bridge( *m_grid_tess );
            m_project->geometry( bridge );
            m_do_update_subset = true;
        }
        catch( std::runtime_error& e ) {
            LOGGER_ERROR( log, "While getting geometry: " << e.what() );
        }

        const unsigned int nx = m_project->nx();
        const unsigned int ny = m_project->ny();
        const unsigned int nz = m_project->nz();

        std::stringstream o;
        o << "[ " << nx << " x " << ny << " x " << nz << " ]";
        m_policy->updateElement( "grid_dim", o.str() );

        o.str("");
        o << (nx*ny*nz);
        m_policy->updateElement( "grid_total_cells", o.str() );

        o.str("");
        o << m_grid_tess->activeCells() << " (" << ((100u*m_grid_tess->activeCells())/(nx*ny*nz) ) << "%)";
        m_policy->updateElement( "grid_active_cells", o.str() );

        o.str("");
        o << m_grid_tess->triangleCount() << " triangles";
        m_policy->updateElement( "grid_faces", o.str() );

        m_renderlist_rethink = true;
    }

    if( m_load_color_field ) {
        m_load_color_field = false;
        m_subset_surface_tainted = true;
        m_boundary_surface_tainted = true;
        m_has_color_field = false;
        if( m_report_step_index < m_project->reportSteps() && m_solution_index < m_project->solutions() ) {
            try {
                GridFieldBridge bridge( *m_grid_field, *m_grid_tess );
                m_project->field( bridge, m_solution_index, m_report_step_index );
                m_has_color_field = true;//m_grid_field->hasData();
            }
            catch( std::runtime_error& e ) {
                LOGGER_ERROR( log, "While getting field: " << e.what() );
            }
            bool fix;
            m_policy->getElementValue( "field_range_enable", fix );
            if( !fix ) {
                m_policy->updateElement<double>( "field_range_min", m_grid_field->minValue() );
                m_policy->updateElement<double>( "field_range_max", m_grid_field->maxValue() );
            }


            m_well_labels->clear();
            m_well_renderer->clear();

            std::vector<float> colors;
            std::vector<float> positions;
            for( unsigned int w=0; w<m_project->wellCount(); w++ ) {
                if( !m_project->wellDefined( m_report_step_index, w ) ) {
                    continue;
                }
                m_well_labels->add( m_project->wellName(w),
                                    TextRenderer::FONT_8X12,
                                    m_project->wellHeadPosition( m_report_step_index, w ),
                                    10.f,
                                    TextRenderer::ANCHOR_S );
                positions.clear();
                colors.clear();

                const unsigned int bN = m_project->wellBranchCount( m_report_step_index, w );
                for( unsigned int b=0; b<bN; b++ ) {
                    const std::vector<float>& p = m_project->wellBranchPositions( m_report_step_index, w, b );
                    if( p.empty() ) {
                        continue;
                    }
                    for( size_t i=0; i<p.size(); i+=3 ) {
                        positions.push_back( p[i+0] );
                        positions.push_back( p[i+1] );
                        positions.push_back( p[i+2] );
                        colors.push_back( ((i & 0x1) == 0) ? 1.f : 0.5f );
                        colors.push_back( ((i & 0x2) == 0) ? 1.f : 0.5f );
                        colors.push_back( ((i & 0x4) == 0) ? 1.f : 0.5f );
                    }
                    m_well_renderer->addSegments( positions, colors );
                }
            }

/*

            for( size_t w=0; w<wells.size(); w++ ) {
                for( size_t b=0; b<wells[w].m_branches.size(); b++ ) {

                    for( size_t i=0; i<wells[w].m_branches[b].size(); i+=3 ) {
                        positions.push_back( wells[w].m_branches[b][i+0] );
                        positions.push_back( wells[w].m_branches[b][i+1] );
                        positions.push_back( wells[w].m_branches[b][i+2] );
                        colors.push_back( ((i & 0x1) == 0) ? 1.f : 0.5f );
                        colors.push_back( ((i & 0x2) == 0) ? 1.f : 0.5f );
                        colors.push_back( ((i & 0x4) == 0) ? 1.f : 0.5f );
                        LOGGER_DEBUG( log,
                                      wells[w].m_branches[b][i+0] << ", " <<
                                      wells[w].m_branches[b][i+1] << ", " <<
                                      wells[w].m_branches[b][i+2] );
                    }
                    m_well_renderer->addSegments( positions, colors );
                }
            }
            */






        }

        if( m_has_color_field ) {
            std::stringstream o;
            o << "[ " << m_grid_field->minValue() << ", " << m_grid_field->maxValue() << " ]";
            m_policy->updateElement( "field_info_range", o.str() );
            o.str("");
            o << "[not implemented]";
            m_policy->updateElement( "field_info_calendar", m_project->reportStepDate( m_report_step_index ) );
            m_policy->updateElement( "has_field", true );
        }
        else {
            m_policy->updateElement( "field_info_range", "[not available]" );
            m_policy->updateElement( "field_info_calendar", "[not available]" );
            m_policy->updateElement( "has_field", false );
        }
        m_renderlist_rethink = true;
    }

}
