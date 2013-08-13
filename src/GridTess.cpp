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


    glGenBuffers( 1, &m_tri_nrm_ix.m_buf );
    glGenTextures( 1, &m_tri_nrm_ix.m_tex );

    glGenBuffers( 1, &m_normals.m_buf );
    glGenTextures( 1, &m_normals.m_tex );

    glGenVertexArrays( 1, &m_triangle_vao );
}

void
GridTess::exportTikZ( const std::string& filename,
                      const glm::mat4& transformation )
{
    Logger log = getLogger( "GridTess.exportTikZ");
    LOGGER_DEBUG( log, "exporting to '" << filename << "'." );

    std::ofstream o( filename.c_str() );
    if( !o.is_open() ) {
        LOGGER_ERROR( log, "Failed to open output file." );
        return;
    }

    std::vector<glm::vec3> V;
    for(size_t i=0; i<m_vertices.m_N; i++ ) {
        glm::vec4 p = transformation*glm::make_vec4( m_vertices.m_body.data() + 4*i );
        V.push_back( glm::vec3( p.x/p.w, p.y/p.w, p.z/p.w ) );
    }

    glm::mat4 nm = glm::transpose( glm::inverse( transformation ) );

    std::vector<float> shading;

        std::vector< std::tuple<float, int> > sorting;
    for(size_t i=0; i<m_triangles.m_index_count/3; i++ ) {
        LOGGER_DEBUG( log, m_tri_info.m_body[2*i+0] << " .. " <<  m_tri_info.m_body[2*i+1] );
        bool flip = true;
        if( m_tri_info.m_body[2*i+0] == ~0u ) {
        }
        else if (m_tri_info.m_body[2*i+1] == ~0u)  {
            flip = false;
        }
        else {
            continue;
        }

        float z = V[ m_triangles.m_body[3*i+0] ].z
                + V[ m_triangles.m_body[3*i+1] ].z
                + V[ m_triangles.m_body[3*i+2] ].z;
        sorting.push_back( std::tuple<float,int>( z, i ) );

        glm::vec4 n = glm::normalize( nm * (glm::make_vec4( m_normals.m_body.data() + 4*m_tri_nrm_ix.m_body[i+0] ) +
                                            glm::make_vec4( m_normals.m_body.data() + 4*m_tri_nrm_ix.m_body[i+1] ) +
                                            glm::make_vec4( m_normals.m_body.data() + 4*m_tri_nrm_ix.m_body[i+2] ) ) );
        float nz = n.z;
        if( nz < 0.f ) nz = 0.f;
        if( nz > 1.f ) nz = 1.f;
        shading.push_back( nz );
    }

    std::sort( sorting.begin(), sorting.end(), []( const std::tuple<float,int>& a,
                                                   const std::tuple<float,int>& b )
    {
               return std::get<0>(a) > std::get<0>(b);
    } );




    LOGGER_DEBUG( log, "m_vertices.m_N=" << m_vertices.m_N );
    o << "\\begin{tikzpicture}\n";
    for(size_t i=0; i<m_vertices.m_N; i++ ) {
        o << "  \\coordinate (c" << i << ") at ("
          << std::setiosflags(std::ios::fixed) << std::setprecision(6)
          << 5*V[i].x << ", "
          << 5*V[i].y << ");\n";
    }
    for(size_t i=0; i<sorting.size(); i++ ) {
        int t = std::get<1>( sorting[i] );
        int s = 25 + 50*shading[ i ];

        unsigned int info = m_tri_info.m_body[2*t+0];
        if( info == ~0u ) info = m_tri_info.m_body[2*t+1];


        o << "  \\fill[blue!"<<s<<"!white] "
          << "(c" << m_triangles.m_body[3*t+0 ]
          << ") -- (c" << m_triangles.m_body[3*t+1 ]
          << ") -- (c" << m_triangles.m_body[3*t+2 ]
          << ") -- cycle;\n";

        if( (info & 0x40000000u) != 0u ) {
            o << "  \\draw "
              << "(c" << m_triangles.m_body[3*t+0 ]
              << ") -- (c" << m_triangles.m_body[3*t+1 ]
              << ");\n";
        }
        if( (info & 0x20000000u) != 0u ) {
            o << "  \\draw "
              << "(c" << m_triangles.m_body[3*t+1 ]
              << ") -- (c" << m_triangles.m_body[3*t+2 ]
              << ");\n";
        }
        if( (info & 0x10000000u) != 0u ) {
            o << "  \\draw "
              << "(c" << m_triangles.m_body[3*t+2 ]
              << ") -- (c" << m_triangles.m_body[3*t+0 ]
              << ");\n";
        }


        //edge_a = m_tri_info.m_body[2*i+1] & (1<<31) != 0;
        //edge_b = m_tri_info.m_body[2*i+1] & (1<<30) != 0;
        //edge_c = m_tri_info.m_body[2*i+1] & (1<<29) != 0;


    }
    o << "\\end{tikzpicture}\n";

    o.close();
}

