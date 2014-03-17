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

#include "utils/Logger.hpp"
#include "utils/GLSLTools.hpp"
#include "render/mesh/CellSetInterface.hpp"
#include "render/GridField.hpp"
#include "render/subset/Representation.hpp"
#include "render/subset/Builder.hpp"

namespace resources {
    extern const std::string all_select_vs;
    extern const std::string field_select_vs;
    extern const std::string index_select_vs;
    extern const std::string halfplane_select_vs;
    extern const std::string plane_select_vs;
}
namespace render {
    namespace subset {

Builder::Builder( const std::string& vs )
{
    Logger log = getLogger( "render.CellSelector.constructor" );
    m_program = glCreateProgram();
    GLuint v = utils::compileShader( log, vs, GL_VERTEX_SHADER );
    glAttachShader( m_program, v );
    const char* varyings[1] = {
        "selected"
    };
    glTransformFeedbackVaryings( m_program, 1, varyings, GL_INTERLEAVED_ATTRIBS );
    utils::linkProgram( log, m_program );
    glDeleteShader( v );
}

Builder::~Builder()
{
    glDeleteProgram( m_program );
}


    } // of namespace subset
} // of namespace render
