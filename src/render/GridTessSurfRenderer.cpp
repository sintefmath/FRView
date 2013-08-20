/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "utils/GLSLTools.hpp"
#include "utils/Logger.hpp"
#include "GridTess.hpp"
#include "GridTessSurf.hpp"
#include "GridField.hpp"
#include "GridTessSurfRenderer.hpp"

namespace resources {
    extern const std::string screen_common_vs;
    extern const std::string screen_colorize_fs;
    extern const std::string screen_weighted_sum_merge_fs;
    extern const std::string surface_common_vs;
    extern const std::string surface_common_gs;
    extern const std::string surface_frag_list_fs;
    extern const std::string surface_twopass_fs;
    extern const std::string surface_singlepass_fs;
    extern const std::string surface_crappy_fs;
    extern const std::string surface_common_fs;
    extern const std::string geo_edge_vs;
    extern const std::string geo_edge_fs;
}

namespace render {

GridTessSurfRenderer::GridTessSurfRenderer()
    : m_quality( ~0u ),
      m_width( ~0u ),
      m_height( ~0u )
{
    Logger log = getLogger( "render.GridTessSurfRenderer.constructor" );
    
    glGenFramebuffers( 1, &m_fbo_solid );
    glGenFramebuffers( 1, &m_fbo_weighted_average_transparent );
    glGenFramebuffers( 1, &m_fbo_weighted_sum_transparent );
    glGenTextures( 1, &m_depth_tex );
    glGenTextures( 1, &m_solid_color_tex );
    glGenTextures( 1, &m_transparent_color_tex );
    glGenTextures( 1, &m_transparent_complexity_tex );

    // create GPGPU quad
    static const GLfloat quad[ 4*4 ] = {
         1.f, -1.f, 0.f, 1.f,
         1.f,  1.f, 0.f, 1.f,
        -1.f, -1.f, 0.f, 1.f,
        -1.f,  1.f, 0.f, 1.f
    };
    glGenVertexArrays( 1, &m_gpgpu_quad_vertex_array );
    glBindVertexArray( m_gpgpu_quad_vertex_array );
    glGenBuffers( 1, &m_gpgpu_quad_buffer );
    glBindBuffer( GL_ARRAY_BUFFER, m_gpgpu_quad_buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 0, NULL );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );


    for( unsigned int i=0; i<PASS_N; i++ ) {
        m_pass_data[i].m_program = 0;
    }

    GLuint geo_edge_vs = utils::compileShader( log, resources::geo_edge_vs, GL_VERTEX_SHADER );
    GLuint geo_edge_fs = utils::compileShader( log, resources::geo_edge_fs, GL_FRAGMENT_SHADER );

    GLuint screen_common_vs = utils::compileShader( log, resources::screen_common_vs, GL_VERTEX_SHADER );
    GLuint avg_alpha_fs = utils::compileShader( log, resources::screen_colorize_fs, GL_FRAGMENT_SHADER );

    GLuint screen_weighted_sum_fs = utils::compileShader( log, resources::screen_weighted_sum_merge_fs, GL_FRAGMENT_SHADER );

    GLuint surface_common_vs = utils::compileShader( log, resources::surface_common_vs, GL_VERTEX_SHADER );
    GLuint surface_common_gs = utils::compileShader( log, resources::surface_common_gs, GL_GEOMETRY_SHADER );
    GLuint surface_common_paint_gs = utils::compileShader( log, "#define DO_PAINT\n" + resources::surface_common_gs, GL_GEOMETRY_SHADER );


