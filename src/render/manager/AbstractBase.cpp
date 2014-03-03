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

#include <iostream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "utils/Logger.hpp"
#include "utils/GLSLTools.hpp"
#include "render/wells/Representation.hpp"
#include "render/manager/AbstractBase.hpp"


namespace render {
    namespace manager {
        namespace glsl {
            extern const std::string AbstractBase_legend_vs;
            extern const std::string AbstractBase_legend_fs;
        }
        static const std::string package = "render.manager.AbstractBase";

AbstractBase::AbstractBase( const models::Appearance& appearance,
                            const GLsizei width,
                            const GLsizei height )
    : m_width( width ),
      m_height( height ),
      m_color_map( "color_map" ),
      m_appearance_revision( appearance.revision() ),
      m_shading_model( appearance.shadingModel() ),
      m_legend_min( std::numeric_limits<float>::max() ),
      m_legend_max( std::numeric_limits<float>::max() )
{
    Logger log = getLogger( package + ".constructor" );
    
    // --- create full-screen quad buffer and vertex array ---------------------
    static const GLfloat quad[ 4*4 ] = {
         1.f, -1.f, 0.f, 1.f,
         1.f,  1.f, 0.f, 1.f,
        -1.f, -1.f, 0.f, 1.f,
        -1.f,  1.f, 0.f, 1.f
    };
    glBindVertexArray( m_fsq_vao.get() );
    glBindBuffer( GL_ARRAY_BUFFER, m_fsq_buf.get() );
    glBufferData( GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 0, NULL );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    // --- create color map texture --------------------------------------------
    std::vector<glm::vec3> ramp( 256 );
    for(size_t i=0; i<ramp.size(); i++ ) {
        float scalar = static_cast<float>(i)/static_cast<float>(ramp.size()-1);
        
        ramp[i] = glm::clamp( glm::sin( glm::vec3( 4.14f*(scalar - 0.5f),
                                                   3.14f*(scalar),
                                                   4.14f*(scalar + 0.5f) ) ),
                              glm::vec3( 0.f ),
                              glm::vec3( 1.f ) );
    }
    
    glBindTexture( GL_TEXTURE_1D, m_color_map.get() );
    glTexImage1D( GL_TEXTURE_1D, 0, GL_RGB, 256, 0, GL_RGB, GL_FLOAT, ramp.data() );
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture( GL_TEXTURE_1D, 0 );
    
    // --- create legend shader ------------------------------------------------
    GLuint vs = utils::compileShader( log, glsl::AbstractBase_legend_vs, GL_VERTEX_SHADER );
    GLuint fs = utils::compileShader( log, glsl::AbstractBase_legend_fs, GL_FRAGMENT_SHADER );
    glAttachShader( m_legend_prog.get(), vs );
    glAttachShader( m_legend_prog.get(), fs );
    utils::linkProgram( log, m_legend_prog.get() );
    glDeleteShader( vs );
    glDeleteShader( fs );    
}
    
AbstractBase::~AbstractBase()
{
}

bool
AbstractBase::expired( const models::Appearance& appearance )
{
    if( m_appearance_revision != appearance.revision() ) {
        if( appearance.shadingModel() != m_shading_model ) {
            return true;
        }
    }
    return false;
}

const std::string
AbstractBase::defines() const
{
    std::string ret;
    switch( m_shading_model ) {
    case models::Appearance::Solid:
        ret += "#define SHADING_MODEL_SOLID\n";
        break;
    case models::Appearance::Diffuse:
        ret += "#define SHADING_MODEL_DIFFUSE\n";
        ret += "#define SHADING_DIFFUSE_COMPONENT\n";
        break;
    case models::Appearance::DiffuseSpecular:
        ret += "#define SHADING_MODEL_DIFFUSE_SPECULAR\n";
        ret += "#define SHADING_DIFFUSE_COMPONENT\n";
        ret += "#define SHADING_SPECULAR_COMPONENT\n";
        break;
    }
    return ret;
}


void
AbstractBase::renderMiscellaneous( const GLsizei                       width,
                                   const GLsizei                       height,
                                   const GLfloat*                      local_to_world,
                                   const GLfloat*                      modelview,
                                   const GLfloat*                      projection,
                                   const std::vector<RenderItem>&      items )
{
    glm::mat4 MVP =
            glm::make_mat4( projection ) *
            glm::make_mat4( modelview ) *
            glm::make_mat4( local_to_world );
            
            

    for( size_t i=0; i<items.size(); i++ ) {
        if( items[i].m_renderer == RenderItem::RENDERER_WELL ) {
            m_well_renderer.render( width,
                                    height,
                                    projection,
                                    modelview,
                                    local_to_world,
                                    items[i].m_well );
            items[i].m_well->wellHeads().render( width,
                                                    height,
                                                    glm::value_ptr( MVP ) );
        }
    }
    
}

void
AbstractBase::renderOverlay( const GLsizei                       width,
                             const GLsizei                       height,
                             const GLfloat*                      local_to_world,
                             const GLfloat*                      modelview,
                             const GLfloat*                      projection,
                             const std::vector<RenderItem>&      items )
{
    bool redo_text = false;

    bool has_log_map = false;
    bool has_field = false;
    for( size_t i=0; i<items.size(); i++ ) {
        const RenderItem& item = items[i];
        if( (item.m_renderer != RenderItem::RENDERER_SURFACE)
                || (!item.m_field ) )
        {
            continue;
        }
        has_field = true;
        has_log_map = item.m_field_log_map;
        
        if( (m_legend_max != item.m_field_max )
                || (m_legend_min != item.m_field_min ) )
        {
            m_legend_max = item.m_field_max;
            m_legend_min = item.m_field_min;
            redo_text = true;
        }
    }
    if( !has_field ) {
        return;
    }
    if( redo_text ) {
        m_legend_text.clear();

        for( int i=0; i<5; i++ ) {
            glm::vec3 pos = glm::vec3( -1.f, 2.f*(i/4.f)-1.f, 0.f );
            
            std::stringstream o;
            o << i;
            m_legend_text.add( o.str(),
                               render::TextRenderer::FONT_8X12,
                               glm::value_ptr( pos ),
                               2.f, 
                               render::TextRenderer::ANCHOR_W );
        }
        
        
    }

    GLfloat MP[16] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 0.8f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };

    
    glDisable( GL_DEPTH_TEST );
    glViewport( 35, glm::max(0, height-130), 120, 120 );
    
//    glViewport( 0, 0, 100, 100 );
    m_legend_text.render( 120, 120, MP );
    
    glUseProgram( 0 );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glMatrixMode( GL_MODELVIEW );
    glLoadMatrixf( MP );
    
    glColor3f( 1.f, 1.f, 0.f );
    glPointSize( 3.f );
    glBegin( GL_POINTS );
    for( int i=0; i<5; i++ ) {
        glm::vec3 pos = glm::vec3( -1.f, 2.f*(i/4.f)-1.f, 0.f );
        glVertex3fv( glm::value_ptr( pos ) );
    }
    glEnd();
    
    
    glViewport( 10, glm::max(0, height-130), 20, 120 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_1D, m_color_map.get() );
    
    glUseProgram( m_legend_prog.get() );
    glUniform1i( glGetUniformLocation( m_legend_prog.get(), "log_map"),
                 has_log_map ? 1 : 0 );
    
    glUniformMatrix4fv( glGetUniformLocation( m_legend_prog.get(), "MP"),
                        1, GL_FALSE, MP );
    
    glBindVertexArray( m_fsq_vao.get() );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );    
    glBindVertexArray( 0 );
}


    } // of namespace screen
} // of namespace render
