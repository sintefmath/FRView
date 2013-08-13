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

#include "CPViewJob.hpp"
#include "GridTess.hpp"
#include "ClipPlane.hpp"
#include "GridTessSurfBBoxFinder.hpp"

const tinia::renderlist::DataBase*
CPViewJob::getRenderList( const std::string& session, const std::string& key )
{
    if( !m_cares_about_renderlists ) {
        initRenderList();
        m_cares_about_renderlists = true;
    }
    fetchData();
    updateModelMatrices();
    doCompute();
    if( m_renderlist_rethink ) {
        m_bbox_finder->find( m_proxy_transform,
                             m_proxy_box_min,
                             m_proxy_box_max,
                             m_grid_tess,
                             m_subset_surface );

        updateProxyMatrices();
        updateRenderList();
    }
    return &m_renderlist_db;
}

void
CPViewJob::initRenderList()
{

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

    std::string solid_vs =
            "uniform mat4 MVP;\n"
            "attribute vec3 position;\n"
            "void\n"
            "main()\n"
            "{\n"
            "    gl_Position = MVP * vec4( position, 1.0 );\n"
            "}\n";

    std::string solid_fs =
            "#ifdef GL_ES\n"
            "precision highp float;\n"
            "#endif\n"
            "uniform vec3 color;\n"
            "void\n"
            "main()\n"
            "{\n"
            "    gl_FragColor = vec4( color, 1.0 );\n"
            "}\n";

    m_renderlist_db.createBuffer( "wire_cube_pos" )->set( wire_cube_pos, 12*2*3 );
    m_renderlist_db.createAction<tinia::renderlist::Draw>( "wire_cube_draw" )
            ->setNonIndexed( tinia::renderlist::PRIMITIVE_LINES, 0, 12*2 );
    m_renderlist_db.createShader( "solid" )
            ->setVertexStage( solid_vs )
            ->setFragmentStage( solid_fs );
    m_renderlist_db.createAction<tinia::renderlist::SetShader>( "solid_use" )
            ->setShader( "solid" );
    m_renderlist_db.createAction<tinia::renderlist::SetInputs>( "solid_wire_cube_input" )
            ->setShader( "solid" )
            ->setInput( "position", "wire_cube_pos", 3 );
    m_renderlist_db.createAction<tinia::renderlist::SetLocalCoordSys>( "bbox_pos" );
    m_renderlist_db.createAction<tinia::renderlist::SetLocalCoordSys>( "proxy_pos" );
    m_renderlist_db.createAction<tinia::renderlist::SetLocalCoordSys>( "identity_pos" );

    m_renderlist_db.createAction<tinia::renderlist::SetUniforms>( "solid_orient" )
            ->setShader( "solid" )
            ->setSemantic( "MVP", tinia::renderlist::SEMANTIC_MODELVIEW_PROJECTION_MATRIX );
    m_renderlist_db.createAction<tinia::renderlist::SetUniforms>( "solid_white" )
            ->setShader( "solid" )
            ->setFloat3( "color", 1.f, 1.f, 1.f );
    m_renderlist_db.createAction<tinia::renderlist::SetUniforms>( "solid_green" )
            ->setShader( "solid" )
            ->setFloat3( "color", 0.f, 1.f, 0.f );
    m_renderlist_db.createAction<tinia::renderlist::SetUniforms>( "solid_yellow" )
            ->setShader( "solid" )
            ->setFloat3( "color", 1.f, 1.f, 0.f );

}

void
CPViewJob::updateRenderList( )
{

    m_renderlist_db.castedItemByName<tinia::renderlist::SetLocalCoordSys*>( "bbox_pos" )
            ->setOrientation( glm::value_ptr( m_bbox_from_world ),
                              glm::value_ptr( m_bbox_to_world ) );
    m_renderlist_db.castedItemByName<tinia::renderlist::SetLocalCoordSys*>( "proxy_pos" )
            ->setOrientation( glm::value_ptr( m_proxy_from_world ),
                              glm::value_ptr( m_proxy_to_world ) );

    m_renderlist_db.drawOrderClear()
        ->drawOrderAdd( "solid_use");

    if( true || m_renderlist_rethink ) {
        m_renderlist_rethink = false;
        if( m_render_clip_plane ) {
            std::vector<float> vertices;
            m_clip_plane->getLineLoop( vertices );

            tinia::renderlist::Buffer* buf = m_renderlist_db.castedItemByName<tinia::renderlist::Buffer*>( "clip_plane_pos" );
            if( buf == NULL ) {
                buf = m_renderlist_db.createBuffer( "clip_plane_pos" );
            }
            buf->set( vertices.data(), vertices.size() );

            tinia::renderlist::Draw* draw = m_renderlist_db.castedItemByName<tinia::renderlist::Draw*>( "clip_plane_draw" );
            if( draw == NULL ) {
                draw = m_renderlist_db.createAction<tinia::renderlist::Draw>( "clip_plane_draw" );
            }
            draw->setNonIndexed( tinia::renderlist::PRIMITIVE_LINE_LOOP, 0, vertices.size()/3 );

            tinia::renderlist::SetInputs* inputs = m_renderlist_db.castedItemByName<tinia::renderlist::SetInputs*>( "clip_plane_inputs" );
            if( inputs == NULL ) {
                inputs = m_renderlist_db.createAction<tinia::renderlist::SetInputs>( "clip_plane_inputs" );
            }
            inputs->setShader( "solid" )->setInput( "position", "clip_plane_pos", 3 );


            m_renderlist_db.drawOrderAdd( "solid_yellow" )
                    ->drawOrderAdd( "identity_pos" )
                    ->drawOrderAdd( "solid_orient" )
                    ->drawOrderAdd( inputs->id() )
                    ->drawOrderAdd( draw->id() );
        }

        m_renderlist_db.drawOrderAdd( "solid_wire_cube_input" )
                ->drawOrderAdd( "bbox_pos" )
                ->drawOrderAdd( "solid_orient" )
                ->drawOrderAdd( "solid_white" )
                ->drawOrderAdd( "wire_cube_draw" )

                ->drawOrderAdd( "proxy_pos" )
                ->drawOrderAdd( "solid_orient" )
                ->drawOrderAdd( "solid_green" )
                ->drawOrderAdd( "wire_cube_draw" );

    }
    m_renderlist_db.process();
    //m_policyLib->updateElement<int>( "renderlist", m_renderlist_db.latest() );
}
