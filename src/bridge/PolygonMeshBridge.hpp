#pragma once
/* Copyright STIFTELSEN SINTEF 2014
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

#include <vector>
#include "bridge/AbstractMeshBridge.hpp"

namespace bridge {

class PolygonMeshBridge
        : public AbstractMeshBridge
{
public:
    PolygonMeshBridge( bool triangulate );

    Index
    addVertex( const Real4 pos );
    
    Index
    addNormal( const Real4 dir );

    Index
    addCell();
    
    Index
    addPolygon( const Index cell,
                const Segment* segments,
                const Index N );

    void
    boundingBox( Real4& minimum, Real4& maximum ) const;
    
    void
    process();
    
protected:
    bool                m_triangulate;      ///< If true, triangulate polygons.
    Index               m_max_corners;      ///< Max number of corners in a polygon.

    std::vector<Real4>  m_vertices;         ///< Vertex positions.
    std::vector<Real4>  m_normals;          ///< Normal directions.

    std::vector<Index>  m_polygon_vtx_ix;   ///< Polygon corner index into m_vertices.
    std::vector<Index>  m_polygon_nrm_ix;   ///< Polygon corner index into m_normals;
    std::vector<Index>  m_polygon_offset;   ///< polygons_N+1 offsets into m_polygon_vtx_ix and m_polygon_nrm_ix.
    std::vector<Index>  m_polygon_cell;     ///< polygons_N cell indices.

    std::vector<Index>  m_cell_corners;     ///< Unique corners for a given cell.
    std::vector<Index>  m_cell_offset;      ///< cell_N+1 indices into m_cell_corners.
    
};


} // of namespace bridge