    GLuint surface_twopass_fs = utils::compileShader( log, resources::surface_common_fs +
                                               resources::surface_twopass_fs,
                                               GL_FRAGMENT_SHADER );
    GLuint surface_twopass_paint_fs = utils::compileShader( log, "#define DO_PAINT\n" +
                                                     resources::surface_common_fs +
                                                     resources::surface_twopass_fs,
                                                     GL_FRAGMENT_SHADER );
    GLuint surface_singlepass_fs = utils::compileShader( log, resources::surface_common_fs +
                                                  resources::surface_singlepass_fs,
                                                  GL_FRAGMENT_SHADER );
    GLuint surface_singlepass_paint_fs = utils::compileShader( log, "#define DO_PAINT\n" +
                                                        resources::surface_common_fs +
                                                        resources::surface_singlepass_fs,
                                                        GL_FRAGMENT_SHADER );

    GLuint surface_crappy_fs = utils::compileShader( log, resources::surface_crappy_fs, GL_FRAGMENT_SHADER );
    //GLuint surface_frag_list_fs = utils::compileShader( log, resources::surface_frag_list_fs, GL_FRAGMENT_SHADER );
    //GLuint surface_frag_list_paint_fs = utils::compileShader( log, "#define DO_PAINT\n" + resources::surface_frag_list_fs, GL_FRAGMENT_SHADER );

    //GLuint surface_fraglist_fs =

    m_pass_data[ PASS_EDGE ].m_program = glCreateProgram();
    glAttachShader( m_pass_data[ PASS_EDGE ].m_program, geo_edge_vs );
    glAttachShader( m_pass_data[ PASS_EDGE ].m_program, geo_edge_fs );

    m_pass_data[ PASS_SURFACE_ONEPASS ].m_program = glCreateProgram();
    glAttachShader( m_pass_data[ PASS_SURFACE_ONEPASS ].m_program, surface_common_vs );
    glAttachShader( m_pass_data[ PASS_SURFACE_ONEPASS ].m_program, surface_common_gs );
    glAttachShader( m_pass_data[ PASS_SURFACE_ONEPASS ].m_program, surface_singlepass_fs );

    m_pass_data[ PASS_SURFACE_ONEPASS_PAINT ].m_program = glCreateProgram();
    glAttachShader( m_pass_data[ PASS_SURFACE_ONEPASS_PAINT ].m_program, surface_common_vs );
    glAttachShader( m_pass_data[ PASS_SURFACE_ONEPASS_PAINT ].m_program, surface_common_paint_gs );
    glAttachShader( m_pass_data[ PASS_SURFACE_ONEPASS_PAINT ].m_program, surface_singlepass_paint_fs );

    m_pass_data[ PASS_SURFACE_TWOPASS ].m_program = glCreateProgram();
    glAttachShader( m_pass_data[ PASS_SURFACE_TWOPASS ].m_program, surface_common_vs );
    glAttachShader( m_pass_data[ PASS_SURFACE_TWOPASS ].m_program, surface_common_gs );
    glAttachShader( m_pass_data[ PASS_SURFACE_TWOPASS ].m_program, surface_twopass_fs );

    m_pass_data[ PASS_SURFACE_TWOPASS_PAINT ].m_program = glCreateProgram();
    glAttachShader( m_pass_data[ PASS_SURFACE_TWOPASS_PAINT ].m_program, surface_common_vs );
    glAttachShader( m_pass_data[ PASS_SURFACE_TWOPASS_PAINT ].m_program, surface_common_paint_gs );
    glAttachShader( m_pass_data[ PASS_SURFACE_TWOPASS_PAINT ].m_program, surface_twopass_paint_fs );

    // if 420
    //m_pass_data[ PASS_SURFACE_FRAG_LIST ].m_program = glCreateProgram();
    //glAttachShader( m_pass_data[ PASS_SURFACE_FRAG_LIST ].m_program, surface_common_vs );
    //glAttachShader( m_pass_data[ PASS_SURFACE_FRAG_LIST ].m_program, surface_common_paint_gs );
    //glAttachShader( m_pass_data[ PASS_SURFACE_FRAG_LIST ].m_program, surface_frag_list_fs );

