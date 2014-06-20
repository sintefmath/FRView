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
#include "render/subset/BuilderSelectAll.hpp"


namespace render {
    namespace subset {
        namespace glsl {
            extern std::string BuilderSelectAll_vs;
        }

BuilderSelectAll::BuilderSelectAll()
    : Builder( glsl::BuilderSelectAll_vs )
{
}

void
BuilderSelectAll::apply( boost::shared_ptr<Representation> cell_subset,
                         boost::shared_ptr<const mesh::CellSetInterface> cell_set)
{
    glUseProgram( m_program );
    cell_subset->populateBuffer( cell_set );
    glUseProgram( 0 );
}


    } // of namespace subset
} // of namespace render
