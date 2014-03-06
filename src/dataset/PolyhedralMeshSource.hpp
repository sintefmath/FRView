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

#pragma once
#include <memory>
#include <vector>
#include <tinia/model/ExposedModel.hpp>
#include "render/GridTess.hpp"
#include "dataset/AbstractDataSource.hpp"
#include "dataset/PolyhedralDataInterface.hpp"

namespace dataset {

class PolyhedralMeshSource
        : public AbstractDataSource,
          public PolyhedralDataInterface
{
public:
    

    void
    geometry( Tessellation&                                  geometry_bridge,
              boost::shared_ptr<tinia::model::ExposedModel>  model,
              const std::string&                             progress_description_key,
              const std::string&                             progress_counter_key );
    
    void
    field(  Field& field,
            const size_t field_index,
            const size_t timestep_index ) const;
    
    size_t
    fields() const { return m_cell_field_name.size(); }

    size_t
    timesteps() const { return 1; }
    
    const std::string&
    fieldName( unsigned int name_index ) const { return m_cell_field_name[ name_index ]; }
    
    size_t
    activeCells() const { return (m_cells.empty()?0:m_cells.size()-1); }
    
    
protected:
    std::vector<float>                  m_vertices; ///< in R^3.
    std::vector<int>                    m_indices;  ///< Offsets into m_vertices.
    std::vector<int>                    m_polygons; ///< Offsets into m_indices.
    std::vector<int>                    m_cells;    ///< Offsets into m_polygons.

    std::vector<std::string>            m_cell_field_name;
    std::vector< std::vector<float> >   m_cell_field_data;
};


} // of namespace input