    //m_pass_data[ PASS_SURFACE_FRAG_LIST_PAINT ].m_program = glCreateProgram();
    //glAttachShader( m_pass_data[ PASS_SURFACE_FRAG_LIST_PAINT ].m_program, surface_common_vs );
    //glAttachShader( m_pass_data[ PASS_SURFACE_FRAG_LIST_PAINT ].m_program, surface_common_paint_gs );
    //glAttachShader( m_pass_data[ PASS_SURFACE_FRAG_LIST_PAINT ].m_program, surface_frag_list_paint_fs );

    for( unsigned int i=0; i<PASS_N; i++ ) {
        if( m_pass_data[ i ].m_program == 0 ) {
            continue;
        }
        utils::linkProgram( log, m_pass_data[i].m_program );
        // Not all of these are necessarily defined in all techniques
        m_pass_data[i].m_loc_mvp           = glGetUniformLocation( m_pass_data[i].m_program, "MVP" );
        m_pass_data[i].m_loc_mv            = glGetUniformLocation( m_pass_data[i].m_program, "MV" );
        m_pass_data[i].m_loc_nm            = glGetUniformLocation( m_pass_data[i].m_program, "NM" );
        m_pass_data[i].m_loc_surface_color = glGetUniformLocation( m_pass_data[i].m_program, "surface_color" );
        m_pass_data[i].m_loc_edge_color    = glGetUniformLocation( m_pass_data[i].m_program, "edge_color" );
        m_pass_data[i].m_loc_screen_size   = glGetUniformLocation( m_pass_data[i].m_program, "screen_size" );
    }

    m_pass_weighted_sum_merge.m_program = glCreateProgram();
    glAttachShader( m_pass_weighted_sum_merge.m_program, screen_common_vs );
    glAttachShader( m_pass_weighted_sum_merge.m_program, screen_weighted_sum_fs );
    utils::linkProgram( log, m_pass_weighted_sum_merge.m_program );

    m_deferred_prog = glCreateProgram();
    glAttachShader( m_deferred_prog, screen_common_vs );
    glAttachShader( m_deferred_prog, avg_alpha_fs );
    utils::linkProgram( log, m_deferred_prog );
    m_deferred_loc_fill_opacities = glGetUniformLocation( m_deferred_prog, "fill_opacities" );
    m_deferred_loc_edge_opacities = glGetUniformLocation( m_deferred_prog, "edge_opacities" );
    m_deferred_loc_use_field = glGetUniformLocation( m_deferred_prog, "use_field" );
    m_deferred_loc_linear_map = glGetUniformLocation( m_deferred_prog, "linear_map" );
    m_deferred_loc_log_map = glGetUniformLocation( m_deferred_prog, "log_map" );

    m_surface_crappy_prog = glCreateProgram();
    glAttachShader( m_surface_crappy_prog, surface_common_vs );
    glAttachShader( m_surface_crappy_prog, surface_crappy_fs );
    utils::linkProgram( log, m_surface_crappy_prog );
    m_surface_crappy_loc_mvp = glGetUniformLocation( m_surface_crappy_prog, "MVP" );
    m_surface_crappy_loc_color = glGetUniformLocation( m_surface_crappy_prog, "color" );


    glDeleteShader( geo_edge_vs );
    glDeleteShader( geo_edge_fs );
    glDeleteShader( screen_common_vs );
    glDeleteShader( avg_alpha_fs );
    glDeleteShader( surface_common_vs );
    glDeleteShader( surface_common_gs );
    glDeleteShader( surface_twopass_fs );
    glDeleteShader( surface_singlepass_fs );
    glDeleteShader( surface_crappy_fs );
}

GridTessSurfRenderer::~GridTessSurfRenderer()
{
}


