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
#include "render/GridField.hpp"
#include "render/subset/Representation.hpp"
#include "render/subset/BuilderSelectByFieldValue.hpp"

namespace render {
    namespace subset {
    namespace glsl {
        extern std::string BuilderSelectByFieldValue_vs;
    }

BuilderSelectByFieldValue::BuilderSelectByFieldValue()
    : Builder( glsl::BuilderSelectByFieldValue_vs )
{
    m_loc_min_max = glGetUniformLocation( m_program, "min_max" );
}

void
BuilderSelectByFieldValue::apply( boost::shared_ptr<Representation> tess_subset,
                     boost::shared_ptr<const GridTess> tess,
                     boost::shared_ptr<const GridField> field,
                     const float minval,
                     const float maxval)
{
    glUseProgram( m_program );
    glUniform2f( m_loc_min_max, minval, maxval );
    glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_BUFFER, field->texture() );
    tess_subset->populateBuffer( tess );
    glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glUseProgram( 0 );
}

    } // of namespace subset
} // of namespace render
