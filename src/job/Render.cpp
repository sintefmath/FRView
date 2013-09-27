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
#include "dataset/Project.hpp"
#include "job/FRViewJob.hpp"
#include "render/GridTess.hpp"
#include "utils/Logger.hpp"
#include "render/GridCubeRenderer.hpp"
#include "render/ClipPlane.hpp"
#include "render/GridField.hpp"
#include "render/TextRenderer.hpp"
#include "render/wells/Renderer.hpp"
#include "render/CoordSysRenderer.hpp"
#include "render/rlgen/GridVoxelization.hpp"          // move to renderlist
#include "render/rlgen/VoxelSurface.hpp"  // move to renderlist
#include "render/screen/TransparencyNone.hpp"
#include "render/screen/TransparencyAdditive.hpp"
#include "render/screen/TransparencyWeightedAverage.hpp"
#include "render/RenderItem.hpp"

void
FRViewJob::render( const float*  projection,
                   const float*  modelview,
                   unsigned int  fbo,
                   const size_t  width,
                   const size_t  height )
{
    Logger log = getLogger( "CPViewJob.render" );


    // fetch state from exposed model
//    bool do_render_grid = true;
//    bool render_wells = true;
//    bool render_clipplane = true;
    bool log_map = false;
    double min, max;
    std::string field_map;
    try {
        m_model->getElementValue( "field_range_min", min );
        m_model->getElementValue( "field_range_max", max );
        m_model->getElementValue( "colormap_type", field_map );
    }
    catch( std::runtime_error& e ) {
        LOGGER_ERROR( log, "Error fetchin state from exposed model: " << e.what() );
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


    glUseProgram( 0 );
    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glViewport( 0, 0, width, height );

    const glm::vec4& bg = m_appearance.backgroundColor();
    glClearColor( bg.r, bg.g, bg.b, bg.a );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );
    if( !m_has_pipeline ) {
        return;
    }


    // ---- Actual rendering ---------------------------------------------------
    if( m_under_the_hood.profilingEnabled() ) {
        m_under_the_hood.surfaceRenderTimer().beginQuery();
    }
    try {


        if( m_appearance.renderGrid() ) {
            m_grid_cube_renderer->setUnitCubeToObjectTransform( glm::value_ptr( m_local_from_world ) );
            m_grid_cube_renderer->render( projection, modelview );
        }

        if( field_map.length() >= 3 && field_map.substr(0, 3) == "Log" ) {
            log_map = true;
        }

        if( m_render_clip_plane && m_appearance.clipPlaneVisible() ) {
            glBindFramebuffer( GL_FRAMEBUFFER, fbo );
            glUseProgram( 0 );
            glColor4fv( glm::value_ptr( m_appearance.clipPlaneColor() ) );
            m_clip_plane->render( projection, modelview );
        }


        glBindFramebuffer( GL_FRAMEBUFFER, fbo );
        std::vector<render::RenderItem> items;
        
        if( m_appearance.renderWells() ) {
            items.resize( items.size() + 1 );
            items.back().m_renderer = render::RenderItem::RENDERER_WELL;
            items.back().m_well = m_wells;
        }

        
        
        if( m_visibility_mask & models::Appearance::VISIBILITY_MASK_FAULTS )  {
            const glm::vec4& fc = m_appearance.faultsFillColor();
            const glm::vec4& oc = m_appearance.faultsOutlineColor();
            items.resize( items.size() + 1 );
            items.back().m_renderer = render::RenderItem::RENDERER_SURFACE;
            items.back().m_surf = m_faults_surface;
            items.back().m_field = false;
            items.back().m_line_thickness = m_appearance.lineThickness();
            items.back().m_edge_color[0] = oc.r;
            items.back().m_edge_color[1] = oc.g;
            items.back().m_edge_color[2] = oc.b;
            items.back().m_edge_color[3] = oc.a;
            items.back().m_face_color[0] = fc.r;
            items.back().m_face_color[1] = fc.g;
            items.back().m_face_color[2] = fc.b;
            items.back().m_face_color[3] = fc.a;
        }
        if( m_visibility_mask & models::Appearance::VISIBILITY_MASK_SUBSET ) {
            const glm::vec4& fc = m_appearance.subsetFillColor();
            const glm::vec4& oc = m_appearance.subsetOutlineColor();
            items.resize( items.size() + 1 );
            items.back().m_renderer = render::RenderItem::RENDERER_SURFACE;
            items.back().m_surf = m_subset_surface;
            items.back().m_field = m_has_color_field;
            items.back().m_field_log_map = log_map;
            items.back().m_field_min = min;
            items.back().m_field_max = max;
            items.back().m_line_thickness =m_appearance.lineThickness();
            items.back().m_edge_color[0] = oc.r;
            items.back().m_edge_color[1] = oc.g;
            items.back().m_edge_color[2] = oc.b;
            items.back().m_edge_color[3] = oc.a;
            items.back().m_face_color[0] = fc.r;
            items.back().m_face_color[1] = fc.g;
            items.back().m_face_color[2] = fc.b;
            items.back().m_face_color[3] = fc.a;
        }
        if( m_visibility_mask & models::Appearance::VISIBILITY_MASK_BOUNDARY ) {
            const glm::vec4& fc = m_appearance.boundaryFillColor();
            const glm::vec4& oc = m_appearance.boundaryOutlineColor();
            items.resize( items.size() + 1 );
            items.back().m_renderer = render::RenderItem::RENDERER_SURFACE;
            items.back().m_surf = m_boundary_surface;
            items.back().m_field = m_has_color_field;
            items.back().m_field_log_map = log_map;
            items.back().m_field_min = min;
            items.back().m_field_max = max;
            items.back().m_line_thickness = m_appearance.lineThickness();
            items.back().m_edge_color[0] = oc.r;
            items.back().m_edge_color[1] = oc.g;
            items.back().m_edge_color[2] = oc.b;
            items.back().m_edge_color[3] = oc.a;
            items.back().m_face_color[0] = fc.r;
            items.back().m_face_color[1] = fc.g;
            items.back().m_face_color[2] = fc.b;
            items.back().m_face_color[3] = fc.a;
        }



        switch ( m_appearance.renderQuality() ) {
        case 0:
            if( (!m_screen_manager) ||
                    (typeid(*m_screen_manager.get()) != typeid(render::screen::TransparencyNone)) )
            {
                m_screen_manager.reset( new render::screen::TransparencyNone );
                LOGGER_DEBUG( log, "Created " << typeid(*m_screen_manager.get()).name() );
            }
            break;
        case 1:
            if( (!m_screen_manager) ||
                    (typeid(*m_screen_manager.get()) != typeid(render::screen::TransparencyAdditive)) )
            {
                m_screen_manager.reset( new render::screen::TransparencyAdditive( width, height ) );
                LOGGER_DEBUG( log, "Created " << typeid(*m_screen_manager.get()).name() );
            }
            break;
        case 2:
        case 3:
        default:
            if( (!m_screen_manager) ||
                    (typeid(*m_screen_manager.get()) != typeid(render::screen::TransparencyWeightedAverage)) )
            {
                m_screen_manager.reset( new render::screen::TransparencyWeightedAverage( width, height ) );
                LOGGER_DEBUG( log, "Created " << typeid(*m_screen_manager.get()).name() );
            }
            break;
        }
        m_screen_manager->render( fbo,
                                  width,
                                  height,
                                  glm::value_ptr( m_local_to_world ),
                                  glm::value_ptr( mv ),
                                  projection,
                                  m_grid_tess,
                                  m_grid_field,
                                  items);
        

        glViewport( 0, 0, 0.1*width, 0.1*height );
        m_coordsys_renderer->render( glm::value_ptr( mv ), 0.1*width, 0.1*height);

    }
    catch( std::runtime_error& e ) {
        LOGGER_ERROR( log, "While rendering: " << e.what() );
    }
    if( m_under_the_hood.profilingEnabled() ) {
        m_under_the_hood.surfaceRenderTimer().endQuery();
    }
}