void
GridTessSurfRenderer::resizeBuffers( )
{
    Logger log = getLogger( "render.GridTessSurfRenderer.resizeBuffers" );
    
    GLenum drawbuffers[2] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1
    };

    glBindTexture( GL_TEXTURE_2D, m_depth_tex );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_DEPTH_COMPONENT, m_width, m_height, 0,
                  GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture( GL_TEXTURE_2D, m_solid_color_tex );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_RGBA, m_width, m_height, 0,
                  GL_RGBA, GL_FLOAT, NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture( GL_TEXTURE_2D, m_transparent_color_tex );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_RGBA16F, m_width, m_height, 0,
                  GL_RGBA, GL_FLOAT, NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture( GL_TEXTURE_2D, m_transparent_complexity_tex );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_R16F, m_width, m_height, 0,
                  GL_RED, GL_FLOAT, NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture( GL_TEXTURE_2D, 0 );

    glBindFramebuffer( GL_FRAMEBUFFER, m_fbo_solid );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, m_solid_color_tex, 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_2D, m_depth_tex, 0 );
    glDrawBuffers( 1, drawbuffers );
//    glDrawBuffer( GL_COLOR_ATTACHMENT0 );
    utils::checkFBO( log );

    glBindFramebuffer( GL_FRAMEBUFFER, m_fbo_weighted_sum_transparent );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, m_transparent_color_tex, 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_2D, m_depth_tex, 0 );
    utils::checkFBO( log );


    glBindFramebuffer( GL_FRAMEBUFFER, m_fbo_weighted_average_transparent );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, m_transparent_color_tex, 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                            GL_TEXTURE_2D, m_transparent_complexity_tex, 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_2D, m_depth_tex, 0 );
    glDrawBuffers( 2, drawbuffers );
//    glDrawBuffer( GL_COLOR_ATTACHMENT0 );
    utils::checkFBO( log );
}


