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
    m_bb_valid = false;
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
            || !updateBoundingBox()
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
    Logger log = getLogger( package + ".updateVertices" );

    m_vertices_num = mesh_bridge->m_vertices.size();
    if( m_vertices_num < 1 ) {
        LOGGER_DEBUG( log, "No vertices." );
        return false;
    }
    
    // --- make host copy ------------------------------------------------------
    m_vertex_positions_host.resize( 4*m_vertices_num );
    for( size_t i=0; i<(size_t)m_vertices_num; i++ ) {
        m_vertex_positions_host[ 4*i + 0 ] = mesh_bridge->m_vertices[i].x();
        m_vertex_positions_host[ 4*i + 1 ] = mesh_bridge->m_vertices[i].y();
        m_vertex_positions_host[ 4*i + 2 ] = mesh_bridge->m_vertices[i].z();
        m_vertex_positions_host[ 4*i + 3 ] = 1.f;
    }
    
    // --- make GPU copy -------------------------------------------------------
    glBindBuffer( GL_ARRAY_BUFFER, m_vertex_positions_buf.get() );
    glBufferData( GL_ARRAY_BUFFER,
                  4*sizeof(float)*m_vertices_num,
                  m_vertex_positions_host.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    
    // --- set up vertex array -------------------------------------------------
    glBindVertexArray( m_vertex_positions_vao.get() );
    glBindBuffer( GL_ARRAY_BUFFER, m_vertex_positions_buf.get() );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 0, NULL );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );
    
    // --- set up buffer texture -----------------------------------------------
    glBindTexture( GL_TEXTURE_BUFFER, m_vertex_positions_tex.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_vertex_positions_buf.get() );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );

    return true;
}

bool
PolygonMeshGPUModel::updateBoundingBox( )
{
    Logger log = getLogger( package + ".updateBoundingBox" );
    if( m_vertices_num < 1 ) {
        m_bb_valid = false;
        LOGGER_DEBUG( log, "No vertices." );
        return false;
    }

    m_bb_min[0] = m_bb_max[0] = m_vertex_positions_host[0];
    m_bb_min[1] = m_bb_max[1] = m_vertex_positions_host[1];
    m_bb_min[2] = m_bb_max[2] = m_vertex_positions_host[2];
    for(unsigned int j=0; j<m_vertex_positions_host.size(); j+=4) {
        m_bb_min[0] = std::min( m_bb_min[0], m_vertex_positions_host[j+0] );
        m_bb_max[0] = std::max( m_bb_max[0], m_vertex_positions_host[j+0] );
        m_bb_min[1] = std::min( m_bb_min[1], m_vertex_positions_host[j+1] );
        m_bb_max[1] = std::max( m_bb_max[1], m_vertex_positions_host[j+1] );
        m_bb_min[2] = std::min( m_bb_min[2], m_vertex_positions_host[j+2] );
        m_bb_max[2] = std::max( m_bb_max[2], m_vertex_positions_host[j+2] );
    }
    for(unsigned int i=0; i<3; i++) {
        m_scale[i] = 1.f/(m_bb_max[i]-m_bb_min[i]);
    }
    float xy_scale = std::min( m_scale[0], m_scale[1] );
    m_scale[0] = xy_scale;
    m_scale[1] = xy_scale;

    for(unsigned int i=0; i<3; i++) {
        m_shift[i] = -0.5f*(m_bb_max[i]+m_bb_min[i]) + 0.5f/m_scale[i];
    }

    LOGGER_DEBUG( log, "bbox = ["
                  << m_bb_min[0] << ", " << m_bb_min[1] << ", " << m_bb_min[2] << "] x ["
                  << m_bb_max[0] << ", " << m_bb_max[1] << ", " << m_bb_max[2] << "]." );

    m_bb_valid = true;
    return true;
}

bool
PolygonMeshGPUModel::updateNormals( shared_ptr<const PolygonMeshBridge> mesh_bridge  )
{
    Logger log = getLogger( package + ".updateNormals" );
    
    m_normals_num =  mesh_bridge->m_normals.size();
    if( m_normals_num < 1 ) {
        LOGGER_DEBUG( log, "No normals." );
        return false;
    }
    
    // --- make host copy ------------------------------------------------------
    m_normal_vectors_host.resize( 4*m_normals_num );
    for(GLsizei i=0; i<m_normals_num; i++ ) {
        m_normal_vectors_host[ 4*i+0 ] = mesh_bridge->m_normals[i].x();
        m_normal_vectors_host[ 4*i+1 ] = mesh_bridge->m_normals[i].y();
        m_normal_vectors_host[ 4*i+2 ] = mesh_bridge->m_normals[i].z();
        m_normal_vectors_host[ 4*i+3 ] = mesh_bridge->m_normals[i].w();
    }
    
    // --- make GPU copy -------------------------------------------------------
    glBindBuffer( GL_TEXTURE_BUFFER, m_normal_vectors_buf.get() );
    glBufferData( GL_TEXTURE_BUFFER,
                  4*sizeof(float)*m_normals_num,
                  m_normal_vectors_host.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );

    // --- set up buffer texture -----------------------------------------------
    glBindTexture( GL_TEXTURE_BUFFER, m_normal_vectors_tex.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_normal_vectors_buf.get() );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    
    return true;
}

bool
PolygonMeshGPUModel::updateCells( shared_ptr<const PolygonMeshBridge> mesh_bridge  )
{
    Logger log = getLogger( package + ".updateNormals" );
    
    if( mesh_bridge->m_cell_offset.size() < 2 ) {
        m_cells_num = 0;
        LOGGER_DEBUG( log, "No cells." );
        return false;
    }
    m_cells_num = mesh_bridge->m_cell_offset.size()-1;

    // --- make host copy of local to global cell map, currently identiy -------
    m_cell_global_index_host.resize( m_cells_num );
    for(size_t i=0; i<m_cell_global_index_host.size(); i++ ) {
        m_cell_global_index_host[i] = i;
    }

    // make GPU copy
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_global_index_buf.get() );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GLuint)*m_cell_global_index_host.size(),
                  m_cell_global_index_host.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
    
    // set up texture
    glBindTexture( GL_TEXTURE_BUFFER, m_cell_global_index_tex.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_cell_global_index_buf.get() );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    
    
    // --- make host copy of vertex indices spanning each cell -----------------
    
    // currently assumed to be 8. If less, we duplicate vertices, if more, we
    // omit some (yes, incorrect result).
    m_cell_vertex_indices_host.resize( 8*m_cells_num );

    for(size_t c=0; c<(size_t)m_cells_num; c++ ) {
        size_t o = mesh_bridge->m_cell_offset[c];
        size_t n = mesh_bridge->m_cell_offset[c+1] - o;
        for( size_t i=0; i<8; i++ ) {
            m_cell_vertex_indices_host[ 8*c + i ] = mesh_bridge->m_cell_corners[ o + (i%n) ];
        }
    }
    
    // make GPU copy
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_vertex_indices_buf.get() );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GLuint)*m_cell_vertex_indices_host.size(),
                  m_cell_vertex_indices_host.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );

    // set up texture
    glBindTexture( GL_TEXTURE_BUFFER, m_cell_vertex_indices_tex.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32UI, m_cell_vertex_indices_buf.get() );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );    

    return true;
}

    
    } // of namespace mesh
} // of namespace render