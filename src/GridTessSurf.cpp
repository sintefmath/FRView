/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <siut2/gl_utils/GLSLtools.hpp>
#include "Logger.hpp"
#include "GridTess.hpp"
#include "GridTessSubset.hpp"
#include "GridTessSurf.hpp"

GridTessSurf::GridTessSurf()
{
    m_triangles.m_cell_buffer =  0u;
    m_triangles.m_cell_texture =  0u;
    m_triangles.m_cornerpoint_index_buffer = 0u;
    m_triangles.m_count_query = 0u;
    m_triangles.m_count = 0u;
    m_triangles.m_alloc = 0u;
    glGenBuffers( 1, &m_triangles.m_cell_buffer );
    glGenTextures( 1, &m_triangles.m_cell_texture );
    glGenBuffers( 1, &m_triangles.m_cornerpoint_index_buffer );
    glGenQueries( 1, &m_triangles.m_count_query );

    m_edges.m_enabled = false;
    m_edges.m_cornerpoint_index_buffer = 0u;
    m_edges.m_count_query = 0u;
    m_edges.m_count = 0;
    m_edges.m_alloc = 0;
    glGenBuffers( 1, &m_edges.m_cornerpoint_index_buffer );
    glGenQueries( 1, &m_edges.m_count_query );
}

GridTessSurf::~GridTessSurf()
{
    if( m_triangles.m_cell_buffer != 0 ) {
        glDeleteBuffers( 1, &m_triangles.m_cell_buffer );
        m_triangles.m_cell_buffer = 0;
    }
    if( m_triangles.m_cell_texture != 0 ) {
        glDeleteTextures( 1, &m_triangles.m_cell_texture );
        m_triangles.m_cell_texture = 0;
    }
    if( m_triangles.m_cornerpoint_index_buffer != 0 ) {
        glDeleteBuffers( 1, &m_triangles.m_cornerpoint_index_buffer );
        m_triangles.m_cornerpoint_index_buffer = 0;
    }
    if( m_triangles.m_count_query != 0 ) {
        glDeleteQueries( 1, &m_triangles.m_count_query );
        m_triangles.m_count_query = 0;
    }
    if( m_edges.m_cornerpoint_index_buffer != 0 ) {
        glDeleteBuffers( 1, &m_edges.m_cornerpoint_index_buffer );
        m_edges.m_cornerpoint_index_buffer = 0;
    }
    if( m_edges.m_count_query != 0 ) {
        glDeleteQueries( 1, &m_edges.m_count_query );
        m_edges.m_count_query = 0;
    }


}


void
GridTessSurf::clearEdgeBuffer()
{
    m_edges.m_enabled = false;
    m_edges.m_count = 0;
}

void
GridTessSurf::resizeEdgeBuffer( const GLsizei count )
{
    Logger log = getLogger( "GridTessSurf.resizeEdgeBuffer" );
    m_edges.m_alloc = count;
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_edges.m_cornerpoint_index_buffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                  sizeof(GLuint)*2*m_edges.m_alloc,
                  NULL,
                  GL_STREAM_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    LOGGER_DEBUG( log, "Allocated space for " << m_edges.m_alloc << " edges." );
    CHECK_GL;
}

void
GridTessSurf::populateEdgeBuffer( const GridTess*  tess,
                                  GLuint           edge_indices_index )
{
    Logger log = getLogger( "GridTessSurf.populateEdgeBuffer" );
    CHECK_GL;
    m_edges.m_enabled = true;
    if( tess->edgeCount() == 0 ) {
        return;
    }


    glEnable( GL_RASTERIZER_DISCARD );

    // Finite number of iterations to avoid infinite loops
    for(uint i=0; i<2; i++) {
        // populate buffer
        glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, edge_indices_index, m_edges.m_cornerpoint_index_buffer );

        glBeginQuery( GL_PRIMITIVES_GENERATED, m_edges.m_count_query );
        glBeginTransformFeedback( GL_POINTS );
        glDrawArrays( GL_POINTS, 0, tess->edgeCount() );
        glEndTransformFeedback();
        glEndQuery( GL_PRIMITIVES_GENERATED );

        // check if buffer was large enough
        GLuint result;
        glGetQueryObjectuiv( m_edges.m_count_query,
                             GL_QUERY_RESULT,
                             &result );
        m_edges.m_count = result;
        if( m_edges.m_count <= m_edges.m_alloc ) {
            // Buffer was large enough.
            break;
        }
        else {
            LOGGER_DEBUG( log, "Buffer can hold " << m_edges.m_alloc
                          << " edges, but surface contains " << m_edges.m_count << ", resizing." );
            resizeEdgeBuffer( m_edges.m_count );
        }
    }
    glDisable( GL_RASTERIZER_DISCARD );
}

