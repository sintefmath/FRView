/* Copyright STIFTELSEN SINTEF 2014
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

#include <glm/gtc/type_ptr.hpp>
#include "models/Appearance.hpp"
#include "render/rlgen/SplatRenderer.hpp"
#include "render/rlgen/VoxelGrid.hpp"
#include "render/rlgen/Splats.hpp"
#include "render/GridField.hpp"
#include "utils/GLSLTools.hpp"

namespace {
    const std::string package = "render.rlgen.SplatRenderer";
}

namespace render {
namespace rlgen {
namespace glsl {
    extern const std::string SplatRenderer_vs;
    extern const std::string SplatRenderer_fs;
    extern const std::string SplatRenderer_gs;
}

    
SplatRenderer::SplatRenderer()
    : m_program( package + ".m_program" )
{

    m_program.addShaderStage( glsl::SplatRenderer_vs, GL_VERTEX_SHADER );
    m_program.addShaderStage( glsl::SplatRenderer_gs, GL_GEOMETRY_SHADER );
    m_program.addShaderStage( glsl::SplatRenderer_fs, GL_FRAGMENT_SHADER );

    m_program.link();
    
    m_loc_slice         = m_program.uniformLocation( "slice" );
    m_loc_field_remap   = m_program.uniformLocation( "field_remap" );
    m_loc_use_field     = m_program.uniformLocation( "use_field" );
    m_loc_log_map       = m_program.uniformLocation( "log_map" );
    m_loc_surface_color = m_program.uniformLocation( "surface_color" );

}
    
void
SplatRenderer::apply( boost::shared_ptr<GridVoxelization>               target,
                      const std::list<boost::shared_ptr<SourceItem> >&  items )
{
    Logger log = getLogger( package + ".apply" );

    GLsizei dim[3];
    target->dimension( dim );

    m_program.use();
    m_slice_fbo.bind();

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );
    glClearColor( 0.f, 0.f, 0.f, 0.f );
    glViewport( 1, 1, dim[0]-2, dim[1]-2 );
    
    GLTexture& voxels = target->voxelTexture();
    for(GLsizei i=0; i<dim[2]; i++ ) {
        glFramebufferTexture3D( GL_FRAMEBUFFER,
                                GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_3D,
                                voxels.get(),
                                0,
                                i );
        utils::checkFBO( log );
        glClear( GL_COLOR_BUFFER_BIT );
        if( (0 < i) && (i+1<dim[2]) ) {
            typedef std::list<boost::shared_ptr<SourceItem> >::const_iterator iterator;
            
            glUniform1f( m_loc_slice, (i-0.5f)/(dim[2]-2.f) );
            for( iterator it=items.begin(); it!=items.end(); ++it ) {
                if( !(*it)->m_appearance_data ) {
                    continue;
                }
                
                // if source has a field associated
                if( (*it)->m_grid_field && (*it)->m_color_map ) {
                    glUniform1i( m_loc_use_field, GL_TRUE );

                    glActiveTexture( GL_TEXTURE0 );
                    glBindTexture( GL_TEXTURE_BUFFER, (*it)->m_grid_field->texture() );
                    glActiveTexture( GL_TEXTURE1 );
                    glBindTexture( GL_TEXTURE_1D, (*it)->m_color_map->get() );

                    if( (*it)->m_appearance_data->colorMapType() == models::AppearanceData::COLORMAP_LOGARITMIC ) {
                        glUniform1i( m_loc_log_map, GL_TRUE );
                        // fixme: min and map should come from appearance
                        glUniform2f( m_loc_field_remap,
                                     1.f/(*it)->m_grid_field->minValue(),
                                     1.f/logf( (*it)->m_grid_field->maxValue()/(*it)->m_grid_field->minValue() ) );
                    }
                    else {
                        glUniform1i( m_loc_log_map, GL_FALSE );
                        glUniform2f( m_loc_field_remap,
                                     (*it)->m_grid_field->minValue(),
                                     1.f/((*it)->m_grid_field->maxValue()-(*it)->m_grid_field->minValue() ) );
                    }
                }
                // otherwise, use surface color
                else {
                    glm::vec3 color = (*it)->m_appearance_data->subsetColor();
                    
                    // fixme: surface color should come from appearance
                    glUniform1i( m_loc_use_field, GL_FALSE );
                    glUniform3fv( m_loc_surface_color, 1, glm::value_ptr( color ) );
                }
                glBindVertexArray( (*it)->m_splats->asAttributes().get() );
                glDrawArrays( GL_POINTS, 0, (*it)->m_splats->count() );
            }
        }
    }
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_1D, 0 );
    glBindVertexArray( 0 );
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}
    

            
        
    } // of namespace rlgen
} // of namespace render
