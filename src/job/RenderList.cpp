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
#include <tinia/renderlist/Buffer.hpp>
#include <tinia/renderlist/Draw.hpp>
#include <tinia/renderlist/SetViewCoordSys.hpp>
#include <tinia/renderlist/Shader.hpp>
#include <tinia/renderlist/SetShader.hpp>
#include <tinia/renderlist/SetInputs.hpp>
#include <tinia/renderlist/SetUniforms.hpp>
#include <tinia/renderlist/SetLight.hpp>
#include <tinia/renderlist/SetLocalCoordSys.hpp>
#include <tinia/renderlist/SetFramebuffer.hpp>
#include <tinia/renderlist/SetFramebufferState.hpp>
#include <tinia/renderlist/SetPixelState.hpp>
#include <tinia/renderlist/SetRasterState.hpp>

#include "job/FRViewJob.hpp"
#include "render/ClipPlane.hpp"
#include "render/mesh/AbstractMeshGPUModel.hpp"
#include "render/mesh/CellSetInterface.hpp"
#include "render/rlgen/GridVoxelization.hpp"
#include "render/rlgen/VoxelSurface.hpp"

namespace resources {
    extern const std::string gles_solid_vs;
    extern const std::string gles_solid_fs;
    extern const std::string gles_shaded_triangles_vs;
    extern const std::string gles_shaded_triangles_fs;
}

const tinia::renderlist::DataBase*
FRViewJob::getRenderList( const std::string& session, const std::string& key )
{
    if( !m_renderlist_initialized ) {
        initRenderList();
        m_renderlist_initialized = true;
    }
    m_under_the_hood.update();
    doLogic();
    fetchData();
    updateModelMatrices();
    doCompute();

    if( currentSourceItemValid() && m_has_pipeline ) {
        if( m_renderlist_state == RENDERLIST_CHANGED_CLIENTS_NOTIFIED ) {
            m_renderlist_state = RENDERLIST_SENT;

            if( m_under_the_hood.profilingEnabled() ) {
                m_under_the_hood.proxyGenerateTimer().beginQuery();
            }

            for( size_t i=0; i<m_source_items.size(); i++ ) {
                SourceItem& source_item = m_source_items[i];
                
                // FIXME: Needs to add support for multi-object
                
                boost::shared_ptr<const render::mesh::CellSetInterface> cell_set =
                        boost::dynamic_pointer_cast<const render::mesh::CellSetInterface>( source_item.m_grid_tess );
                if( cell_set && source_item.m_grid_tess_subset ) {
                    m_grid_voxelizer->build( cell_set,
                                             source_item.m_grid_tess_subset,
                                             glm::value_ptr( m_local_to_world ) );
                
                    // FIXME: coloring should be part of splatting and surface
                    // extraction should be moved outside of loop (independent on
                    // geometries).
                }
                m_voxel_surface->build( m_grid_voxelizer, source_item.m_grid_field );
            }
            if( m_under_the_hood.profilingEnabled() ) {
                m_under_the_hood.proxyGenerateTimer().endQuery();
            }
            updateRenderList();
        }
    }
    return &m_renderlist_db;
}