void
GridTessSurf::resizeTriangleBuffer( const GLsizei count )
{
    Logger log = getLogger( "GridTessSurf.resizeTriangleBuffer" );
    m_triangles.m_alloc = count;
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_triangles.m_cell_buffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                  sizeof(GLuint)*4*m_triangles.m_alloc,
                  NULL,
                  GL_STREAM_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_triangles.m_cornerpoint_index_buffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                  sizeof(GLuint)*3*m_triangles.m_alloc,
                  NULL,
                  GL_STREAM_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    LOGGER_DEBUG( log, "Allocated space for " << m_triangles.m_alloc << " triangles" );
    CHECK_GL;
}

void
GridTessSurf::populatePolygonBuffer( const GridTess*        tess,
                                     GLsizei                N,
                                     GLuint                 tri_cell_index,
                                     GLuint                 tri_indices_index )
{
    Logger log = getLogger( "GridTessSurf.populateTriangleBuffer" );

    glEnable( GL_RASTERIZER_DISCARD );
    for(uint i=0; i<2; i++) {
        // populate buffer
        glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, tri_cell_index, m_triangles.m_cell_buffer );
        glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, tri_indices_index, m_triangles.m_cornerpoint_index_buffer );

        glBeginQuery( GL_PRIMITIVES_GENERATED, m_triangles.m_count_query );
        glBeginTransformFeedback( GL_POINTS );
        glDrawArrays( GL_POINTS, 0, N );
        //glDrawArrays( GL_POINTS, 0, tess->polygonCount() );
        glEndTransformFeedback( );
        glEndQuery( GL_PRIMITIVES_GENERATED );
        CHECK_GL;

        // check if buffer was large enough
        GLuint result;
        glGetQueryObjectuiv( m_triangles.m_count_query,
                             GL_QUERY_RESULT,
                             &result );
        m_triangles.m_count = result;
        if( m_triangles.m_count <= m_triangles.m_alloc ) {
            break;
        }
        else {
            LOGGER_DEBUG( log, "Buffer can hold " << m_triangles.m_alloc
                          << " triangles, but surface contains " << m_triangles.m_count << ", resizing." );
            resizeTriangleBuffer( m_triangles.m_count );
        }
    }
    glDisable( GL_RASTERIZER_DISCARD );

    // We do this at last to avoid messing with the bound shader program.
    glBindTexture( GL_TEXTURE_BUFFER, m_triangles.m_cell_texture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32UI, m_triangles.m_cell_buffer );
    //glTexParameteri( GL_TEXTURE_BUFFER, GL_TEXTURE_BASE_LEVEL, 0 );
    //glTexParameteri( GL_TEXTURE_BUFFER, GL_TEXTURE_MAX_LEVEL, 0 );

    glBindTexture( GL_TEXTURE_BUFFER, 0 );

}

void
GridTessSurf::populateTriangleBuffer( const GridTess*  tess,
                                      GLuint           tri_cell_index,
                                      GLuint           tri_indices_index )
{
    Logger log = getLogger( "GridTessSurf.populateTriangleBuffer" );

    glEnable( GL_RASTERIZER_DISCARD );
    for(uint i=0; i<2; i++) {
        // populate buffer
        glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, tri_cell_index, m_triangles.m_cell_buffer );
        glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, tri_indices_index, m_triangles.m_cornerpoint_index_buffer );

        glBeginQuery( GL_PRIMITIVES_GENERATED, m_triangles.m_count_query );
        glBeginTransformFeedback( GL_POINTS );
        glDrawArrays( GL_POINTS, 0, tess->triangleCount() );
        glEndTransformFeedback( );
        glEndQuery( GL_PRIMITIVES_GENERATED );
        CHECK_GL;

        // check if buffer was large enough
        GLuint result;
        glGetQueryObjectuiv( m_triangles.m_count_query,
                             GL_QUERY_RESULT,
                             &result );
        m_triangles.m_count = result;
        if( m_triangles.m_count <= m_triangles.m_alloc ) {
            break;
        }
        else {
            LOGGER_DEBUG( log, "Buffer can hold " << m_triangles.m_alloc
                          << " triangles, but surface contains " << m_triangles.m_count << ", resizing." );
            resizeTriangleBuffer( m_triangles.m_count );
        }
    }
    glDisable( GL_RASTERIZER_DISCARD );

    // We do this at last to avoid messing with the bound shader program.
    glBindTexture( GL_TEXTURE_BUFFER, m_triangles.m_cell_texture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32UI, m_triangles.m_cell_buffer );
    //glTexParameteri( GL_TEXTURE_BUFFER, GL_TEXTURE_BASE_LEVEL, 0 );
    //glTexParameteri( GL_TEXTURE_BUFFER, GL_TEXTURE_MAX_LEVEL, 0 );

    glBindTexture( GL_TEXTURE_BUFFER, 0 );
}
