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

namespace dataset {

class PolyhedralMeshSource
{
public:
    
protected:
    std::vector<float>                  m_vertices; ///< in R^3.
    std::vector<int>                    m_indices;  ///< Offsets into m_vertices.
    std::vector<int>                    m_polygons; ///< Offsets into m_indices.
    std::vector<int>                    m_cells;    ///< Offsets into m_polygons.

    std::vector<std::string>            m_cell_field_name;
    std::vector< std::vector<float> >   m_cell_field_data;
};




template<typename Tessellation>
class TetraMesh
{
public:
    /** \name Forwarding
      * Forwarding of tessellation types and constants. */
    /** \@{ */
    typedef float                               SrcReal;
    typedef typename Tessellation::Real         Real;
    typedef typename Tessellation::Index        Index;
    typedef typename Tessellation::Real4        Real4;
    typedef typename Tessellation::Orientation  Orientation;
    typedef typename Tessellation::Segment      Segment;
    typedef typename Tessellation::Interface    Interface;
    static const Orientation ORIENTATION_I = Tessellation::ORIENTATION_I;
    static const Orientation ORIENTATION_J = Tessellation::ORIENTATION_J;
    static const Orientation ORIENTATION_K = Tessellation::ORIENTATION_K;
    static const Index IllegalIndex = (~(Index)0u);
    /** @} */

    TetraMesh( Tessellation& tessellation );
    
    void
    parse( boost::shared_ptr<tinia::model::ExposedModel> model,
           const std::vector<SrcReal>&                   vertices,
           const std::vector<int>&                       indices );
    
private:
    Tessellation&                   m_tessellation;
    
};




} // of namespace input
