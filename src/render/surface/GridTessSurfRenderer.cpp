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
#include "render/GridTess.hpp"
#include "render/GridField.hpp"
#include "render/surface/GridTessSurf.hpp"
#include "render/surface/GridTessSurfRenderer.hpp"

namespace render {
    namespace  surface {
        namespace glsl {
            extern const std::string GridTessSurfRenderer_common_vs;
            extern const std::string GridTessSurfRenderer_common_gs;
            //extern const std::string GridTessSurfRenderer_frag_list_fs;
            extern const std::string GridTessSurfRenderer_twopass_fs;
            extern const std::string GridTessSurfRenderer_singlepass_fs;
            extern const std::string GridTessSurfRenderer_crappy_fs;
            extern const std::string GridTessSurfRenderer_common_fs;
            //extern const std::string GridTessSurfRenderer_screen_common_vs;
            //extern const std::string GridTessSurfRenderer_screen_colorize_fs;
            //extern const std::string GridTessSurfRenderer_screen_weighted_sum_merge_fs;
            extern const std::string GridTessSurfRenderer_geo_edge_vs;
            extern const std::string GridTessSurfRenderer_geo_edge_fs;
        }

GridTessSurfRenderer::GridTessSurfRenderer()

{
    Logger log = getLogger( "render.GridTessSurfRenderer.constructor" );
    

    for( unsigned int i=0; i<PASS_N; i++ ) {
        m_pass_data[i].m_program = 0;
    }

    GLuint geo_edge_vs = utils::compileShader( log, glsl::GridTessSurfRenderer_geo_edge_vs, GL_VERTEX_SHADER );
    GLuint geo_edge_fs = utils::compileShader( log, glsl::GridTessSurfRenderer_geo_edge_fs, GL_FRAGMENT_SHADER );


    GLuint common_vs = utils::compileShader( log, glsl::GridTessSurfRenderer_common_vs, GL_VERTEX_SHADER );
    GLuint common_gs = utils::compileShader( log, glsl::GridTessSurfRenderer_common_gs, GL_GEOMETRY_SHADER );
    GLuint common_paint_gs = utils::compileShader( log, "#define DO_PAINT\n" + glsl::GridTessSurfRenderer_common_gs, GL_GEOMETRY_SHADER );


    GLuint twopass_fs = utils::compileShader( log, glsl::GridTessSurfRenderer_common_fs +
                                              glsl::GridTessSurfRenderer_twopass_fs,
                                              GL_FRAGMENT_SHADER );
    GLuint twopass_paint_fs = utils::compileShader( log, "#define DO_PAINT\n" +
                                                     glsl::GridTessSurfRenderer_common_fs +
                                                     glsl::GridTessSurfRenderer_twopass_fs,
                                                     GL_FRAGMENT_SHADER );
    GLuint singlepass_fs = utils::compileShader( log, glsl::GridTessSurfRenderer_common_fs +
                                                  glsl::GridTessSurfRenderer_singlepass_fs,
                                                  GL_FRAGMENT_SHADER );
    GLuint singlepass_paint_fs = utils::compileShader( log, "#define DO_PAINT\n" +
                                                        glsl::GridTessSurfRenderer_common_fs +
                                                        glsl::GridTessSurfRenderer_singlepass_fs,
                                                        GL_FRAGMENT_SHADER );

    GLuint crappy_fs = utils::compileShader( log, glsl::GridTessSurfRenderer_crappy_fs, GL_FRAGMENT_SHADER );

    m_pass_data[ PASS_EDGE ].m_program = glCreateProgram();
    glAttachShader( m_pass_data[ PASS_EDGE ].m_program, geo_edge_vs );
    glAttachShader( m_pass_data[ PASS_EDGE ].m_program, geo_edge_fs );

    m_pass_data[ PASS_SURFACE_ONEPASS ].m_program = glCreateProgram();
    glAttachShader( m_pass_data[ PASS_SURFACE_ONEPASS ].m_program, common_vs );
    glAttachShader( m_pass_data[ PASS_SURFACE_ONEPASS ].m_program, common_gs );
    glAttachShader( m_pass_data[ PASS_SURFACE_ONEPASS ].m_program, singlepass_fs );

    m_pass_data[ PASS_SURFACE_ONEPASS_PAINT ].m_program = glCreateProgram();
    glAttachShader( m_pass_data[ PASS_SURFACE_ONEPASS_PAINT ].m_program, common_vs );
    glAttachShader( m_pass_data[ PASS_SURFACE_ONEPASS_PAINT ].m_program, common_paint_gs );
    glAttachShader( m_pass_data[ PASS_SURFACE_ONEPASS_PAINT ].m_program, singlepass_paint_fs );

    m_pass_data[ PASS_SURFACE_TWOPASS ].m_program = glCreateProgram();
    glAttachShader( m_pass_data[ PASS_SURFACE_TWOPASS ].m_program, common_vs );
    glAttachShader( m_pass_data[ PASS_SURFACE_TWOPASS ].m_program, common_gs );
    glAttachShader( m_pass_data[ PASS_SURFACE_TWOPASS ].m_program, twopass_fs );

    m_pass_data[ PASS_SURFACE_TWOPASS_PAINT ].m_program = glCreateProgram();
    glAttachShader( m_pass_data[ PASS_SURFACE_TWOPASS_PAINT ].m_program, common_vs );
    glAttachShader( m_pass_data[ PASS_SURFACE_TWOPASS_PAINT ].m_program, common_paint_gs );
    glAttachShader( m_pass_data[ PASS_SURFACE_TWOPASS_PAINT ].m_program, twopass_paint_fs );


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

    m_surface_crappy_prog = glCreateProgram();
    glAttachShader( m_surface_crappy_prog, common_vs );
    glAttachShader( m_surface_crappy_prog, crappy_fs );
    utils::linkProgram( log, m_surface_crappy_prog );
    m_surface_crappy_loc_mvp = glGetUniformLocation( m_surface_crappy_prog, "MVP" );
    m_surface_crappy_loc_color = glGetUniformLocation( m_surface_crappy_prog, "color" );

    glDeleteShader( geo_edge_vs );
    glDeleteShader( geo_edge_fs );
    glDeleteShader( common_vs );
    glDeleteShader( common_gs );
    glDeleteShader( twopass_fs );
    glDeleteShader( singlepass_fs );
    glDeleteShader( crappy_fs );
}

GridTessSurfRenderer::~GridTessSurfRenderer()
{
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
GridTessSurfRenderer::draw(const GLfloat*                  modelview,
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

    } // of namespace surface
} // of namespace render
