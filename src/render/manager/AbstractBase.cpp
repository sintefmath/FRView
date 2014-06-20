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

AbstractBase::AbstractBase( const models::RenderConfig& appearance,
                            const GLsizei width,
                            const GLsizei height )
    : m_width( width ),
      m_height( height ),
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
AbstractBase::expired( const models::RenderConfig& appearance )
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
    case models::RenderConfig::Solid:
        ret += "#define SHADING_MODEL_SOLID\n";
        break;
    case models::RenderConfig::Diffuse:
        ret += "#define SHADING_MODEL_DIFFUSE\n";
        ret += "#define SHADING_DIFFUSE_COMPONENT\n";
        break;
    case models::RenderConfig::DiffuseSpecular:
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
    boost::shared_ptr<const GLTexture> color_map;
    
    bool redo_text = false;

    bool has_field = false;
    for( size_t i=0; i<items.size(); i++ ) {
        const RenderItem& item = items[i];
        if( (item.m_renderer != RenderItem::RENDERER_SURFACE)
                || (!item.m_field )
                || (!item.m_color_map))
        {
            continue;
        }
        has_field = true;
        color_map = item.m_color_map;
        
        if( (m_legend_max != item.m_field_max )
                || (m_legend_min != item.m_field_min )
                || (m_legend_log != item.m_field_log_map ) )
        {
            m_legend_max = item.m_field_max;
            m_legend_min = item.m_field_min;
            m_legend_log = item.m_field_log_map;
            redo_text = true;
        }
    }
    if( !has_field ) {
        return;
    }
    
    if( redo_text ) {
        m_legend_text.clear();

        
        for( int i=0; i<10; i++ ) {
            float t = i/9.f;
            std::stringstream o;
            o << std::setprecision(4 );
            if( m_legend_log ) {
                o << (std::pow( m_legend_max/m_legend_min, t )*m_legend_min);
            }
            else {
                o << (m_legend_min*(1.f-t) + t*m_legend_max);
            }
            GLfloat pos[3] = { -1.f, 2.f*t-1.f, 0.f };
            m_legend_text.add( o.str(), render::TextRenderer::FONT_8X12,
                               pos, 2.f, render::TextRenderer::ANCHOR_W );
        }
    }

    GLfloat MP[16] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 0.8f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };

    
    if( !items.empty()  ) {
        glDisable( GL_DEPTH_TEST );
        glViewport( 35, glm::max(0, height-130), 120, 120 );
        m_legend_text.render( 120, 120, MP );
        
        glViewport( 10, glm::max(0, height-130), 20, 120 );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_1D, color_map->get() );
        
        glUseProgram( m_legend_prog.get() );
        glUniformMatrix4fv( glGetUniformLocation( m_legend_prog.get(), "MP"),
                            1, GL_FALSE, MP );
        
        glBindVertexArray( m_fsq_vao.get() );
        glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );    
        glBindVertexArray( 0 );
    }
}


    } // of namespace screen
} // of namespace render