void
GridTessSurfRenderer::renderCells(GLuint                          fbo,
                                   const GLsizei                   width,
                                   const GLsizei                   height,
                                   const GLfloat*                  modelview,
                                   const GLfloat*                  projection,
                                   boost::shared_ptr<const GridTess> tess,
                                   boost::shared_ptr<const GridField> field,
                                   const std::vector<GridTessSurfRenderer::RenderItem>&  render_items,
                                   const unsigned int              quality )
{
    if( (width == 0) || (height == 0) ) {
        return;
    }

    if( (m_width != width) || (m_height != height) || (m_quality != quality) ) {
        m_width = width;
        m_height = height;
        m_quality = quality;
        resizeBuffers();
    }

    glViewport( 0, 0, width, height );

    glm::mat4 M(modelview[0], modelview[1], modelview[ 2], modelview[3],
                modelview[4], modelview[5], modelview[ 6], modelview[7],
                modelview[8], modelview[9], modelview[10], modelview[11],
                modelview[12], modelview[13], modelview[14], modelview[15] );
    glm::mat4 P(projection[0], projection[1], projection[ 2], projection[3],
                projection[4], projection[5], projection[ 6], projection[7],
                projection[8], projection[9], projection[10], projection[11],
                projection[12], projection[13], projection[14], projection[15] );
    glm::mat4 MVP = P*M;

    glm::mat4 NM = glm::transpose( glm::inverse( M ) );
    const GLfloat* nm4 = glm::value_ptr( NM );
    const GLfloat nm3[9] = { nm4[0], nm4[1], nm4[2],
                             nm4[4], nm4[5], nm4[6],
                             nm4[8], nm4[9], nm4[10] };


    switch( m_quality ) {
    // crappy quality
    case 0:
        glBindFramebuffer( GL_FRAMEBUFFER, fbo );
        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LESS );
        glDepthMask( GL_TRUE );
        glDisable( GL_BLEND );
        drawCrappy( modelview, projection, glm::value_ptr( MVP ), nm3, width, height,
              tess, field, render_items );
        break;

    // low quality;
    case 1:
        glBindFramebuffer( GL_FRAMEBUFFER, fbo );
        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LESS );
        glDepthMask( GL_TRUE );
        glDisable( GL_BLEND );
        drawSinglePass( modelview, projection, glm::value_ptr( MVP ), nm3, width, height,
              tess, field, render_items );
        break;

    case 2:
        // medium quality
        // --------------
        //
        // Simple formula, Meshkin 'Sort Independent Alpha Blending', GDC2007.
        // additive blending

        // Render solid geometry
        glBindFramebuffer( GL_READ_FRAMEBUFFER, fbo );
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_fbo_solid );
        glBlitFramebuffer( 0, 0, width, height,
                           0, 0, width, height,
                           GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                           GL_NEAREST );
        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LESS );
        glDepthMask( GL_TRUE );
        glDisable( GL_BLEND );
        drawTwoPass( modelview, projection, glm::value_ptr( MVP ), nm3, width, height,
              tess, field, render_items, true );
        // Render transparent geometry (we share depth buffer with solid pass,
        // i.e., no need to copy that)
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_fbo_weighted_sum_transparent );
        glClear( GL_COLOR_BUFFER_BIT );
        glDepthMask( GL_FALSE );
        glEnable( GL_BLEND );
        glBlendFunc( GL_ONE, GL_ONE );  // alpha * color done in geometry shader

        drawTwoPass( modelview, projection, glm::value_ptr( MVP ), nm3, width, height,
              tess, field, render_items, false );
        glDepthMask( GL_TRUE );
        glDisable( GL_BLEND );

        // Merge results
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, fbo );
        glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_2D, m_solid_color_tex );
        glActiveTexture( GL_TEXTURE1 ); glBindTexture( GL_TEXTURE_2D, m_transparent_color_tex );
        glBindVertexArray( m_gpgpu_quad_vertex_array );
        glUseProgram( m_pass_weighted_sum_merge.m_program );
        glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
        break;

    case 3:
        // High quality
        // ------------
        //
        // Weighted average forumla, Bavoil & Myers, 'Order Independent
        // transparency with dual depth peeling', NVIDIA whitepaper.
        glBindFramebuffer( GL_READ_FRAMEBUFFER, fbo );
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_fbo_solid );
        glBlitFramebuffer( 0, 0, width, height,
                           0, 0, width, height,
                           GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                           GL_NEAREST );
        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LESS );
        glDepthMask( GL_TRUE );
        glDisable( GL_BLEND );
        drawTwoPass( modelview, projection, glm::value_ptr( MVP ), nm3, width, height,
              tess, field, render_items, true );
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_fbo_weighted_average_transparent );
        glClearColor( 0.f, 0.f, 0.f, 0.f );
        glClear( GL_COLOR_BUFFER_BIT );
        glDepthMask( GL_FALSE );
        glEnable( GL_BLEND );
        glBlendFunc( GL_ONE, GL_ONE );  // alpha * color done in geometry shader
        drawTwoPass( modelview, projection, glm::value_ptr( MVP ), nm3, width, height,
              tess, field, render_items, false );
        glDepthMask( GL_TRUE );
        glDisable( GL_BLEND );

        // alpha average pass
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, fbo );
        glUseProgram( m_deferred_prog );
        glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_2D, m_solid_color_tex );
        glActiveTexture( GL_TEXTURE1 ); glBindTexture( GL_TEXTURE_2D, m_transparent_color_tex );
        glActiveTexture( GL_TEXTURE2 ); glBindTexture( GL_TEXTURE_2D, m_transparent_complexity_tex );

        glBindVertexArray( m_gpgpu_quad_vertex_array );
        glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
        break;
    }



    // Cleanup
    glBindVertexArray( 0 );
    glUseProgram( 0 );
    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glActiveTexture( GL_TEXTURE2 ); glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE1 ); glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_2D, 0 );
    glEnable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );
    glDepthMask( GL_TRUE );
}


