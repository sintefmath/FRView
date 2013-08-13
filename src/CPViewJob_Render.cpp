#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "CPViewJob.hpp"
#include "GridTess.hpp"
#include "Logger.hpp"
#include "Project.hpp"
#include "GridCubeRenderer.hpp"
#include "ClipPlane.hpp"
#include "GridTessSurfRenderer.hpp"
#include "GridField.hpp"
#include "TextRenderer.hpp"
#include "WellRenderer.hpp"

void
CPViewJob::render( const float*  projection,
                   const float*  modelview,
                   unsigned int  fbo,
                   const size_t  width,
                   const size_t  height )
{
    Logger log = getLogger( "CPViewJob.render" );
    GLenum error = glGetError();
    while( error != GL_NO_ERROR ) {
        LOGGER_ERROR( log, "Entered function with GL error: " << gluGetString(error) );
        error = glGetError();
    }

    // create object space to model space matrix
    glm::mat4 p = glm::mat4( projection[0],  projection[1],  projection[2],  projection[3],
                             projection[4],  projection[5],  projection[6],  projection[7],
                             projection[8],  projection[9],  projection[10], projection[11],
                             projection[12], projection[13], projection[14], projection[15] );

    glm::mat4 mv = glm::mat4( modelview[0], modelview[1],  modelview[2],  modelview[3],
                              modelview[4], modelview[5],  modelview[6],  modelview[7],
                              modelview[8], modelview[9],  modelview[10], modelview[11],
                              modelview[12],modelview[13], modelview[14], modelview[15] );

    // check what we need
    int faults_fill_opacity = 0;
    int faults_line_opacity = 0;
    bool faults_needed = false;
    int subset_fill_opacity = 0;
    int subset_line_opacity = 0;
    bool subset_needed = false;
    int boundary_fill_opacity = 0;
    int boundary_line_opacity = 0;
    bool boundary_needed = false;
    try {
        m_policy->getElementValue( "faults_fill_opacity", faults_fill_opacity );
        m_policy->getElementValue( "faults_outline_opacity", faults_line_opacity );
        faults_needed = faults_fill_opacity != 0 || faults_line_opacity != 0;
        m_policy->getElementValue( "subset_fill_opacity", subset_fill_opacity );
        m_policy->getElementValue( "subset_outline_opacity", subset_line_opacity );
        subset_needed = subset_fill_opacity != 0 || subset_line_opacity != 0;
        m_policy->getElementValue( "boundary_fill_opacity", boundary_fill_opacity );
        m_policy->getElementValue( "boundary_outline_opacity", boundary_line_opacity );
        boundary_needed = boundary_fill_opacity != 0 || boundary_line_opacity != 0;
    }
    catch( std::runtime_error& e ) {
        LOGGER_ERROR( log, "Got exception: " << e.what() );
    }


    // ---- Actual rendering ---------------------------------------------------
    glUseProgram( 0 );
    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glViewport( 0, 0, width, height );
    glClearColor(0.f, 0.f, 0.f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );
    try {
        m_grid_cube_renderer->setUnitCubeToObjectTransform( glm::value_ptr( m_local_from_world ) );
        m_grid_cube_renderer->render( projection, modelview );

        bool log_map = false;
        double min, max;
        std::string field_map;
        m_policy->getElementValue( "field_range_min", min );
        m_policy->getElementValue( "field_range_max", max );
        m_policy->getElementValue( "colormap_type", field_map );
        if( field_map.length() >= 3 && field_map.substr(0, 3) == "Log" ) {
            log_map = true;
        }
        int quality;
        m_policy->getElementValue( "rendering_quality", quality );

        if( m_render_clip_plane ) {
            glBindFramebuffer( GL_FRAMEBUFFER, fbo );
            glUseProgram( 0 );
            glColor4f( 1.f, 1.f, 0.f, 1.f );
            m_clip_plane->render( projection, modelview );
        }


        glBindFramebuffer( GL_FRAMEBUFFER, fbo );
        m_well_renderer->render( width,
                                 height,
                                 projection,
                                 modelview,
                                 glm::value_ptr( m_local_to_world ) );

        std::vector<GridTessSurfRenderer::RenderItem> items;
        if( faults_needed ) {
            items.resize( items.size() + 1 );
            items.back().m_surf = m_faults_surface;
            items.back().m_opacity = 0.01f*faults_fill_opacity;
            items.back().m_edge_opacity = 0.01f*faults_line_opacity;
            items.back().m_field = false;
            items.back().m_solid_color[0] = 1.0f;
            items.back().m_solid_color[1] = 0.5f;
            items.back().m_solid_color[2] = 1.0f;
        }
        if( subset_needed ) {
            items.resize( items.size() + 1 );
            items.back().m_surf = m_subset_surface;
            items.back().m_opacity = 0.01f*subset_fill_opacity;
            items.back().m_edge_opacity = 0.01f*subset_line_opacity;
            items.back().m_field = m_has_color_field;
            items.back().m_field_log_map = log_map;
            items.back().m_field_min = min;
            items.back().m_field_max = max;
            items.back().m_solid_color[0] = 0.8f;
            items.back().m_solid_color[1] = 0.8f;
            items.back().m_solid_color[2] = 0.8f;
        }
        if( boundary_needed ) {
            items.resize( items.size() + 1 );
            items.back().m_surf = m_boundary_surface;
            items.back().m_opacity = 0.01f*boundary_fill_opacity;
            items.back().m_edge_opacity = 0.01f*boundary_line_opacity;
            items.back().m_field = m_has_color_field;
            items.back().m_field_log_map = log_map;
            items.back().m_field_min = min;
            items.back().m_field_max = max;
            items.back().m_solid_color[0] = 0.5f;
            items.back().m_solid_color[1] = 0.5f;
            items.back().m_solid_color[2] = 0.5f;
        }
        m_tess_renderer->renderCells( fbo,
                                      width,
                                      height,
                                      glm::value_ptr( mv*m_local_to_world ),
                                      projection,
                                      m_grid_tess,
                                      m_grid_field,
                                      items,
                                      quality );


        m_well_labels->render( width,
                               height,
                               glm::value_ptr( p*mv*m_local_to_world ) );



    }
    catch( std::runtime_error& e ) {
        LOGGER_ERROR( log, "While rendering: " << e.what() );
    }

}
