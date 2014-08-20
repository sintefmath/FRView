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

#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "utils/GLSLTools.hpp"
#include "utils/Logger.hpp"
#include "render/wells/Renderer.hpp"
#include "render/wells/Representation.hpp"


namespace render {
    namespace wells {

    namespace glsl {
        extern const std::string well_tube_vs;
        extern const std::string well_tube_cs;
        extern const std::string well_tube_es;
        extern const std::string well_tube_fs;
    }
    
    static const std::string package = "render.WellRenderer";

WellRenderer::WellRenderer()
    : m_well_prog( package + ".m_well_prog" )
{
    Logger log = getLogger( package + ".constructor" );
    

    GLuint well_vs = utils::compileShader( log, glsl::well_tube_vs, GL_VERTEX_SHADER );
    GLuint well_cs = utils::compileShader( log, glsl::well_tube_cs, GL_TESS_CONTROL_SHADER );
    GLuint well_es = utils::compileShader( log, glsl::well_tube_es, GL_TESS_EVALUATION_SHADER );
    GLuint well_fs = utils::compileShader( log, glsl::well_tube_fs, GL_FRAGMENT_SHADER );

    glAttachShader( m_well_prog.get(), well_vs );
    glAttachShader( m_well_prog.get(), well_cs );
    glAttachShader( m_well_prog.get(), well_es );
    glAttachShader( m_well_prog.get(), well_fs );
    utils::linkProgram( log, m_well_prog.get() );

    glDeleteShader( well_vs );
    glDeleteShader( well_cs );
    glDeleteShader( well_es );
    glDeleteShader( well_fs );
}

WellRenderer::~WellRenderer()
{
}

void
WellRenderer::render( GLsizei           width,
                      GLsizei           height,
                      const GLfloat*    projection,
                      const GLfloat*    camera_from_world,
                      const GLfloat*    world_from_model,
                      boost::shared_ptr<Representation> wells )
{
    Logger log = getLogger( package + ".render" );
    if( wells->empty() ) {
        return;
    }
    wells->upload();
    
    glBindVertexArray( wells->attribs().get() );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, wells->indices().get() );
    
    glm::mat3 w_f_m_l( world_from_model[0], world_from_model[1], world_from_model[2],
                       world_from_model[4], world_from_model[5], world_from_model[6],
                       world_from_model[8], world_from_model[9], world_from_model[10] );
    glm::mat3 w_f_m_nm = glm::inverse( w_f_m_l );


    glUseProgram( m_well_prog.get() );
    glPatchParameteri( GL_PATCH_VERTICES, 4 );

    glUniformMatrix4fv( glGetUniformLocation( m_well_prog.get(), "projection" ), 1, GL_FALSE, projection );
    glUniformMatrix4fv( glGetUniformLocation( m_well_prog.get(), "modelview" ), 1, GL_FALSE, camera_from_world );

    glUniformMatrix4fv( glGetUniformLocation( m_well_prog.get(), "world_from_model" ), 1, GL_FALSE, world_from_model );
    glUniformMatrix3fv( glGetUniformLocation( m_well_prog.get(), "world_from_model_nm" ), 1, GL_FALSE, glm::value_ptr(w_f_m_nm) );

    glDrawElements( GL_PATCHES, wells->indexCount(), GL_UNSIGNED_INT, NULL );

    // cleanup
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

}

    } // of namespace wells
} // of namespace render