void
GridTessSurfRenderer::drawTwoPass( const GLfloat*                  modelview,
                                   const GLfloat*                  projection,
                                   const GLfloat*                  modelview_projection,
                                   const GLfloat*                  normal_transform,
                                   const GLsizei                   width,
                                   const GLsizei                   height,
                                   boost::shared_ptr<const GridTess>                 tess,
                                   boost::shared_ptr<const GridField>                field,
                                   const std::vector<RenderItem>&  render_items,
                                   const bool                      solid_pass )
{
    glBindVertexArray( tess->vertexPositonsAsVertexArrayObject() );

    for( size_t i = 0; i<render_items.size(); i++) {
        const RenderItem& item = render_items[i];

        PassData* pass = &m_pass_data[ PASS_SURFACE_TWOPASS ];
        if( item.m_edge_color[3] > 0.f ) {
            pass = &m_pass_data[ PASS_SURFACE_TWOPASS_PAINT ];
            glDisable( GL_POLYGON_OFFSET_FILL );
        }
        else {
            glPolygonOffset( 1.f, 1.f );
            glEnable( GL_POLYGON_OFFSET_FILL );
        }

        glUseProgram( pass->m_program );
        glUniformMatrix4fv( pass->m_loc_mvp, 1, GL_FALSE, modelview_projection );
        glUniformMatrix3fv( pass->m_loc_nm, 1, GL_FALSE, normal_transform );
        glUniformMatrix4fv( pass->m_loc_mv, 1, GL_FALSE, modelview );
        glUniform2f( pass->m_loc_screen_size, width, height );

        glActiveTexture( GL_TEXTURE2 );
        if( field.get() != NULL ) {
            glBindTexture( GL_TEXTURE_BUFFER, field->texture() );
        }
        else {
            glBindTexture( GL_TEXTURE_BUFFER, 0 );
        }
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_BUFFER, tess->normalVectorsAsBufferTexture() );

        glActiveTexture( GL_TEXTURE0 );

        glUniform1i( glGetUniformLocation( pass->m_program, "flat_normals"), GL_FALSE );
        glUniform1i( glGetUniformLocation( pass->m_program, "use_field" ), item.m_field );
        glUniform1i( glGetUniformLocation( pass->m_program, "log_map"), item.m_field_log_map );
        glUniform1i( glGetUniformLocation( pass->m_program, "solid_pass"), solid_pass ? GL_TRUE : GL_FALSE );
        glUniform1f( glGetUniformLocation( pass->m_program, "line_width"), 0.5f*item.m_line_thickness );

        glUniform4fv( pass->m_loc_surface_color, 1, item.m_face_color );

        glUniform4fv( pass->m_loc_edge_color, 1, item.m_edge_color );
        if( item.m_field_log_map ) {
            glUniform1i( glGetUniformLocation( pass->m_program, "log_map"), GL_TRUE );
            glUniform2f( glGetUniformLocation( pass->m_program, "field_remap"),
                         logf( item.m_field_min ),
                         1.f/(logf( item.m_field_max) - logf( item.m_field_min ) ) );
        }
        else {
            glUniform1i( glGetUniformLocation( pass->m_program, "log_map"), GL_FALSE );
            glUniform2f( glGetUniformLocation( pass->m_program, "field_remap"),
                         item.m_field_min,
                         1.f/(item.m_field_max-item.m_field_min ) );
        }
        glBindTexture( GL_TEXTURE_BUFFER, item.m_surf->triangleCellTexture() );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, item.m_surf->triangleCornerpointIndexBuffer() );
        glDrawElements( GL_TRIANGLES, 3*item.m_surf->triangleCount(), GL_UNSIGNED_INT, NULL );
    }
    glDisable( GL_POLYGON_OFFSET_FILL );
}

