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
#include "Logger.hpp"
#include <siut2/gl_utils/GLSLtools.hpp>
#include <siut2/io_utils/snarf.hpp>

GridTess::GridTess()
{
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

    m_vertices_num = 0;
    glGenBuffers( 1, &m_vertex_positions_buf );
    glGenTextures( 1, &m_vertex_positions_tex );
    glGenVertexArrays( 1, &m_vertex_positions_vao );

    m_triangles_num = 0;

    glGenBuffers( 1, &m_triangle_cell_indices_buf );
    glGenTextures( 1, &m_triangle_cell_indices_tex );
    glGenBuffers( 1, &m_triangle_vertex_indices_buf );
    glGenBuffers( 1, &m_cell_global_index_buf );
    glGenTextures( 1, &m_cell_global_index_tex );
    glGenBuffers( 1, &m_cell_vertex_indices_buf );
    glGenTextures( 1, &m_cell_vertex_indices_tex );


    glGenBuffers( 1, &m_triangle_normal_indices_buf );
    glGenTextures( 1, &m_triangle_normal_indices_tex );

    glGenBuffers( 1, &m_normal_vectors_buf );
    glGenTextures( 1, &m_normal_vectors_tex );

    glGenVertexArrays( 1, &m_triangle_index_tuple_vao );

    glGenBuffers( 1, &m_edge_vertex_indices_buf );
    glGenBuffers( 1, &m_edge_cell_indices_buf );
    glGenVertexArrays( 1, &m_edge_index_tuple_vao );

    glGenVertexArrays( 1, &m_polygon_vao );
    glGenBuffers( 1, &m_polygon_info_buf );
    glGenBuffers( 1, &m_polygon_offset_buf );
    glGenBuffers( 1, &m_polygon_vtx_buf );
    glGenTextures( 1, &m_polygon_vtx_tex );
    glGenBuffers( 1, &m_polygon_nrm_buf );
    glGenTextures( 1, &m_polygon_nrm_tex );

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
    updateTriangles( bridge );
#ifdef GPU_TRIANGULATOR
    updatePolygons( bridge );
#endif
    updateEdges( bridge );
    CHECK_GL;
}

void
GridTess::updatePolygons( GridTessBridge& bridge )
{

#ifdef GPU_TRIANGULATOR
    Logger log = getLogger( "GridTess.updatePolygons" );
    m_polygons_N = bridge.m_polygon_info.size()/2;

    m_polygon_max_n = 0;
    for(size_t i=0; i+1<bridge.m_polygon_offset.size(); i++ ) {
        m_polygon_max_n = std::max( m_polygon_max_n,
                                    (GLsizei)(bridge.m_polygon_offset[i+1]-bridge.m_polygon_offset[i]) );
    }
    LOGGER_DEBUG( log, "Max polygon size: " << m_polygon_max_n );

    glBindVertexArray( m_polygon_vao );

    glBindBuffer( GL_ARRAY_BUFFER, m_polygon_info_buf );
    glBufferData( GL_ARRAY_BUFFER,
                  sizeof(GLuint)*bridge.m_polygon_info.size(),
                  bridge.m_polygon_info.data(),
                  GL_STATIC_DRAW );
    glVertexAttribIPointer( 0, 2, GL_UNSIGNED_INT, 0, NULL );
    glEnableVertexAttribArray( 0 );

    glBindBuffer( GL_ARRAY_BUFFER, m_polygon_offset_buf );
    glBufferData( GL_ARRAY_BUFFER,
                  sizeof(GLuint)*bridge.m_polygon_offset.size(),
                  bridge.m_polygon_offset.data(),
                  GL_STATIC_DRAW );
    glVertexAttribIPointer( 1, 1, GL_UNSIGNED_INT, 1*sizeof(GLuint), NULL );
    glEnableVertexAttribArray( 1 );
    glVertexAttribIPointer( 2, 1, GL_UNSIGNED_INT, 1*sizeof(GLuint), reinterpret_cast<const GLvoid*>( sizeof(GLuint) ) );
    glEnableVertexAttribArray( 2 );

    glBindVertexArray( 0 );

    glBindBuffer( GL_TEXTURE_BUFFER, m_polygon_vtx_buf );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GLuint)*bridge.m_polygon_vtx_ix.size(),
                  bridge.m_polygon_vtx_ix.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );

    glBindTexture( GL_TEXTURE_BUFFER, m_polygon_vtx_tex );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_polygon_vtx_buf );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );


    glBindBuffer( GL_TEXTURE_BUFFER, m_polygon_nrm_buf );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GLuint)*bridge.m_polygon_nrm_ix.size(),
                  bridge.m_polygon_nrm_ix.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );

    glBindTexture( GL_TEXTURE_BUFFER, m_polygon_nrm_tex );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_polygon_nrm_buf );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );

#endif
}


void
GridTess::checkTopology() const
{
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
        glBindBuffer( GL_ARRAY_BUFFER, m_vertex_positions_buf );
        glBufferData( GL_ARRAY_BUFFER,
                      4*sizeof(float)*m_vertices_num,
                      m_vertex_positions_host.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        // vertex array object
        glBindVertexArray( m_vertex_positions_vao );
        glBindBuffer( GL_ARRAY_BUFFER, m_vertex_positions_buf );
        glVertexPointer( 4, GL_FLOAT, 0, NULL );
        glEnableClientState( GL_VERTEX_ARRAY );
        glBindVertexArray( 0 );
        // texture
        glBindTexture( GL_TEXTURE_BUFFER, m_vertex_positions_tex );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_vertex_positions_buf );
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
        for(unsigned int i=0; i<m_normals_num; i++ ) {
            m_normal_vectors_host[ 4*i+0 ] = bridge.m_normals[i].x();
            m_normal_vectors_host[ 4*i+1 ] = bridge.m_normals[i].y();
            m_normal_vectors_host[ 4*i+2 ] = bridge.m_normals[i].z();
            m_normal_vectors_host[ 4*i+3 ] = bridge.m_normals[i].w();
        }
        // buffer
        glBindBuffer( GL_TEXTURE_BUFFER, m_normal_vectors_buf );
        glBufferData( GL_TEXTURE_BUFFER,
                      4*sizeof(float)*m_normals_num,
                      m_normal_vectors_host.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_TEXTURE_BUFFER, 0 );
        // texture
        glBindTexture( GL_TEXTURE_BUFFER, m_normal_vectors_tex );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_normal_vectors_buf );
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
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_global_index_buf );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GLuint)*m_cell_global_index_host.size(),
                  m_cell_global_index_host.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_global_index_buf );
    glBindTexture( GL_TEXTURE_BUFFER, m_cell_global_index_tex );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_cell_global_index_buf );

    m_cell_vertex_indices_host.resize( 8*m_cells_num );
    for(GLsizei i=0; i<8*m_cells_num; i++ ) {
        m_cell_vertex_indices_host[i] = bridge.m_cell_corner[i];
    }
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_vertex_indices_buf );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GLuint)*m_cell_vertex_indices_host.size(),
                  m_cell_vertex_indices_host.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_vertex_indices_buf );
    glBindTexture( GL_TEXTURE_BUFFER, m_cell_vertex_indices_tex );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32UI, m_cell_vertex_indices_buf );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
}