void
FRViewJob::initRenderList()
{
    namespace rl = tinia::renderlist;


    float wire_cube_pos[12*2*3] = {
        0.f, 0.f, 0.f,  1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,  1.f, 1.f, 0.f,
        0.f, 0.f, 0.f,  0.f, 1.f, 0.f,
        1.f, 0.f, 0.f,  1.f, 1.f, 0.f,
        0.f, 0.f, 0.f,  0.f, 0.f, 1.f,
        0.f, 1.f, 0.f,  0.f, 1.f, 1.f,
        1.f, 0.f, 0.f,  1.f, 0.f, 1.f,
        1.f, 1.f, 0.f,  1.f, 1.f, 1.f,
        0.f, 0.f, 1.f,  1.f, 0.f, 1.f,
        0.f, 1.f, 1.f,  1.f, 1.f, 1.f,
        0.f, 0.f, 1.f,  0.f, 1.f, 1.f,
        1.f, 0.f, 1.f,  1.f, 1.f, 1.f,
    };

    // simple solid color shader
    m_renderlist_db.createShader( "solid" )
            ->setVertexStage( resources::gles_solid_vs )
            ->setFragmentStage( resources::gles_solid_fs );
    m_renderlist_db.createAction<rl::SetShader>( "solid_use" )
            ->setShader( "solid" );
    m_renderlist_db.createAction<rl::SetUniforms>( "solid_orient" )
            ->setShader( "solid" )
            ->setSemantic( "MVP", rl::SEMANTIC_MODELVIEW_PROJECTION_MATRIX );
    m_renderlist_db.createAction<rl::SetUniforms>( "solid_white" )
            ->setShader( "solid" )
            ->setFloat3( "color", 1.f, 1.f, 1.f );
    m_renderlist_db.createAction<rl::SetUniforms>( "solid_green" )
            ->setShader( "solid" )
            ->setFloat3( "color", 0.f, 1.f, 0.f );
    m_renderlist_db.createAction<rl::SetUniforms>( "solid_yellow" )
            ->setShader( "solid" )
            ->setFloat3( "color", 1.f, 1.f, 0.f );

    // fakey triangle with outline shader
    m_renderlist_db.createShader( "surface" )
            ->setVertexStage( resources::gles_shaded_triangles_vs )
            ->setFragmentStage( resources::gles_shaded_triangles_fs );
    m_renderlist_db.createAction<rl::SetShader>( "surface_use" )
            ->setShader( "surface" );
    m_renderlist_db.createAction<rl::SetUniforms>( "surface_orient" )
            ->setShader( "surface" )
            ->setSemantic( "MVP", rl::SEMANTIC_MODELVIEW_PROJECTION_MATRIX )
            ->setSemantic( "NM", rl::SEMANTIC_NORMAL_MATRIX );

    // set various local coordinate systems
    m_renderlist_db.createAction<rl::SetLocalCoordSys>( "bbox_pos" );
    m_renderlist_db.createAction<rl::SetLocalCoordSys>( "identity_pos" );

    // clip geometry and draw
    m_renderlist_db.createBuffer( "clip_plane_pos" );
    m_renderlist_db.createAction<rl::SetInputs>( "clip_plane_inputs" )
            ->setShader( "solid" )
            ->setInput( "position", "clip_plane_pos", 3 );
    m_renderlist_db.createAction<rl::Draw>( "clip_plane_draw" );

    // wire cube geometry and draw
    m_renderlist_db.createBuffer( "wire_cube_pos" )
            ->set( wire_cube_pos, 12*2*3 );
    m_renderlist_db.createAction<rl::SetInputs>( "solid_wire_cube_input" )
            ->setShader( "solid" )
            ->setInput( "position", "wire_cube_pos", 3 );
    m_renderlist_db.createAction<rl::Draw>( "wire_cube_draw" )
            ->setNonIndexed( rl::PRIMITIVE_LINES, 0, 12*2 );

    // voxelized surface geometry and draw
    m_renderlist_db.createBuffer( "surface_pos" );
    m_renderlist_db.createAction<rl::SetInputs>( "surface_input" )
            ->setShader( "surface" )
            ->setInput( "position", "surface_pos", 4 );
    m_renderlist_db.createAction<rl::Draw>( "surface_draw" );


}

void
FRViewJob::updateRenderList( )
{
    namespace rl = tinia::renderlist;

    models::Appearance::Theme theme = m_appearance.theme();
    if( m_theme != theme ) {

    }


    m_renderlist_db.castedItemByName<rl::SetLocalCoordSys*>( "bbox_pos" )
            ->setOrientation( glm::value_ptr( m_bbox_from_world ),
                              glm::value_ptr( m_bbox_to_world ) );

    rl::Buffer* surf_buf = m_renderlist_db.castedItemByName<rl::Buffer*>( "surface_pos" );
    surf_buf->set( m_voxel_surface->surfaceInHostMem().data(), m_voxel_surface->surfaceInHostMem().size() );

    rl::Draw* surf_draw = m_renderlist_db.castedItemByName<rl::Draw*>( "surface_draw" );
    surf_draw->setNonIndexed( rl::PRIMITIVE_TRIANGLES, 0, m_voxel_surface->surfaceInHostMem().size()/4 );

    m_renderlist_db.drawOrderClear()
            ->drawOrderAdd( "identity_pos" )
            ->drawOrderAdd( "surface_use" )
            ->drawOrderAdd( "surface_orient" )
            ->drawOrderAdd( "surface_input" )
            ->drawOrderAdd( "surface_draw" )
            ->drawOrderAdd( "solid_use")

            ->drawOrderAdd( "solid_wire_cube_input" )
            ->drawOrderAdd( "bbox_pos" )
            ->drawOrderAdd( "solid_orient" )
            ->drawOrderAdd( "solid_white" )
            ->drawOrderAdd( "wire_cube_draw" );

    if( m_render_clip_plane && currentSourceItemValid() ) {
        SourceItem& source_item = currentSourceItem();
        
        // vertex positions of line loop
        std::vector<float> vertices;
        source_item.m_clip_plane->getLineLoop( vertices );

        m_renderlist_db.castedItemByName<rl::Buffer*>( "clip_plane_pos" )
                ->set( vertices.data(), vertices.size() );
        // update draw command
        m_renderlist_db.castedItemByName<rl::Draw*>( "clip_plane_draw" )
                ->setNonIndexed( rl::PRIMITIVE_LINE_LOOP, 0, vertices.size()/3 );

        m_renderlist_db.drawOrderAdd( "solid_yellow" )
                ->drawOrderAdd( "identity_pos" )
                ->drawOrderAdd( "solid_orient" )
                ->drawOrderAdd( "clip_plane_inputs" )
                ->drawOrderAdd( "clip_plane_draw" );
    }
    m_renderlist_db.process();
    //m_modelLib->updateElement<int>( "renderlist", m_renderlist_db.latest() );
}
