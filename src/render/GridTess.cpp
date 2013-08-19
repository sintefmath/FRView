/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <stdlib.h>
#include <algorithm>
#include <tuple>
#include "CellSelector.hpp"
#include "GridTess.hpp"
#include "GridField.hpp"
#include "GridTessBridge.hpp"
#include "GridTessSubset.hpp"
#include "utils/Logger.hpp"
#include <siut2/gl_utils/GLSLtools.hpp>
#include <siut2/io_utils/snarf.hpp>

namespace render {

GridTess::GridTess()
    : m_vertices_num( 0 ),
      m_vertex_positions_buf( "GridTess.m_vertex_positions_buf" ),
      m_vertex_positions_tex( "GridTess.m_vertex_positions_tex" ),
      m_vertex_positions_vao( "GridTess.m_vertex_positions_vao" ),
      m_normals_num( 0 ),
      m_normal_vectors_buf( "GridTess.m_normal_vectors_buf" ),
      m_normal_vectors_tex( "GridTess.m_normal_vectors_tex" ),
      m_cells_num(0),
      m_cell_global_index_buf( "GridTess.m_cell_global_index_buf" ),
      m_cell_global_index_tex( "GridTess.m_cell_global_index_tex" ),
      m_cell_vertex_indices_buf( "GridTess.m_cell_vertex_indices_buf" ),
      m_cell_vertex_indices_tex( "GridTess.m_cell_vertex_indices_tex" ),
      m_triangles_N(0),
      m_polygons_N( 0 ),
      m_polygon_vao( "GridTess.m_polygon_vao" ),
      m_polygon_info_buf( "GridTess.m_polygon_info_buf" ),
      m_polygon_offset_buf( "GridTess.m_polygon_offset_buf" ),
      m_polygon_vtx_buf( "GridTess.m_polygon_vtx_buf" ),
      m_polygon_vtx_tex( "GridTess.m_polygon_vtx_tex" ),
      m_polygon_nrm_buf( "GridTess.m_polygon_nrm_buf" ),
      m_polygon_nrm_tex( "GridTess.m_polygon_nrm_tex" )
{
    Logger log = getLogger( "GridTess.constructor" );
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

GridTess::~GridTess()
{
}

void
GridTess::update( GridTessBridge& bridge )
{
    Logger log = getLogger( "GridTess.import" );

    if( bridge.m_vertices.empty() ) {
        return;
    }
    updateVertices( bridge );
    updateNormals( bridge );
    updateBoundingBox( bridge );
    updateCells( bridge );
    updatePolygons( bridge );
}

void
GridTess::updatePolygons( GridTessBridge& bridge )
{
    Logger log = getLogger( "GridTess.updatePolygons" );
    m_polygons_N = bridge.m_polygon_info.size()/2;

    m_polygon_max_n = 0;
    m_triangles_N = 0;
    uint faults = 0;
    for(size_t i=0; i+1<bridge.m_polygon_offset.size(); i++ ) {
        GLsizei N = (GLsizei)(bridge.m_polygon_offset[i+1]-bridge.m_polygon_offset[i]);
        m_triangles_N += (N-2);
        m_polygon_max_n = std::max( m_polygon_max_n, N );
        if( (bridge.m_polygon_info[2*i]&0x80000000) != 0 ) {
            faults++;
        }

    }
    LOGGER_DEBUG( log, "Max polygon size: " << m_polygon_max_n );
    LOGGER_DEBUG( log, "Number of fault polygons: " << faults );


    glBindVertexArray( m_polygon_vao.get() );

    glBindBuffer( GL_ARRAY_BUFFER, m_polygon_info_buf.get() );
    glBufferData( GL_ARRAY_BUFFER,
                  sizeof(GLuint)*bridge.m_polygon_info.size(),
                  bridge.m_polygon_info.data(),
                  GL_STATIC_DRAW );
    glVertexAttribIPointer( 0, 2, GL_UNSIGNED_INT, 0, NULL );
    glEnableVertexAttribArray( 0 );

    glBindBuffer( GL_ARRAY_BUFFER, m_polygon_offset_buf.get() );
    glBufferData( GL_ARRAY_BUFFER,
                  sizeof(GLuint)*bridge.m_polygon_offset.size(),
                  bridge.m_polygon_offset.data(),
                  GL_STATIC_DRAW );
    glVertexAttribIPointer( 1, 1, GL_UNSIGNED_INT, 1*sizeof(GLuint), NULL );
    glEnableVertexAttribArray( 1 );
    glVertexAttribIPointer( 2, 1, GL_UNSIGNED_INT, 1*sizeof(GLuint), reinterpret_cast<const GLvoid*>( sizeof(GLuint) ) );
    glEnableVertexAttribArray( 2 );

    glBindVertexArray( 0 );

    glBindBuffer( GL_TEXTURE_BUFFER, m_polygon_vtx_buf.get() );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GLuint)*bridge.m_polygon_vtx_ix.size(),
                  bridge.m_polygon_vtx_ix.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );

