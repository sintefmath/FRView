/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <cmath>
#include <stdlib.h>
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
    m_vertices.m_N = 0;
    m_triangles.m_index_count = 0;

    glGenVertexArrays( 1, &m_vertices.m_vao );
    glGenTextures( 1, &m_vertices.m_texture );
    glGenBuffers( 1, &m_vertices.m_vbo );
    glGenBuffers( 1, &m_tri_info.m_buffer );
    glGenTextures( 1, &m_tri_info.m_texture );
    glGenBuffers( 1, &m_triangles.m_ibo );
    glGenBuffers( 1, &m_cell_index.m_buffer );
    glGenTextures( 1, &m_cell_index.m_texture );
    glGenBuffers( 1, &m_cell_corner.m_buffer );
    glGenTextures( 1, &m_cell_corner.m_texture );

    glGenVertexArrays( 1, &m_triangle_vao );
}

GridTessBridge::GridTessBridge(GridTess &owner)
    : m_owner( owner )
{
}

GridTessBridge::~GridTessBridge()
{
    m_owner.import( *this );
}

void
GridTess::import( GridTessBridge& bridge )
{
    Logger log = getLogger( "GridTess.import" );

    if( bridge.m_vertices.empty() ) {
        return;
    }


    for(unsigned int i=0; i<3; i++) {
        m_bb_min[i] = m_bb_max[i] = bridge.m_vertices[i];
    }
    for(unsigned int j=0; j<bridge.m_vertices.size()/4; j++) {
        for(unsigned int i=0; i<3; i++) {
            m_bb_min[i] = std::min( m_bb_min[i], bridge.m_vertices[4*j+i] );
            m_bb_max[i] = std::max( m_bb_max[i], bridge.m_vertices[4*j+i] );
        }
    }

    LOGGER_DEBUG( log, "bbox = ["
                  << m_bb_min[0] << ", " << m_bb_min[1] << ", " << m_bb_min[2] << "] x ["
                  << m_bb_max[0] << ", " << m_bb_max[1] << ", " << m_bb_max[2] << "]." );

    for(unsigned int i=0; i<3; i++) {
        m_scale[i] = 1.f/(m_bb_max[i]-m_bb_min[i]);
    }
    float xy_scale = std::min( m_scale[0], m_scale[1] );
    m_scale[0] = xy_scale;
    m_scale[1] = xy_scale;

    for(unsigned int i=0; i<3; i++) {
        m_shift[i] = -0.5f*(m_bb_max[i]+m_bb_min[i]) + 0.5f/m_scale[i];
    }

/*
    for(unsigned int j=0; j<bridge.m_vertices.size()/4; j++) {
        for(unsigned int i=0; i<3; i++) {
            bridge.m_vertices[4*j+i] = m_scale[i]*(bridge.m_vertices[4*j+i]+m_shift[i]  );
        }
        bridge.m_vertices[4*j+3] = 1.f;
    }
*/

    m_vertices.m_N = bridge.m_vertices.size()/4;
    if( m_vertices.m_N > 0 ) {
        glBindVertexArray( m_vertices.m_vao );
        glBindBuffer( GL_ARRAY_BUFFER, m_vertices.m_vbo );
        glBufferData( GL_ARRAY_BUFFER,
                      sizeof(float)*bridge.m_vertices.size(),
                      bridge.m_vertices.data(),
                      GL_STATIC_DRAW );
        glVertexPointer( 4, GL_FLOAT, 0, NULL );
        glEnableClientState( GL_VERTEX_ARRAY );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindVertexArray( 0 );

        glBindTexture( GL_TEXTURE_BUFFER, m_vertices.m_texture );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_vertices.m_vbo );
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
    }


    m_triangles.m_index_count = bridge.m_triangles.size();
    if( m_triangles.m_index_count > 0 ) {
        glBindBuffer( GL_TEXTURE_BUFFER, m_tri_info.m_buffer );
        glBufferData( GL_TEXTURE_BUFFER,
                      sizeof(unsigned int)*bridge.m_triangle_info.size(),
                      bridge.m_triangle_info.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_TEXTURE_BUFFER, m_tri_info.m_buffer );
        glBindTexture( GL_TEXTURE_BUFFER, m_tri_info.m_texture );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RG32UI, m_tri_info.m_buffer );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_triangles.m_ibo );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                      sizeof(int)*bridge.m_triangles.size(),
                      bridge.m_triangles.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );


        glBindVertexArray( m_triangle_vao );
        glBindBuffer( GL_ARRAY_BUFFER, m_tri_info.m_buffer );
        glVertexAttribIPointer( 0, 2, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 0 );
        glBindBuffer( GL_ARRAY_BUFFER, m_triangles.m_ibo );
        glVertexAttribIPointer( 1, 3, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 1 );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindVertexArray( 0 );
    }

    // steal the contents of the cell_index buffer
    m_cell_index.m_body.swap( bridge.m_cell_index );

    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_index.m_buffer );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(unsigned int)*m_cell_index.m_body.size(),
                  m_cell_index.m_body.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_index.m_buffer );
    glBindTexture( GL_TEXTURE_BUFFER, m_cell_index.m_texture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_cell_index.m_buffer );


    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_corner.m_buffer );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(unsigned int)*bridge.m_cell_corner.size(),
                  bridge.m_cell_corner.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, m_cell_corner.m_buffer );
    glBindTexture( GL_TEXTURE_BUFFER, m_cell_corner.m_texture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32UI, m_cell_corner.m_buffer );


    glBindTexture( GL_TEXTURE_BUFFER, 0 );


    CHECK_GL;
}
