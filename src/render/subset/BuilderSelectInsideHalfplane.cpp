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

#include "render/GridTess.hpp"
#include "render/subset/Representation.hpp"
#include "render/subset/BuilderSelectInsideHalfplane.hpp"

namespace render {
    namespace subset {
        namespace glsl {
            extern std::string BuilderSelectInsideHalfplane_vs;
        }

BuilderSelectInsideHalfplane::BuilderSelectInsideHalfplane()
    : Builder( glsl::BuilderSelectInsideHalfplane_vs )
{
    glUseProgram( m_program );
    m_loc_halfplane_eq = glGetUniformLocation( m_program, "plane_equation" );
    glUniform1i( glGetUniformLocation( m_program, "vertices" ), 0 );
    glUniform1i( glGetUniformLocation( m_program, "cell_corner" ), 1 );
    glUseProgram( 0 );
}

void
BuilderSelectInsideHalfplane::apply(boost::shared_ptr<Representation> tess_subset,
                          boost::shared_ptr<const GridTess> tess,
                          const float *equation )
{
    glUseProgram( m_program );
    glUniform4fv( m_loc_halfplane_eq, 1, equation );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->vertexPositionsAsBufferTexture() );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->cellCornerTexture() );
    tess_subset->populateBuffer( tess );
    glUseProgram( 0 );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
}

    } // of namespace subset
} // of namespace render