void
GridTess::updateTriangles( GridTessBridge& bridge )
{
    Logger log = getLogger( "GridTess.updateTriangles" );

    m_triangles_num = bridge.m_tri_N;

    // triangle normal vector indices
    if( m_triangles_num > 0 ) {
        glBindBuffer( GL_TEXTURE_BUFFER, m_triangle_normal_indices_buf );
        glBufferData( GL_TEXTURE_BUFFER,
                      3*sizeof(GLuint)*m_triangles_num,
                      NULL,
                      GL_STATIC_DRAW );
        GLsizei o = 0;
        for( auto it=bridge.m_tri_nrm_ix_chunks.begin(); it!=bridge.m_tri_nrm_ix_chunks.end(); ++it ) {
            glBufferSubData( GL_TEXTURE_BUFFER,
                             3*sizeof(GLuint)*o,
                             3*sizeof(GLuint)*std::min( bridge.m_chunk_size, bridge.m_tri_N-o ),
                             *it );
            o += bridge.m_chunk_size;
        }

        m_triangle_vertex_indices_host.resize( 3*m_triangles_num );
        glGetBufferSubData( GL_TEXTURE_BUFFER,
                            NULL,
                            3*sizeof(GLuint)*m_triangles_num,
                            m_triangle_vertex_indices_host.data() );
        glBindBuffer( GL_TEXTURE_BUFFER, 0 );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_triangle_vertex_indices_buf );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                      3*sizeof(GLuint)*bridge.m_tri_N,
                      NULL,
                      GL_STATIC_DRAW );
        o = 0;
        for(auto it=bridge.m_tri_vtx_chunks.begin(); it!=bridge.m_tri_vtx_chunks.end(); ++it ) {
            glBufferSubData( GL_ELEMENT_ARRAY_BUFFER,
                             3*sizeof(GLuint)*o,
                             3*sizeof(GLuint)*std::min( bridge.m_chunk_size, bridge.m_tri_N-o ),
                             *it );
            o += bridge.m_chunk_size;
        }

        m_triangle_vertex_indices_host.resize( 3*m_triangles_num );
        glGetBufferSubData( GL_ELEMENT_ARRAY_BUFFER,
                            NULL,
                            3*sizeof(GLuint)*bridge.m_tri_N,
                            m_triangle_vertex_indices_host.data() );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

        glBindBuffer( GL_TEXTURE_BUFFER, m_triangle_cell_indices_buf );
        glBufferData( GL_TEXTURE_BUFFER,
                      2*sizeof(unsigned int)*bridge.m_tri_N,
                      NULL,
                      GL_STATIC_DRAW );
        o = 0;
        for(auto it=bridge.m_tri_info_chunks.begin(); it!=bridge.m_tri_info_chunks.end(); ++it ) {
            glBufferSubData( GL_TEXTURE_BUFFER,
                             2*sizeof(GLuint)*o,
                             2*sizeof(GLuint)*std::min( bridge.m_chunk_size, bridge.m_tri_N-o ),
                             *it );
            o += bridge.m_chunk_size;
        }

        // retrieve buffer to CPU for debugging
        m_triangle_cell_indices_host.resize( 2*bridge.m_tri_N );
        glGetBufferSubData( GL_TEXTURE_BUFFER,
                            NULL,
                            2*sizeof(GLuint)*bridge.m_tri_N,
                            m_triangle_cell_indices_host.data() );


        glBindBuffer( GL_TEXTURE_BUFFER, m_triangle_cell_indices_buf );
        glBindTexture( GL_TEXTURE_BUFFER, m_triangle_cell_indices_tex );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RG32UI, m_triangle_cell_indices_buf );

        // Set up of triangle index VAO
        glBindVertexArray( m_triangle_index_tuple_vao );
        glBindBuffer( GL_ARRAY_BUFFER, m_triangle_cell_indices_buf );
        glVertexAttribIPointer( 0, 2, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 0 );
        glBindBuffer( GL_ARRAY_BUFFER, m_triangle_vertex_indices_buf );
        glVertexAttribIPointer( 1, 3, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 1 );
        glBindBuffer( GL_ARRAY_BUFFER, m_triangle_normal_indices_buf );
        glVertexAttribIPointer( 2, 3, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 2 );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindVertexArray( 0 );
    }
    LOGGER_DEBUG( log, "Uploaded " << bridge.m_tri_vtx_chunks.size() << " chunks." );

}


void
GridTess::updateEdges( GridTessBridge& bridge )
{
    m_edges_num = bridge.m_edges.size();
    if( m_edges_num == 0 ) {
        m_edge_vertex_indices_buf = 0;
        m_edge_cell_indices_buf = 0;
        m_edge_cell_indices_tex = 0;
    }
    else {
        // set up index VAO
        glBindVertexArray( m_edge_index_tuple_vao );

        glBindBuffer( GL_ARRAY_BUFFER, m_edge_vertex_indices_buf );
        glBufferData( GL_ARRAY_BUFFER,
                      2*sizeof(GLuint)*m_edges_num,
                      NULL,
                      GL_STATIC_DRAW );
        GLuint* cp_ibo = static_cast<GLuint*>( glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY ) );
        for(size_t i=0; i<m_edges_num; i++ ) {
            cp_ibo[ 2*i+0 ] = bridge.m_edges[i].m_cp[0];
            cp_ibo[ 2*i+1 ] = bridge.m_edges[i].m_cp[1];
        }
        glUnmapBuffer( GL_ARRAY_BUFFER );
        glVertexAttribIPointer( 0, 2, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 0 );

        glBindBuffer( GL_ARRAY_BUFFER, m_edge_cell_indices_buf );
        glBufferData( GL_ARRAY_BUFFER,
                      4*sizeof(GLuint)*m_edges_num,
                      NULL,
                      GL_STATIC_DRAW );
        GLuint* cells_ibo = static_cast<GLuint*>( glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY ) );
        for(size_t i=0; i<m_edges_num; i++ ) {
            cells_ibo[ 4*i+0 ] = bridge.m_edges[i].m_cells[0];
            cells_ibo[ 4*i+1 ] = bridge.m_edges[i].m_cells[1];
            cells_ibo[ 4*i+2 ] = bridge.m_edges[i].m_cells[2];
            cells_ibo[ 4*i+3 ] = bridge.m_edges[i].m_cells[3];
        }
        glUnmapBuffer( GL_ARRAY_BUFFER );
        glVertexAttribIPointer( 1, 4, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 1 );

        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindVertexArray( 0 );
    }
    CHECK_GL;
}

