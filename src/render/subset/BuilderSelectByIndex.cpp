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

#include "render/mesh/CellSetInterface.hpp"
#include "render/subset/Representation.hpp"
#include "render/subset/BuilderSelectByIndex.hpp"

namespace render {
    namespace subset {
        namespace glsl {
            extern std::string BuilderSelectByIndex_vs;
        }

BuilderSelectByIndex::BuilderSelectByIndex()
        : Builder( glsl::BuilderSelectByIndex_vs )
{
    glUseProgram( m_program );
    m_loc_grid_dim = glGetUniformLocation( m_program, "grid_dim" );
    m_loc_index_min = glGetUniformLocation( m_program, "index_min" );
    m_loc_index_max = glGetUniformLocation( m_program, "index_max" );
    glUniform1i( glGetUniformLocation( m_program, "cell_global_index"), 0 );
    glUseProgram( 0 );
}

void
BuilderSelectByIndex::apply( boost::shared_ptr<Representation> cell_subset,
                             boost::shared_ptr<const mesh::CellSetInterface> cell_set,
                      unsigned int n_i,
                      unsigned int n_j,
                      unsigned int n_k,
                      unsigned int min_i,
                      unsigned int min_j,
                      unsigned int min_k,
                      unsigned int max_i,
                      unsigned int max_j,
                      unsigned int max_k )
{
    glUseProgram( m_program );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, cell_set->cellGlobalIndexTexture() );
    glUniform3ui( m_loc_grid_dim, n_i, n_j, n_k );
    glUniform3ui( m_loc_index_min, min_i, min_j, min_k );
    glUniform3ui( m_loc_index_max, max_i, max_j, max_k );
    cell_subset->populateBuffer( cell_set );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glUseProgram( 0 );
}

    } // of namespace subset
} // of namespace render