void
GridTess::import( GridTessBridge& bridge )
{
    Logger log = getLogger( "GridTess.import" );

    if( bridge.m_vertices.empty() ) {
        return;
    }


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



    // Normal vectors
    m_normals.m_N = bridge.m_normals.size();
    if( m_normals.m_N > 0 ) {
        glBindBuffer( GL_TEXTURE_BUFFER, m_normals.m_buf );
        glBufferData( GL_TEXTURE_BUFFER,
                      4*sizeof(float)*m_normals.m_N,
                      bridge.m_normals.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_TEXTURE_BUFFER, 0 );
        glBindTexture( GL_TEXTURE_BUFFER, m_normals.m_tex );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_normals.m_buf );
        glBindTexture( GL_TEXTURE_BUFFER, 0 );

        m_normals.m_body.resize( 4*m_normals.m_N );
        for(unsigned int i=0; i<m_normals.m_N; i++ ) {
            m_normals.m_body[ 4*i+0 ] = bridge.m_normals[i].x();
            m_normals.m_body[ 4*i+1 ] = bridge.m_normals[i].y();
            m_normals.m_body[ 4*i+2 ] = bridge.m_normals[i].z();
            m_normals.m_body[ 4*i+3 ] = bridge.m_normals[i].w();
        }
    }

    // Vertex positions
    m_vertices.m_N = bridge.m_vertices.size();
    if( m_vertices.m_N > 0 ) {
        glBindBuffer( GL_ARRAY_BUFFER, m_vertices.m_vbo );
        glBufferData( GL_ARRAY_BUFFER,
                      4*sizeof(float)*m_vertices.m_N,
                      bridge.m_vertices.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );

        glBindTexture( GL_TEXTURE_BUFFER, m_vertices.m_texture );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_vertices.m_vbo );
        glBindTexture( GL_TEXTURE_BUFFER, 0 );

        m_vertices.m_body.resize( 4*m_vertices.m_N );
        for(unsigned int i=0; i<m_vertices.m_N; i++ ) {
            m_vertices.m_body[ 4*i+0 ] = bridge.m_vertices[i].x();
            m_vertices.m_body[ 4*i+1 ] = bridge.m_vertices[i].y();
            m_vertices.m_body[ 4*i+2 ] = bridge.m_vertices[i].z();
            m_vertices.m_body[ 4*i+3 ] = 1.f;
        }
    }


    // triangle normal vector indices
    m_triangles.m_index_count = 3*bridge.m_tri_N;
    if( m_triangles.m_index_count > 0 ) {
        glBindBuffer( GL_TEXTURE_BUFFER, m_tri_nrm_ix.m_buf );
        glBufferData( GL_TEXTURE_BUFFER,
                      3*sizeof(GLuint)*bridge.m_tri_N,
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

        m_tri_nrm_ix.m_body.resize( m_triangles.m_index_count );
        glGetBufferSubData( GL_TEXTURE_BUFFER,
                            NULL,
                            3*sizeof(GLuint)*bridge.m_tri_N,
                            m_tri_nrm_ix.m_body.data() );
        glBindBuffer( GL_TEXTURE_BUFFER, 0 );
    }

    // triangle vertex indices
    if( m_triangles.m_index_count > 0 ) {
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_triangles.m_ibo );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                      3*sizeof(GLuint)*bridge.m_tri_N,
                      NULL,
                      GL_STATIC_DRAW );
        GLsizei o = 0;
        for(auto it=bridge.m_tri_vtx_chunks.begin(); it!=bridge.m_tri_vtx_chunks.end(); ++it ) {
            glBufferSubData( GL_ELEMENT_ARRAY_BUFFER,
                             3*sizeof(GLuint)*o,
                             3*sizeof(GLuint)*std::min( bridge.m_chunk_size, bridge.m_tri_N-o ),
                             *it );
            o += bridge.m_chunk_size;
        }

        m_triangles.m_body.resize( m_triangles.m_index_count );
        glGetBufferSubData( GL_ELEMENT_ARRAY_BUFFER,
                            NULL,
                            3*sizeof(GLuint)*bridge.m_tri_N,
                            m_triangles.m_body.data() );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    }

    // triangle cell indices
    if( m_triangles.m_index_count > 0 ) {
        glBindBuffer( GL_TEXTURE_BUFFER, m_tri_info.m_buffer );
        glBufferData( GL_TEXTURE_BUFFER,
                      2*sizeof(unsigned int)*bridge.m_tri_N,
                      NULL,
                      GL_STATIC_DRAW );
        GLsizei o = 0;
        for(auto it=bridge.m_tri_info_chunks.begin(); it!=bridge.m_tri_info_chunks.end(); ++it ) {
            glBufferSubData( GL_TEXTURE_BUFFER,
                             2*sizeof(GLuint)*o,
                             2*sizeof(GLuint)*std::min( bridge.m_chunk_size, bridge.m_tri_N-o ),
                             *it );
            o += bridge.m_chunk_size;
        }

        // retrieve buffer to CPU for debugging
        m_tri_info.m_body.resize( 2*bridge.m_tri_N );
        glGetBufferSubData( GL_TEXTURE_BUFFER,
                            NULL,
                            2*sizeof(GLuint)*bridge.m_tri_N,
                            m_tri_info.m_body.data() );


        glBindBuffer( GL_TEXTURE_BUFFER, m_tri_info.m_buffer );
        glBindTexture( GL_TEXTURE_BUFFER, m_tri_info.m_texture );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RG32UI, m_tri_info.m_buffer );
    }


    // Set up of vertex pos  VAO
    if( m_vertices.m_N > 0 ) {
        glBindVertexArray( m_vertices.m_vao );
        glBindBuffer( GL_ARRAY_BUFFER, m_vertices.m_vbo );
        glVertexPointer( 4, GL_FLOAT, 0, NULL );
        glEnableClientState( GL_VERTEX_ARRAY );
        glBindVertexArray( 0 );
    }

    // Set up of triangle index VAO
    if( m_triangles.m_index_count > 0 ) {
        glBindVertexArray( m_triangle_vao );
        glBindBuffer( GL_ARRAY_BUFFER, m_tri_info.m_buffer );
        glVertexAttribIPointer( 0, 2, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 0 );
        glBindBuffer( GL_ARRAY_BUFFER, m_triangles.m_ibo );
        glVertexAttribIPointer( 1, 3, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 1 );
        glBindBuffer( GL_ARRAY_BUFFER, m_tri_nrm_ix.m_buf );
        glVertexAttribIPointer( 2, 3, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 2 );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindVertexArray( 0 );
    }

    LOGGER_DEBUG( log, "Uploaded " << bridge.m_tri_vtx_chunks.size() << " chunks." );




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
