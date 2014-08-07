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
#include "render/mesh/AbstractMeshGPUModel.hpp"
#include "render/mesh/VertexPositionInterface.hpp"
#include "render/mesh/NormalVectorInterface.hpp"
#include "render/mesh/PolygonSetInterface.hpp"
#include "render/GridField.hpp"
#include "render/surface/GridTessSurf.hpp"
#include "render/surface/Renderer.hpp"
#include "render/surface/TriangleSoup.hpp"
#include "utils/Logger.hpp"
#include "utils/GLSLTools.hpp"

namespace {
    const std::string package = "render.surface.Renderer";
}

namespace render {
    namespace surface {
        namespace glsl {
            extern const std::string Renderer_indexed_vs;
            extern const std::string Renderer_indexed_gs;
            extern const std::string Renderer_indexed_fs;
            extern const std::string Renderer_soup_triangles_vs;
            extern const std::string Renderer_soup_triangles_fs;
            extern const std::string Renderer_soup_edges_vs;
            extern const std::string Renderer_soup_edges_fs;
        }

    
Renderer::Renderer( const std::string& defines, const std::string& fragment_source )
    : m_main( package + ".main" ),
      m_draw_triangle_soup( package + ".draw_triangle_soup" )
{
    Logger log = getLogger( package + ".constructor" );

    GLint shader;


    // -------------------------------------------------------------------------
    {
        // move DO_PAINT into uniform
        shader = utils::compileShader( log, glsl::Renderer_indexed_vs, GL_VERTEX_SHADER );
        glAttachShader( m_main.get(), shader );
        glDeleteShader( shader );

        shader = utils::compileShader( log,
                                       "#define DO_PAINT\n" +
                                       defines +
                                       glsl::Renderer_indexed_gs, GL_GEOMETRY_SHADER );
        glAttachShader( m_main.get(), shader );
        glDeleteShader( shader );

        shader = utils::compileShader( log,
                                       "#define DO_PAINT\n" +
                                       defines +
                                       glsl::Renderer_indexed_fs +
                                       fragment_source, GL_FRAGMENT_SHADER );
        glAttachShader( m_main.get(), shader );
        glDeleteShader( shader );


        utils::linkProgram( log, m_main.get() );

        m_loc_mvp           = glGetUniformLocation( m_main.get(), "MVP" );
        m_loc_mv            = glGetUniformLocation( m_main.get(), "MV" );
        m_loc_nm            = glGetUniformLocation( m_main.get(), "NM" );
        m_loc_surface_color = glGetUniformLocation( m_main.get(), "surface_color" );
        m_loc_edge_color    = glGetUniformLocation( m_main.get(), "edge_color" );
        m_loc_screen_size   = glGetUniformLocation( m_main.get(), "screen_size" );
    }

    // -------------------------------------------------------------------------
    {
        shader = utils::compileShader( log, glsl::Renderer_soup_triangles_vs, GL_VERTEX_SHADER );
        glAttachShader( m_draw_triangle_soup.get(), shader );
        glDeleteShader( shader );

        shader = utils::compileShader( log,
                                       defines +
                                       glsl::Renderer_soup_triangles_fs +
                                       fragment_source, GL_FRAGMENT_SHADER );
        glAttachShader( m_draw_triangle_soup.get(), shader );
        glDeleteShader( shader );

        utils::linkProgram( log, m_draw_triangle_soup.get() );

        m_draw_triangle_soup_loc_mvp           = glGetUniformLocation( m_draw_triangle_soup.get(), "MVP" );
        m_draw_triangle_soup_loc_mv            = glGetUniformLocation( m_draw_triangle_soup.get(), "MV" );
        m_draw_triangle_soup_loc_nm            = glGetUniformLocation( m_draw_triangle_soup.get(), "NM" );
        m_draw_triangle_soup_loc_surface_color = glGetUniformLocation( m_draw_triangle_soup.get(), "surface_color" );
        m_draw_triangle_soup_loc_edge_color    = glGetUniformLocation( m_draw_triangle_soup.get(), "edge_color" );
        m_draw_triangle_soup_loc_screen_size   = glGetUniformLocation( m_draw_triangle_soup.get(), "screen_size" );
    }

    // -------------------------------------------------------------------------
    {
        shader = utils::compileShader( log, glsl::Renderer_soup_edges_vs, GL_VERTEX_SHADER );
        glAttachShader( m_draw_soup_edges_prog.get(), shader );
        glDeleteShader( shader );

        shader = utils::compileShader( log,
                                       defines +
                                       glsl::Renderer_soup_edges_fs +
                                       fragment_source, GL_FRAGMENT_SHADER );
        glAttachShader( m_draw_soup_edges_prog.get(), shader );
        glDeleteShader( shader );

        utils::linkProgram( log, m_draw_soup_edges_prog.get() );

        m_draw_soup_edges_loc_mvp        = glGetUniformLocation( m_draw_soup_edges_prog.get(), "MVP" );
        m_draw_soup_edges_loc_edge_color = glGetUniformLocation( m_draw_soup_edges_prog.get(), "edge_color" );

    }

}

    
void
Renderer::draw( const GLfloat*                            modelview,
                const GLfloat*                            projection,
                const GLsizei                             width,
                const GLsizei                             height,
                const std::vector<RenderItem>&            render_items )
{
    using boost::shared_ptr;
    using boost::dynamic_pointer_cast;
    using mesh::VertexPositionInterface;
    using mesh::NormalVectorInterface;

    Logger log = getLogger( package + ".draw" );
    
     
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
    
    for( size_t i = 0; i<render_items.size(); i++) {

        const RenderItem& item = render_items[i];
        if( item.m_renderer != RenderItem::RENDERER_SURFACE ) {
            continue;
        }
        if( item.m_surf ) {
            if( item.m_surf->triangleCount() < 1 ) {
                continue;
            }
            shared_ptr<const VertexPositionInterface> vertices = dynamic_pointer_cast<const VertexPositionInterface>( item.m_mesh );
            if( !vertices ) {
                continue;
            }
            shared_ptr<const NormalVectorInterface>   normals  = dynamic_pointer_cast<const NormalVectorInterface>(item.m_mesh );
            if( !normals ) {
                continue;
            }

            glUseProgram( m_main.get() );
            glUniformMatrix4fv( m_loc_mvp, 1, GL_FALSE, glm::value_ptr( MVP ) );
            glUniformMatrix3fv( m_loc_nm, 1, GL_FALSE, nm3 );
            glUniformMatrix4fv( m_loc_mv, 1, GL_FALSE, modelview );
            glUniform2f( m_loc_screen_size, width, height );

            glActiveTexture( GL_TEXTURE1 );
            glBindTexture( GL_TEXTURE_BUFFER, normals->normalVectorsAsBufferTexture() );

            glBindVertexArray( vertices->vertexPositonsAsVertexArrayObject() );

            // set up field if enabled
            glActiveTexture( GL_TEXTURE2 );
            if( item.m_field ) {
                glUniform1i( glGetUniformLocation( m_main.get(), "use_field" ), 1 );
                glBindTexture( GL_TEXTURE_BUFFER, item.m_field->texture() );

                if( item.m_color_map ) {
                    glActiveTexture( GL_TEXTURE3 );
                    glBindTexture( GL_TEXTURE_1D, item.m_color_map->get() );
                }
            }
            else {
                glUniform1i( glGetUniformLocation( m_main.get(), "use_field" ), 0 );
                glBindTexture( GL_TEXTURE_BUFFER, 0 );
            }

            //if( item.m_edge_color[3] > 0.f ) {
            //    glDisable( GL_POLYGON_OFFSET_FILL );
            //}
            //else {
            //    glPolygonOffset( 1.f, 1.f );
            //    glEnable( GL_POLYGON_OFFSET_FILL );
            //
            glActiveTexture( GL_TEXTURE0 );

            glUniform1i( glGetUniformLocation( m_main.get(), "flat_normals"), GL_FALSE );
            glUniform1i( glGetUniformLocation( m_main.get(), "log_map"), item.m_field_log_map );
            //        glUniform1i( glGetUniformLocation( m_main.get(), "solid_pass"), solid_pass ? GL_TRUE : GL_FALSE );
            glUniform1f( glGetUniformLocation( m_main.get(), "line_width"), 0.5f*item.m_line_thickness );

            glUniform4fv( m_loc_surface_color, 1, item.m_face_color );

            glUniform4fv( m_loc_edge_color, 1, item.m_edge_color );
            if( item.m_field_log_map ) {
                glUniform1i( glGetUniformLocation( m_main.get(), "log_map"), GL_TRUE );
                glUniform2f( glGetUniformLocation( m_main.get(), "field_remap"),
                             1.f/ item.m_field_min,
                             1.f/logf( item.m_field_max/item.m_field_min ) );
            }
            else {
                glUniform1i( glGetUniformLocation( m_main.get(), "log_map"), GL_FALSE );
                glUniform2f( glGetUniformLocation( m_main.get(), "field_remap"),
                             item.m_field_min,
                             1.f/(item.m_field_max-item.m_field_min ) );
            }
            glBindTexture( GL_TEXTURE_BUFFER, item.m_surf->triangleCellTexture() );
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, item.m_surf->triangleCornerpointIndexBuffer() );
            glDrawElements( GL_TRIANGLES, 3*item.m_surf->triangleCount(), GL_UNSIGNED_INT, NULL );

#if 0
            boost::shared_ptr<const mesh::PolygonSetInterface> polygon_set =
                    boost::dynamic_pointer_cast<const mesh::PolygonSetInterface>( mesh );
            glUseProgram( 0 );
            glMatrixMode( GL_PROJECTION );
            glLoadMatrixf( glm::value_ptr( P ) );
            glMatrixMode( GL_MODELVIEW );
            glLoadMatrixf( glm::value_ptr( M ) );

            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, polygon_set->polygonVertexIndexBuffer() );
            //        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, item.m_surf->triangleCornerpointIndexBuffer() );

            glDrawElements( GL_TRIANGLES, 1024, GL_UNSIGNED_INT, NULL );
            glUseProgram( m_main.get() );
#endif
        }
        else if( item.m_trisoup ) {


            if( item.m_trisoup->triangleVertexCount() > 0 ) {

                glPolygonOffset( 1.f, 1.f );
                glEnable( GL_POLYGON_OFFSET_FILL );

                glUseProgram( m_draw_triangle_soup.get() );
                glUniformMatrix4fv( m_draw_triangle_soup_loc_mvp, 1, GL_FALSE, glm::value_ptr( MVP ) );
                glUniformMatrix3fv( m_draw_triangle_soup_loc_nm, 1, GL_FALSE, nm3 );
                glUniformMatrix4fv( m_draw_triangle_soup_loc_mv, 1, GL_FALSE, modelview );
                glUniform2f( m_draw_triangle_soup_loc_screen_size, width, height );

                glUniform4fv( m_draw_triangle_soup_loc_surface_color, 1, item.m_face_color );

                glBindVertexArray( item.m_trisoup->triangleVertexAttributesAsVertexArrayObject() );

                glDrawArrays( GL_TRIANGLES, 0, item.m_trisoup->triangleVertexCount() );

                glDisable( GL_POLYGON_OFFSET_FILL );

            }
            if( (item.m_trisoup->edgeVertexCount() > 0) /*&& (item.m_edge_color[3] > 0.f )*/ ) {
                glUseProgram( m_draw_soup_edges_prog.get() );
                glUniformMatrix4fv( m_draw_soup_edges_loc_mvp, 1, GL_FALSE, glm::value_ptr( MVP ) );
                glUniform4fv( m_draw_soup_edges_loc_edge_color, 1, item.m_edge_color );

                glBindVertexArray( item.m_trisoup->edgeVertexAttributesAsVertexArrayObject() );
                glDrawArrays( GL_LINES, 0, item.m_trisoup->edgeVertexCount() );
            }
        }
    }
    glDisable( GL_POLYGON_OFFSET_FILL );



}


    } // of namespace surface
} // of namespace render
