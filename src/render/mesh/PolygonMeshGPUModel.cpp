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

#include "utils/Logger.hpp"
#include "bridge/PolygonMeshBridge.hpp"
#include "render/mesh/PolygonMeshGPUModel.hpp"

namespace {

const std::string package = "render.mesh.PolygonMeshGPUModel";

}

namespace render {
    namespace mesh {

using boost::shared_ptr; 
using bridge::PolygonMeshBridge;
    

PolygonMeshGPUModel::PolygonMeshGPUModel()
    : m_vertices_num( 0 ),
      m_vertex_positions_buf( package + ".m_vertex_positions_buf" ),
      m_vertex_positions_tex( package + ".m_vertex_positions_tex" ),
      m_vertex_positions_vao( package + ".m_vertex_positions_vao" ),
      m_normals_num( 0 ),
      m_normal_vectors_buf( package + ".m_normal_vectors_buf" ),
      m_normal_vectors_tex( package + ".m_normal_vectors_tex" ),
      m_cells_num(0),
      m_cell_global_index_buf( package + ".m_cell_global_index_buf" ),
      m_cell_global_index_tex( package + ".m_cell_global_index_tex" ),
      m_cell_vertex_indices_buf( package + ".m_cell_vertex_indices_buf" ),
      m_cell_vertex_indices_tex( package + ".m_cell_vertex_indices_tex" )
{
    reset();
}

void
PolygonMeshGPUModel::reset()
{
    m_vertices_num = 0;
    m_normals_num = 0;
    m_cells_num = 0;
    m_bb_min[0] = 0.f;
    m_bb_min[1] = 0.f;
    m_bb_min[2] = 0.f;
    m_bb_max[0] = 1.f;
    m_bb_max[1] = 1.f;
    m_bb_max[2] = 1.f;
    m_scale[0] = 1.f;
    m_scale[1] = 1.f;
    m_scale[2] = 1.f;
    m_shift[0] = 0.f;
    m_shift[1] = 0.f;
    m_shift[2] = 0.f;
}

PolygonMeshGPUModel::~PolygonMeshGPUModel()
{
}
    
void
PolygonMeshGPUModel::update( shared_ptr<const PolygonMeshBridge> mesh_bridge )
{
    if( !updateVertices(mesh_bridge)
            || !updateBoundingBox(mesh_bridge)
            || !updateNormals(mesh_bridge)
            || !updateCells(mesh_bridge) )
    {
        Logger log = getLogger( package + ".update" );
        LOGGER_DEBUG( log, "Failed to import bridge." );
        reset();
    }
}

bool
PolygonMeshGPUModel::updateVertices( shared_ptr<const PolygonMeshBridge> mesh_bridge  )
{
    return true;
}

bool
PolygonMeshGPUModel::updateBoundingBox( shared_ptr<const PolygonMeshBridge> mesh_bridge  )
{
    return true;
}

bool
PolygonMeshGPUModel::updateNormals( shared_ptr<const PolygonMeshBridge> mesh_bridge  )
{
    return true;
}

bool
PolygonMeshGPUModel::updateCells( shared_ptr<const PolygonMeshBridge> mesh_bridge  )
{
    return true;
}

    
    } // of namespace mesh
} // of namespace render