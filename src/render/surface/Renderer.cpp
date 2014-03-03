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
#include "render/GridTess.hpp"
#include "render/GridField.hpp"
#include "render/surface/GridTessSurf.hpp"
#include "render/surface/Renderer.hpp"
#include "utils/Logger.hpp"
#include "utils/GLSLTools.hpp"

namespace render {
    namespace surface {
        namespace glsl {
            extern const std::string Renderer_vs;
            extern const std::string Renderer_gs;
            extern const std::string Renderer_fs;
        }
        static const std::string package = "render.surface.Renderer";

    
Renderer::Renderer( const std::string& defines, const std::string& fragment_source )
    : m_main( package + ".main" )
{
    Logger log = getLogger( package + ".constructor" );

    // move DO_PAINT into uniform
    
    GLint vs = utils::compileShader( log, glsl::Renderer_vs, GL_VERTEX_SHADER );
    GLint gs = utils::compileShader( log,
                                     "#define DO_PAINT\n" +
                                     defines +
                                     glsl::Renderer_gs, GL_GEOMETRY_SHADER );
    GLint fs = utils::compileShader( log,
                                     "#define DO_PAINT\n" +
                                     defines +
                                     glsl::Renderer_fs +
                                     fragment_source, GL_FRAGMENT_SHADER );

    glAttachShader( m_main.get(), vs );
    glAttachShader( m_main.get(), gs );
    glAttachShader( m_main.get(), fs );
    utils::linkProgram( log, m_main.get() );

    m_loc_mvp           = glGetUniformLocation( m_main.get(), "MVP" );
    m_loc_mv            = glGetUniformLocation( m_main.get(), "MV" );
    m_loc_nm            = glGetUniformLocation( m_main.get(), "NM" );
    m_loc_surface_color = glGetUniformLocation( m_main.get(), "surface_color" );
    m_loc_edge_color    = glGetUniformLocation( m_main.get(), "edge_color" );
    m_loc_screen_size   = glGetUniformLocation( m_main.get(), "screen_size" );
}

    
void
Renderer::draw( const GLfloat*                                        modelview,
                const GLfloat*                                        projection,
                const GLsizei                                         width,
                const GLsizei                                         height,
                const boost::shared_ptr<const GridTess>               tess,
                const boost::shared_ptr<const GridField>              field,
                const std::vector<RenderItem>&  render_items )
{

    
    
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
    
    
    glUseProgram( m_main.get() );
    glUniformMatrix4fv( m_loc_mvp, 1, GL_FALSE, glm::value_ptr( MVP ) );
    glUniformMatrix3fv( m_loc_nm, 1, GL_FALSE, nm3 );
    glUniformMatrix4fv( m_loc_mv, 1, GL_FALSE, modelview );
    glUniform2f( m_loc_screen_size, width, height );

    // bind normal vector texture
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->normalVectorsAsBufferTexture() );

    // bind field values, if available
    glActiveTexture( GL_TEXTURE2 );
    if( field.get() != NULL ) {
        glBindTexture( GL_TEXTURE_BUFFER, field->texture() );
    }
    else {
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
    }

    // Bind vertex position VAO
    glBindVertexArray( tess->vertexPositonsAsVertexArrayObject() );
   
    for( size_t i = 0; i<render_items.size(); i++) {
        const RenderItem& item = render_items[i];
        if( item.m_renderer != RenderItem::RENDERER_SURFACE ) {
            continue;
        }

        if( item.m_edge_color[3] > 0.f ) {
            glDisable( GL_POLYGON_OFFSET_FILL );
        }
        else {
            glPolygonOffset( 1.f, 1.f );
            glEnable( GL_POLYGON_OFFSET_FILL );
        }
        glActiveTexture( GL_TEXTURE0 );

        glUniform1i( glGetUniformLocation( m_main.get(), "flat_normals"), GL_FALSE );
        glUniform1i( glGetUniformLocation( m_main.get(), "use_field" ), item.m_field );
        glUniform1i( glGetUniformLocation( m_main.get(), "log_map"), item.m_field_log_map );
//        glUniform1i( glGetUniformLocation( m_main.get(), "solid_pass"), solid_pass ? GL_TRUE : GL_FALSE );
        glUniform1f( glGetUniformLocation( m_main.get(), "line_width"), 0.5f*item.m_line_thickness );

        glUniform4fv( m_loc_surface_color, 1, item.m_face_color );

        glUniform4fv( m_loc_edge_color, 1, item.m_edge_color );
        if( item.m_field_log_map ) {
            glUniform1i( glGetUniformLocation( m_main.get(), "log_map"), GL_TRUE );
            glUniform2f( glGetUniformLocation( m_main.get(), "field_remap"),
                         logf( item.m_field_min ),
                         1.f/(logf( item.m_field_max) - logf( item.m_field_min ) ) );
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
    }
    glDisable( GL_POLYGON_OFFSET_FILL );

}


    } // of namespace surface
} // of namespace render
