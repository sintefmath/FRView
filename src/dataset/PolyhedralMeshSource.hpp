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
#include "dataset/AbstractDataSource.hpp"
#include "dataset/PolyhedralDataInterface.hpp"
#include "dataset/CellLayoutInterface.hpp"

namespace dataset {

/** Simple, concrete implementation of PolyhedralDataInterface. */
class PolyhedralMeshSource
        : public AbstractDataSource,
          public CellLayoutInterface,
          public PolyhedralDataInterface
{
public:
    /** Create a polyhedral mesh directly from arrays.
     *
     * Note that this constructor will take ownership of the contents of the
     * arguments (through vector.swap).
     */
    PolyhedralMeshSource( const std::string&                  name,
                          std::vector<float>&                 vertices,
                          std::vector<int>&                   indices,
                          std::vector<int>&                   polygons,
                          std::vector<int>&                   cells,
                          std::vector<std::string>&           cell_field_name,
                          std::vector< std::vector<float> >&  cell_field_data );

    const std::string&
    name() const { return m_name; }
    
    // -------------------------------------------------------------------------
    /** \name Implementation of PolyhedralDataInterface */
    /** @{ */
    void
    geometry( Tessellation&                                  geometry_bridge,
              boost::shared_ptr<tinia::model::ExposedModel>  model,
              const std::string&                             progress_description_key,
              const std::string&                             progress_counter_key );
    
    void
    field( boost::shared_ptr<Field>  bridge,
           const size_t              field_index,
           const size_t              timestep_index ) const;    

    size_t
    fields() const { return m_cell_field_name.size(); }

    bool
    validFieldAtTimestep( size_t field_index, size_t timestep_index ) const;

    size_t
    timesteps() const { return 1; }

    
    const std::string
    fieldName( unsigned int name_index ) const { return m_cell_field_name[ name_index ]; }
    /** @} */

    // -------------------------------------------------------------------------
    /** \name Implementation of CellLayoutInterface */
    /** @{ */

    int
    indexDim() const;

    int
    maxIndex( int dimension ) const;
    
    /** @} */

    
protected:
    std::string                         m_name;
    std::vector<float>                  m_vertices; ///< in R^3.
    std::vector<int>                    m_indices;  ///< Offsets into m_vertices.
    std::vector<int>                    m_polygons; ///< Offsets into m_indices.
    std::vector<int>                    m_cells;    ///< Offsets into m_polygons.

    std::vector<std::string>            m_cell_field_name;
    std::vector< std::vector<float> >   m_cell_field_data;
};


} // of namespace input