    glBindTexture( GL_TEXTURE_BUFFER, m_polygon_vtx_tex.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_polygon_vtx_buf.get() );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );


    glBindBuffer( GL_TEXTURE_BUFFER, m_polygon_nrm_buf.get() );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GLuint)*bridge.m_polygon_nrm_ix.size(),
                  bridge.m_polygon_nrm_ix.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );

    glBindTexture( GL_TEXTURE_BUFFER, m_polygon_nrm_tex.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_polygon_nrm_buf.get() );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
}


void
GridTess::checkTopology() const
{
#if 0
    Logger log = getLogger( "GridTess.checkTopolgy" );

    std::vector< std::vector< unsigned int > > cell_faces( cellCount() );

    LOGGER_DEBUG( log, "Extracting faces for each cell..." );
    for( size_t i=0; i< triangleCount(); i++ ) {
        for(size_t k=0; k<2; k++ ) {
            if( m_triangle_cell_indices_host[2*i+k] != ~0u ) {
                if( m_triangle_cell_indices_host[2*i+k] < cellCount() ) {
                    cell_faces[ m_triangle_cell_indices_host[2*i+k] ].push_back( i );
                }
                else {
                    LOGGER_ERROR( log, "Triangle " << i << " at side " << k << " has illegal cell index " << m_triangle_cell_indices_host[2*i+k] );
                }
            }
        }
    }
    LOGGER_DEBUG( log, "Inspecting each cell individually." );
    for( size_t c=0; c<cellCount(); c++ ) {
        std::vector<unsigned int>& triangles = cell_faces[c];
        std::vector< std::tuple<unsigned int,unsigned int> > half_edges;
        for(size_t k=0; k<triangles.size(); k++ ) {
            unsigned int t = triangles[k];
            for(size_t i=0; i<3; i++ ) {
                unsigned int a = m_triangle_vertex_indices_host[ 3*t + i ];
                unsigned int b = m_triangle_vertex_indices_host[ 3*t + ((i+1)%3) ];
                if( a == b ) {
                    LOGGER_ERROR( log, "Illegal edge encounteded in cell " << c );
                }
                else if( a >= vertexCount() ) {
                    LOGGER_ERROR( log, "Illegal vertex index " << a << " in cell " << c );
                }
                else if( b >= vertexCount() ) {
                    LOGGER_ERROR( log, "Illegal vertex index " << b << " in cell " << c );
                }
                else {
                    half_edges.push_back( std::make_tuple( std::min( a,b ), std::max( a, b) ) );
                }
                if( (half_edges.size() % 2) != 0 ) {
                    LOGGER_ERROR( log, "Odd number of half-edges for cell " << c << ", they can't all be regular!" );
                }
                else {
                    // sort half-edges lexicographically
                    std::sort( half_edges.begin(),
                               half_edges.end(),
                               []( std::tuple<unsigned int, unsigned int> a, std::tuple<unsigned int, unsigned int> b )
                    {
                        if( std::get<0>(a) < std::get<0>( b ) ) {
                            return true;
                        }
                        else if( std::get<0>(a) > std::get<0>(b) ) {
                            return false;
                        }
                        else {
                            return std::get<1>(a) < std::get<1>( b );
                        }

                    } );
                    // Check that all two pairs up
                    for(size_t k=0; k<half_edges.size()/2; k++ ) {

                    }
                }
            }
        }


    }

#endif
}

