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
#include "bridge/PolygonMeshBridge.hpp"
#include "bridge/FieldBridge.hpp"
#include "dataset/PolygonMeshSource.hpp"

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

void
PolygonMeshSource::geometry( boost::shared_ptr<bridge::PolygonMeshBridge>   mesh_bridge,
                             boost::shared_ptr<tinia::model::ExposedModel>  model,
                             const std::string&                             progress_description_key,
                             const std::string&                             progress_counter_key )
{
    typedef bridge::PolygonMeshBridge::Real     Real;
    typedef bridge::PolygonMeshBridge::Real4    Real4;
    typedef bridge::PolygonMeshBridge::Index    Index;
    typedef bridge::PolygonMeshBridge::Segment  Segment;
    
    for( size_t i=0; i<m_vertices.size(); i+=3 ) {
        mesh_bridge->addVertex( Real4( m_vertices[i+0],
                                       m_vertices[i+1],
                                       m_vertices[i+2] ) );
    }

    std::vector<Segment> segments;
    for( size_t c=0; (c+1)<m_cells.size(); c++ ) {
        Index cell_ix = mesh_bridge->addCell();

        for( int p=m_cells[c]; p<m_cells[c+1]; p++ ) {
            // create normal vector
            glm::vec3 a = glm::make_vec3( m_vertices.data() + 3*m_indices[ m_polygons[p] + 0 ] );
            glm::vec3 b = glm::make_vec3( m_vertices.data() + 3*m_indices[ m_polygons[p] + 1 ] );
            glm::vec3 c = glm::make_vec3( m_vertices.data() + 3*m_indices[ m_polygons[p] + 2 ] );
            glm::vec3 n = glm::normalize( glm::cross( b-a, c-a ) );
            Index nrm_ix = mesh_bridge->addNormal( Real4( n.x, n.y, n.z ) );

            // create segments for polygon
            segments.clear();
            for( int o=m_polygons[p]; o<m_polygons[p+1]; o++ ) {
                segments.push_back( Segment(  nrm_ix, m_indices[ o ], 3 ) );
            }

            // add polygon
            mesh_bridge->addPolygon( cell_ix, segments.data(), segments.size() );
        }
    }
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
