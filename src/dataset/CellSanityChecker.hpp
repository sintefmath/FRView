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
#include <vector>

class CellSanityChecker
{
public:
    CellSanityChecker();

    void
    addTriangle( unsigned int id,
                 unsigned int v0, unsigned int v1, unsigned int v2 );


    void
    addPolygon( unsigned int id,
                const std::vector<unsigned int>& vertices );

    void
    addPolygonReverse( unsigned int id,
                       const std::vector<unsigned int>& vertices );

    bool
    checkTriangleTopology();

    bool
    checkPolygonTopology();

protected:
    unsigned int                m_id;
    std::vector<unsigned int>   m_triangles;

    std::vector<unsigned int>   m_polygon_offsets;
    std::vector<unsigned int>   m_polygon_vertices;


};