void
GridTess::updateBoundingBox( GridTessBridge& bridge )
{
    Logger log = getLogger( "GridTess.setCornerPointBoundingBox" );

    m_bb_min[0] = m_bb_max[0] = bridge.m_vertices[0].x();
    m_bb_min[1] = m_bb_max[1] = bridge.m_vertices[1].y();
    m_bb_min[2] = m_bb_max[2] = bridge.m_vertices[2].z();
    for(unsigned int j=0; j<bridge.m_vertices.size(); j++) {
        m_bb_min[0] = std::min( m_bb_min[0], bridge.m_vertices[j].x() );
        m_bb_max[0] = std::max( m_bb_max[0], bridge.m_vertices[j].x() );
        m_bb_min[1] = std::min( m_bb_min[1], bridge.m_vertices[j].y() );
        m_bb_max[1] = std::max( m_bb_max[1], bridge.m_vertices[j].y() );
        m_bb_min[2] = std::min( m_bb_min[2], bridge.m_vertices[j].z() );
        m_bb_max[2] = std::max( m_bb_max[2], bridge.m_vertices[j].z() );
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
}

void
GridTess::updateVertices( GridTessBridge& bridge )
{
    m_vertices_num = bridge.m_vertices.size();
    if( m_vertices_num > 0 ) {
        // host data
        m_vertex_positions_host.resize( 4*m_vertices_num );
        for( GLsizei i=0; i<m_vertices_num; i++ ) {
            m_vertex_positions_host[ 4*i+0 ] = bridge.m_vertices[i].x();
            m_vertex_positions_host[ 4*i+1 ] = bridge.m_vertices[i].y();
            m_vertex_positions_host[ 4*i+2 ] = bridge.m_vertices[i].z();
            m_vertex_positions_host[ 4*i+3 ] = 1.f;
        }
        // buffer object
        glBindBuffer( GL_ARRAY_BUFFER, m_vertex_positions_buf.get() );
        glBufferData( GL_ARRAY_BUFFER,
                      4*sizeof(float)*m_vertices_num,
                      m_vertex_positions_host.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        // vertex array object
        glBindVertexArray( m_vertex_positions_vao.get() );
        glBindBuffer( GL_ARRAY_BUFFER, m_vertex_positions_buf.get() );
        glVertexPointer( 4, GL_FLOAT, 0, NULL );
        glEnableClientState( GL_VERTEX_ARRAY );
        glBindVertexArray( 0 );
        // texture
        glBindTexture( GL_TEXTURE_BUFFER, m_vertex_positions_tex.get() );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_vertex_positions_buf.get() );
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
    }

}

void
GridTess::updateNormals( GridTessBridge& bridge )
{
    // Normal vectors
    m_normals_num = bridge.m_normals.size();
    if( m_normals_num > 0 ) {
        m_normal_vectors_host.resize( 4*m_normals_num );
        for(GLsizei i=0; i<m_normals_num; i++ ) {
            m_normal_vectors_host[ 4*i+0 ] = bridge.m_normals[i].x();
            m_normal_vectors_host[ 4*i+1 ] = bridge.m_normals[i].y();
            m_normal_vectors_host[ 4*i+2 ] = bridge.m_normals[i].z();
            m_normal_vectors_host[ 4*i+3 ] = bridge.m_normals[i].w();
        }
        // buffer
        glBindBuffer( GL_TEXTURE_BUFFER, m_normal_vectors_buf.get() );
        glBufferData( GL_TEXTURE_BUFFER,
                      4*sizeof(float)*m_normals_num,
                      m_normal_vectors_host.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_TEXTURE_BUFFER, 0 );
        // texture
        glBindTexture( GL_TEXTURE_BUFFER, m_normal_vectors_tex.get() );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_normal_vectors_buf.get() );
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
    }


}

void
GridTess::updateCells( GridTessBridge& bridge )
{
    m_cells_num = bridge.m_cell_index.size();

    m_cell_global_index_host.resize( m_cells_num );
    for(GLsizei i=0; i<m_cells_num; i++ ) {
        m_cell_global_index_host[i] = bridge.m_cell_index[i];
    }
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_global_index_buf.get() );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GLuint)*m_cell_global_index_host.size(),
                  m_cell_global_index_host.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_global_index_buf.get() );
    glBindTexture( GL_TEXTURE_BUFFER, m_cell_global_index_tex.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_cell_global_index_buf.get() );

    m_cell_vertex_indices_host.resize( 8*m_cells_num );
    for(GLsizei i=0; i<8*m_cells_num; i++ ) {
        m_cell_vertex_indices_host[i] = bridge.m_cell_corner[i];
    }
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_vertex_indices_buf.get() );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GLuint)*m_cell_vertex_indices_host.size(),
                  m_cell_vertex_indices_host.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_vertex_indices_buf.get() );
    glBindTexture( GL_TEXTURE_BUFFER, m_cell_vertex_indices_tex.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32UI, m_cell_vertex_indices_buf.get() );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
}

} // of namespace render
