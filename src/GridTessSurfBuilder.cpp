/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <siut2/gl_utils/GLSLtools.hpp>
#include <siut2/io_utils/snarf.hpp>

#include "GridTess.hpp"
#include "GridTessSubset.hpp"
#include "GridTessSurf.hpp"
#include "GridTessSurfBuilder.hpp"


GridTessSurfBuilder::GridTessSurfBuilder()
    : m_compact_subset_prog( 0u )
{
    const char* feedback[2] = {
        "cell",
        "indices"
    };

    GLuint vs                 = siut2::gl_utils::compileShader( siut2::io_utils::snarfFile( "shaders/compact_vs.glsl" ), GL_VERTEX_SHADER, true );
    GLuint subset_gs          = siut2::gl_utils::compileShader( siut2::io_utils::snarfFile( "shaders/compact_subset_gs.glsl" ), GL_GEOMETRY_SHADER, true );
    GLuint subset_boundary_gs = siut2::gl_utils::compileShader( siut2::io_utils::snarfFile( "shaders/compact_subset_boundary_gs.glsl" ), GL_GEOMETRY_SHADER, true );
    GLuint fault_gs           = siut2::gl_utils::compileShader( siut2::io_utils::snarfFile( "shaders/compact_fault_gs.glsl"), GL_GEOMETRY_SHADER, true );
    GLuint boundary_gs        = siut2::gl_utils::compileShader( siut2::io_utils::snarfFile( "shaders/compact_boundary_gs.glsl"), GL_GEOMETRY_SHADER, true );

    m_compact_subset_prog = glCreateProgram();
    glAttachShader( m_compact_subset_prog, vs );
    glAttachShader( m_compact_subset_prog, subset_gs );
    glTransformFeedbackVaryings( m_compact_subset_prog, 2, feedback, GL_SEPARATE_ATTRIBS );
    siut2::gl_utils::linkProgram( m_compact_subset_prog );
    m_compact_subset_loc_flip = glGetUniformLocation( m_compact_subset_prog, "flip_faces" );

    m_compact_subset_boundary_prog = glCreateProgram();
    glAttachShader( m_compact_subset_boundary_prog, vs );
    glAttachShader( m_compact_subset_boundary_prog, subset_boundary_gs );
    glTransformFeedbackVaryings( m_compact_subset_boundary_prog, 2, feedback, GL_SEPARATE_ATTRIBS );
    siut2::gl_utils::linkProgram( m_compact_subset_boundary_prog );
    m_compact_subset_boundary_loc_flip = glGetUniformLocation( m_compact_subset_boundary_prog, "flip_faces" );

    m_compact_fault_prog = glCreateProgram();
    glAttachShader( m_compact_fault_prog, vs );
    glAttachShader( m_compact_fault_prog, fault_gs );
    glTransformFeedbackVaryings( m_compact_fault_prog, 2, feedback, GL_SEPARATE_ATTRIBS );
    siut2::gl_utils::linkProgram( m_compact_fault_prog );
    m_compact_fault_loc_flip = glGetUniformLocation( m_compact_fault_prog, "flip_faces" );

    m_compact_boundary_prog = glCreateProgram();
    glAttachShader( m_compact_boundary_prog, vs );
    glAttachShader( m_compact_boundary_prog, boundary_gs );
    glTransformFeedbackVaryings( m_compact_boundary_prog, 2, feedback, GL_SEPARATE_ATTRIBS );
    siut2::gl_utils::linkProgram( m_compact_boundary_prog );
    m_compact_boundary_loc_flip = glGetUniformLocation( m_compact_boundary_prog, "flip_faces" );

    glDeleteShader( vs );
    glDeleteShader( subset_gs );
    glDeleteShader( subset_boundary_gs );
    glDeleteShader( fault_gs );
    glDeleteShader( boundary_gs );
    CHECK_GL;
}

GridTessSurfBuilder::~GridTessSurfBuilder()
{
    if( m_compact_subset_prog != 0u ) {
        glDeleteProgram( m_compact_subset_prog );
    }
}

void
GridTessSurfBuilder::buildSubsetSurface( GridTessSurf*          surf,
                                         const GridTessSubset*  subset,
                                         const GridTess*        tess,
                                         bool                   flip_faces )
{
    glBindVertexArray( tess->triangleVertexArray() );

    glUseProgram( m_compact_subset_prog );
    glUniform1i( m_compact_subset_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, subset->texture() );

    surf->populateBuffer( tess );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glUseProgram( 0 );

    glBindVertexArray( 0 );
    CHECK_GL;
}

void
GridTessSurfBuilder::buildSubsetBoundarySurface( GridTessSurf*          surf,
                                                 const GridTessSubset*  subset,
                                                 const GridTess*        tess,
                                                 bool                   flip_faces )
{
    glBindVertexArray( tess->triangleVertexArray() );

    glUseProgram( m_compact_subset_boundary_prog );
    glUniform1i( m_compact_subset_boundary_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, subset->texture() );

    surf->populateBuffer( tess );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glUseProgram( 0 );

    glBindVertexArray( 0 );
    CHECK_GL;
}

void
GridTessSurfBuilder::buildFaultSurface( GridTessSurf* surf, const GridTess* tess, bool flip_faces )
{
    glBindVertexArray( tess->triangleVertexArray() );

    glUseProgram( m_compact_fault_prog );
    glUniform1i( m_compact_fault_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );

    surf->populateBuffer( tess );

    glUseProgram( 0 );
    glBindVertexArray( 0 );
    CHECK_GL;
}

void
GridTessSurfBuilder::buildBoundarySurface( GridTessSurf* surf, const GridTess* tess, bool flip_faces )
{
    glBindVertexArray( tess->triangleVertexArray() );

    glUseProgram( m_compact_boundary_prog );
    glUniform1i( m_compact_boundary_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );

    surf->populateBuffer( tess );

    glUseProgram( 0 );
    glBindVertexArray( 0 );
    CHECK_GL;
}
