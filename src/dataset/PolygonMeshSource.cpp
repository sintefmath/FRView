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
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "dataset/PolygonMeshSource.hpp"
#include "render/GridTessBridge.hpp"
#include "render/GridFieldBridge.hpp"

namespace {

const std::string package = "datasource.PolygonMeshSource";



}


namespace dataset {

PolygonMeshSource::PolygonMeshSource( std::vector<float>&                 vertices,
                                      std::vector<int>&                   indices,
                                      std::vector<int>&                   polygons,
                                      std::vector<int>&                   cells,
                                      std::vector<std::string>&           cell_field_name,
                                      std::vector< std::vector<float> >&  cell_field_data )
{
    m_vertices.swap( vertices );
    m_indices.swap( indices );
    m_polygons.swap( polygons );
    m_cells.swap( cells );
    m_cell_field_name.swap( cell_field_name );
    m_cell_field_data.swap( cell_field_data );
}


bool
PolygonMeshSource::validFieldAtTimestep( size_t field_index, size_t timestep_index ) const
{
    if( timestep_index == 0 ) {
        if( field_index < m_cell_field_data.size() ) {
            return true;
        }
    }
    return false;
}


int
PolygonMeshSource::indexDim() const
{
    return 1;
}

int
PolygonMeshSource::maxIndex( int dimension ) const
{
    if( dimension == 0 ) {
        return std::max(1,(int)m_cells.size())-1;
    }
    else {
        return 0;
    }
}


} // of namespace input