void
GridTessSurfRenderer::drawSinglePass(const GLfloat*                  modelview,
                                      const GLfloat*                  projection,
                                      const GLfloat*                  modelview_projection,
                                      const GLfloat*                  normal_transform,
                                      const GLsizei                   width,
                                      const GLsizei                   height,
                                      boost::shared_ptr<const GridTess> tess,
                                      boost::shared_ptr<const GridField> field,
                                      const std::vector<RenderItem>&  render_items )
{
    glBindVertexArray( tess->vertexPositonsAsVertexArrayObject() );

    for( size_t i = 0; i<render_items.size(); i++) {
        const RenderItem& item = render_items[i];

        PassData* pass = &m_pass_data[ PASS_SURFACE_TWOPASS ];
        if(  item.m_edge_color[3] > 0.f ) {
            pass = &m_pass_data[ PASS_SURFACE_TWOPASS_PAINT ];
            glDisable( GL_POLYGON_OFFSET_FILL );
        }
        else {
            glPolygonOffset( 1.f, 1.f );
            glEnable( GL_POLYGON_OFFSET_FILL );
        }
        glUseProgram( pass->m_program );
        glUniformMatrix4fv( pass->m_loc_mvp, 1, GL_FALSE, modelview_projection );
        glUniformMatrix3fv( pass->m_loc_nm, 1, GL_FALSE, normal_transform );
        glUniform2f( pass->m_loc_screen_size, width, height );

        glActiveTexture( GL_TEXTURE2 );
        if( field.get() != NULL ) {
            glBindTexture( GL_TEXTURE_BUFFER, field->texture() );
        }
        else {
            glBindTexture( GL_TEXTURE_BUFFER, 0 );
        }
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_BUFFER, tess->normalVectorsAsBufferTexture() );

        glActiveTexture( GL_TEXTURE0 );

        glUniform1i( glGetUniformLocation( pass->m_program, "flat_normals"), GL_FALSE );

        glUniform4fv( pass->m_loc_edge_color, 1, item.m_edge_color );
        glUniform4fv( pass->m_loc_surface_color, 1, item.m_face_color );

        glUniform1i( glGetUniformLocation( pass->m_program, "use_field" ), item.m_field );
        glUniform1i( glGetUniformLocation( pass->m_program, "log_map"), item.m_field_log_map );
        if( item.m_field_log_map ) {
            glUniform1i( glGetUniformLocation( pass->m_program, "log_map"), GL_TRUE );
            glUniform2f( glGetUniformLocation( pass->m_program, "field_remap"),
                         logf( item.m_field_min ),
                         1.f/(logf( item.m_field_max) - logf( item.m_field_min ) ) );
        }
        else {
            glUniform1i( glGetUniformLocation( pass->m_program, "log_map"), GL_FALSE );
            glUniform2f( glGetUniformLocation( pass->m_program, "field_remap"),
                         item.m_field_min,
                         1.f/(item.m_field_max-item.m_field_min ) );
        }
        glBindTexture( GL_TEXTURE_BUFFER, item.m_surf->triangleCellTexture() );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, item.m_surf->triangleCornerpointIndexBuffer() );
        glDrawElements( GL_TRIANGLES, 3*item.m_surf->triangleCount(), GL_UNSIGNED_INT, NULL );
    }
    glDisable( GL_POLYGON_OFFSET_FILL );
}

void
GridTessSurfRenderer::drawCrappy(const GLfloat*                  modelview,
                                  const GLfloat*                  projection,
                                  const GLfloat*                  modelview_projection,
                                  const GLfloat*                  normal_transform,
                                  const GLsizei                   width,
                                  const GLsizei                   height,
                                  const boost::shared_ptr<const GridTess> tess,
                                  const boost::shared_ptr<const GridField> field,
                                  const std::vector<RenderItem>&  render_items )
{
    glBindVertexArray( tess->vertexPositonsAsVertexArrayObject() );

    glUseProgram( m_surface_crappy_prog );
    glUniformMatrix4fv( m_surface_crappy_loc_mvp, 1, GL_FALSE, modelview_projection );

    for( size_t i = 0; i<render_items.size(); i++) {
        const RenderItem& item = render_items[i];
        glUniform3fv( glGetUniformLocation( m_surface_crappy_prog, "solid_color" ), 1, item.m_face_color );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, item.m_surf->triangleCornerpointIndexBuffer() );
        glDrawElements( GL_TRIANGLES, 3*item.m_surf->triangleCount(), GL_UNSIGNED_INT, NULL );
    }
}

} // of namespace render
