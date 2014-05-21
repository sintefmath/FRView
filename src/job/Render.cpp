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
#include "render/GridCubeRenderer.hpp"
#include "render/ClipPlane.hpp"
#include "render/GridField.hpp"
#include "render/TextRenderer.hpp"
#include "render/wells/Renderer.hpp"
#include "render/CoordSysRenderer.hpp"
#include "render/rlgen/VoxelGrid.hpp"
#include "render/rlgen/VoxelSurface.hpp"  // move to renderlist
#include "render/manager/OnlySolid.hpp"
#include "render/manager/TransparencyAdditive.hpp"
#include "render/manager/TransparencyWeightedAverage.hpp"
#include "render/manager/OrderIndependentTransparency.hpp"
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

    // create object space to model space matrix
//    glm::mat4 p = glm::mat4( projection[0],  projection[1],  projection[2],  projection[3],
//                             projection[4],  projection[5],  projection[6],  projection[7],
//                             projection[8],  projection[9],  projection[10], projection[11],
//                             projection[12], projection[13], projection[14], projection[15] );

    glm::mat4 mv = glm::mat4( modelview[0], modelview[1],  modelview[2],  modelview[3],
                              modelview[4], modelview[5],  modelview[6],  modelview[7],
                              modelview[8], modelview[9],  modelview[10], modelview[11],
                              modelview[12],modelview[13], modelview[14], modelview[15] );


    glUseProgram( 0 );
    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glViewport( 0, 0, width, height );

    const glm::vec4& bg = m_renderconfig.backgroundColor();
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


        if( m_renderconfig.renderGrid() ) {
            m_grid_cube_renderer->setUnitCubeToObjectTransform( glm::value_ptr( m_local_from_world ) );
            m_grid_cube_renderer->render( projection, modelview );
        }

        
        glBindFramebuffer( GL_FRAMEBUFFER, fbo );
        std::vector<render::RenderItem> items;

        if( currentSourceItemValid() && m_render_clip_plane && m_renderconfig.clipPlaneVisible()) {
            if( m_render_clip_plane && m_renderconfig.clipPlaneVisible() ) {
                glBindFramebuffer( GL_FRAMEBUFFER, fbo );
                glUseProgram( 0 );
                glColor4fv( glm::value_ptr( m_renderconfig.clipPlaneColor() ) );
                currentSourceItem()->m_clip_plane->render( projection, modelview );
            }
        }

        glm::vec3 line_color = glm::vec3(0.0, 0.0, 0.0 );
        
        for(size_t i=0; i<m_source_items.size(); i++ ) {
            boost::shared_ptr<SourceItem> source_item = m_source_items[i];
            
            bool log_map = false;
            double min = 0.0;
            double max = 0.0;
            
            if( source_item->m_grid_field ) {
                min = source_item->m_grid_field->minValue();
                max = source_item->m_grid_field->maxValue();

                if( !source_item->m_color_map ) {
                    // FIXME: Sometimes this happens, haven't tracked down
                    // exactly why yet.
                    if( m_color_maps ) {
                        source_item->m_color_map = m_color_maps;
                    }
                    else {
                        LOGGER_ERROR( log, "There is no color map!" );
                    }
                }
            }

            if( source_item->m_appearance_data == NULL ) {
                LOGGER_ERROR( log, "Source item " << i << " has no appearance data" );
                continue;
            }

            const models::AppearanceData& ap = *source_item->m_appearance_data;
                
            log_map = ap.colorMapType() == models::AppearanceData::COLORMAP_LOGARITMIC;
            if( ap.colorMapFixed() ) {
                min = ap.colorMapFixedMin();
                max = ap.colorMapFixedMax();
            }
            
            if( m_renderconfig.renderWells() ) {
                items.resize( items.size() + 1 );
                items.back().m_renderer = render::RenderItem::RENDERER_WELL;
                items.back().m_well = source_item->m_wells;
            }
            
            if( source_item->m_faults_surface
                    && (source_item->m_visibility_mask & models::AppearanceData::VISIBILITY_MASK_FAULTS ) )
            {
                const glm::vec3 fc = ap.faultsColor();

                items.resize( items.size() + 1 );
                items.back().m_mesh = source_item->m_grid_tess;
                items.back().m_renderer = render::RenderItem::RENDERER_SURFACE;
                items.back().m_surf = source_item->m_faults_surface;
                items.back().m_line_thickness = m_renderconfig.lineThickness();
                items.back().m_edge_color[0] = line_color.r;
                items.back().m_edge_color[1] = line_color.g;
                items.back().m_edge_color[2] = line_color.b;
                items.back().m_edge_color[3] = ap.faultsOutlineAlpha();
                items.back().m_face_color[0] = fc.r;
                items.back().m_face_color[1] = fc.g;
                items.back().m_face_color[2] = fc.b;
                items.back().m_face_color[3] = ap.faultsFillAlpha();
            }

            if( source_item->m_subset_surface
                    && (source_item->m_visibility_mask & models::AppearanceData::VISIBILITY_MASK_SUBSET ) )
            {
                const glm::vec3 fc = ap.subsetColor();

                items.resize( items.size() + 1 );
                items.back().m_mesh = source_item->m_grid_tess;
                items.back().m_renderer = render::RenderItem::RENDERER_SURFACE;
                items.back().m_surf = source_item->m_subset_surface;
                items.back().m_field = source_item->m_grid_field;
                items.back().m_field_log_map = log_map;
                items.back().m_color_map = source_item->m_color_map;
                items.back().m_field_min = min;
                items.back().m_field_max = max;
                items.back().m_line_thickness =m_renderconfig.lineThickness();
                items.back().m_edge_color[0] = line_color.r;
                items.back().m_edge_color[1] = line_color.g;
                items.back().m_edge_color[2] = line_color.b;
                items.back().m_edge_color[3] = ap.subsetOutlineAlpha();
                items.back().m_face_color[0] = fc.r;
                items.back().m_face_color[1] = fc.g;
                items.back().m_face_color[2] = fc.b;
                items.back().m_face_color[3] = ap.subsetFillAlpha();
            }

            if( source_item->m_boundary_surface
                    && (source_item->m_visibility_mask & models::AppearanceData::VISIBILITY_MASK_BOUNDARY ) )
            {
                const glm::vec3 fc = ap.boundaryColor();

                items.resize( items.size() + 1 );
                items.back().m_mesh = source_item->m_grid_tess;
                items.back().m_renderer = render::RenderItem::RENDERER_SURFACE;
                items.back().m_surf = source_item->m_boundary_surface;
                items.back().m_field = source_item->m_grid_field;
                items.back().m_color_map = source_item->m_color_map;
                items.back().m_field_log_map = log_map;
                items.back().m_field_min = min;
                items.back().m_field_max = max;
                items.back().m_line_thickness = m_renderconfig.lineThickness();
                items.back().m_edge_color[0] = line_color.r;
                items.back().m_edge_color[1] = line_color.g;
                items.back().m_edge_color[2] = line_color.b;
                items.back().m_edge_color[3] = ap.boundaryOutlineAlpha();
                items.back().m_face_color[0] = fc.r;
                items.back().m_face_color[1] = fc.g;
                items.back().m_face_color[2] = fc.b;
                items.back().m_face_color[3] = ap.boundaryFillAlpha();
            }
        }
        
        if( m_under_the_hood.debugFrame() ) {
            std::stringstream o;
            o << "Render items:\n";
            for( size_t i=0; i<items.size(); i++ ) {
                const render::RenderItem& ri = items[i];
                
                o << "  item " << i << ":\n";
                o << "    mesh      = " << ri.m_mesh << "\n";
                o << "    field     = " << ri.m_field << "\n";
                o << "    color map = " << ri.m_color_map << "\n";
                o << "    field min = " << ri.m_field_min << "\n";
                o << "    field max = " << ri.m_field_max << "\n";
            }
            
            LOGGER_DEBUG( log, o.str() );
        }
        
        switch ( m_renderconfig.renderQuality() ) {
        case 0:
            if( (!m_screen_manager)
                    || (typeid(*m_screen_manager.get()) != typeid(render::manager::TransparencyNone))
                    || m_screen_manager->expired( m_renderconfig ) )
            {
                m_screen_manager.reset( new render::manager::TransparencyNone( m_renderconfig,
                                                                               width, height ) );
                LOGGER_DEBUG( log, "Created " << typeid(*m_screen_manager.get()).name() );
            }
            break;
        case 1:
            if( (!m_screen_manager)
                    || (typeid(*m_screen_manager.get()) != typeid(render::manager::TransparencyAdditive))
                    || m_screen_manager->expired( m_renderconfig ) )
            {
                m_screen_manager.reset( new render::manager::TransparencyAdditive( m_renderconfig,
                                                                                   width, height ) );
                LOGGER_DEBUG( log, "Created " << typeid(*m_screen_manager.get()).name() );
            }
            break;
        case 2:
            if( (!m_screen_manager)
                    || (typeid(*m_screen_manager.get()) != typeid(render::manager::TransparencyWeightedAverage))
                    || m_screen_manager->expired( m_renderconfig ) )
            {
                m_screen_manager.reset( new render::manager::TransparencyWeightedAverage( m_renderconfig,
                                                                                          width, height ) );
                LOGGER_DEBUG( log, "Created " << typeid(*m_screen_manager.get()).name() );
            }
            break;
        case 3:
        default:
            if( (!m_screen_manager)
                    || (typeid(*m_screen_manager.get()) != typeid(render::manager::OrderIndependentTransparency))
                    || m_screen_manager->expired( m_renderconfig ) )
            {
                m_screen_manager.reset( new render::manager::OrderIndependentTransparency( m_renderconfig,
                                                                                           width, height ) );
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
