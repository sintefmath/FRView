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
#include "ASyncReader.hpp"
#include "EclipseReader.hpp"

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

        // Fetch from async reader
        std::shared_ptr< Project<float> > project;
        std::shared_ptr< GridTessBridge > tess_bridge;
        if( m_async_reader->getProject( project, tess_bridge ) ) {

            m_project = project;
            m_grid_tess->update( *tess_bridge );

            // update state variables
            m_load_geometry = false;
            m_do_update_subset = true;
            m_load_color_field = true;
            m_faults_surface_tainted = true;

            // --- update model variables --------------------------------------
            m_model->updateElement<bool>("has_project", true );

            // --- solution list
            std::list<std::string> solutions;
            if( m_project->solutions() == 0 ) {
                solutions.push_back( "[none]" );
            }
            else {
                for(unsigned int i=0; i<m_project->solutions(); i++ ) {
                    solutions.push_back( m_project->solutionName(i) );
                }
            }
            m_model->updateRestrictions( "field_solution", solutions.front(), solutions );
            m_model->updateRestrictions( "field_select_solution", solutions.front(), solutions );

            // --- report steps
            int reportstep_max = std::max( 1u, m_project->reportSteps() )-1u;
            m_model->updateConstraints<int>("field_report_step", 0,0,  reportstep_max );
            m_model->updateConstraints<int>( "field_select_report_step", 0, 0, reportstep_max );

            // Grid size
            const unsigned int nx = m_project->nx();
            const unsigned int ny = m_project->ny();
            const unsigned int nz = m_project->nz();
            int nx_max = std::max( 1u, nx ) - 1u;
            int ny_max = std::max( 1u, ny ) - 1u;
            int nz_max = std::max( 1u, nz ) - 1u;
            m_model->updateConstraints<int>( "index_range_select_min_i", 0, 0, nx_max );
            m_model->updateConstraints<int>( "index_range_select_max_i", nx_max, 0, nx_max );
            m_model->updateConstraints<int>( "index_range_select_min_j", 0, 0, ny_max );
            m_model->updateConstraints<int>( "index_range_select_max_j", ny_max, 0, ny_max );
            m_model->updateConstraints<int>( "index_range_select_min_k", 0, 0, nz_max );
            m_model->updateConstraints<int>( "index_range_select_max_k", nz_max, 0, nz_max );

            std::stringstream o;
            o << "[ " << nx << " x " << ny << " x " << nz << " ]";
            m_model->updateElement( "grid_dim", o.str() );

            o.str("");
            o << (nx*ny*nz);
            m_model->updateElement( "grid_total_cells", o.str() );

            o.str("");
            o << m_grid_tess->cellCount() << " (" << ((100u*m_grid_tess->cellCount())/(nx*ny*nz) ) << "%)";
            m_model->updateElement( "grid_active_cells", o.str() );

            o.str("");
            o << m_grid_tess->triangleCount() << " triangles";
            m_model->updateElement( "grid_faces", o.str() );

            m_renderlist_rethink = true;
        }
    }

    if( m_load_color_field  && m_project ) {
        m_has_color_field = false;

        std::shared_ptr<GridFieldBridge> bridge;
        if( m_async_reader->getSolution( bridge ) ) {

            if( bridge ) {
                m_grid_field->import( *bridge );
                m_has_color_field = true;
            }
            else {
                m_has_color_field = false;
            }
            m_load_color_field = false;
            m_subset_surface_tainted = true;
            m_boundary_surface_tainted = true;

            // -- Update wells
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

            // -- update policy elements
            bool fix;
            m_model->getElementValue( "field_range_enable", fix );
            if( !fix ) {
                m_model->updateElement<double>( "field_range_min", m_grid_field->minValue() );
                m_model->updateElement<double>( "field_range_max", m_grid_field->maxValue() );
            }
            if( m_has_color_field && m_project ) {
                std::stringstream o;
                o << "[ " << m_grid_field->minValue() << ", " << m_grid_field->maxValue() << " ]";
                m_model->updateElement( "field_info_range", o.str() );
                o.str("");
                o << "[not implemented]";
                m_model->updateElement( "field_info_calendar", m_project->reportStepDate( m_report_step_index ) );
                m_model->updateElement( "has_field", true );
            }
            else {
                m_model->updateElement( "field_info_range", "[not available]" );
                m_model->updateElement( "field_info_calendar", "[not available]" );
                m_model->updateElement( "has_field", false );
            }
            m_renderlist_rethink = true;
        }
    }
}
