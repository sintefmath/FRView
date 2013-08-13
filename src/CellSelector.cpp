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
#include "GridField.hpp"
#include "GridTessSubset.hpp"
#include "CellSelector.hpp"

namespace resources {
    extern const std::string all_select_vs;
    extern const std::string field_select_vs;
    extern const std::string index_select_vs;
    extern const std::string halfplane_select_vs;
    extern const std::string plane_select_vs;
}

CellSelector::CellSelector( const std::string& vs )
{
    m_prog = glCreateProgram();
    m_vs = siut2::gl_utils::compileShader( vs, GL_VERTEX_SHADER, true );
    glAttachShader( m_prog, m_vs );
    const char* varyings[1] = {
        "selected"
    };
    glTransformFeedbackVaryings( m_prog, 1, varyings, GL_INTERLEAVED_ATTRIBS );
    siut2::gl_utils::linkProgram( m_prog );
    glGenQueries( 1, &m_query );
    CHECK_GL;
}


// -----------------------------------------------------------------------------

AllSelector::AllSelector()
    : CellSelector( resources::all_select_vs )
{
}

void
AllSelector::apply(GridTessSubset *tess_subset,
                   const GridTess *tess)
{
    glUseProgram( m_prog );
    tess_subset->populateBuffer( tess );
    glUseProgram( 0 );
    CHECK_GL;
}


// -----------------------------------------------------------------------------
FieldSelector::FieldSelector()
    : CellSelector( resources::field_select_vs )
{
    m_loc_min_max = glGetUniformLocation( m_prog, "min_max" );
}

void
FieldSelector::apply(GridTessSubset *tess_subset,
                     const GridTess *tess,
                     const GridField *field,
                     const float minval,
                     const float maxval)
{
    glUseProgram( m_prog );
    glUniform2f( m_loc_min_max, minval, maxval );
    glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_BUFFER, field->texture() );
    tess_subset->populateBuffer( tess );
    glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glUseProgram( 0 );
    CHECK_GL;
}

// -----------------------------------------------------------------------------

IndexSelector::IndexSelector()
        : CellSelector( resources::index_select_vs )
{
    glUseProgram( m_prog );
    m_loc_grid_dim = glGetUniformLocation( m_prog, "grid_dim" );
    m_loc_index_min = glGetUniformLocation( m_prog, "index_min" );
    m_loc_index_max = glGetUniformLocation( m_prog, "index_max" );
    glUniform1i( glGetUniformLocation( m_prog, "cell_global_index"), 0 );
    glUseProgram( 0 );
}


void
IndexSelector::apply( GridTessSubset *tess_subset,
                      const GridTess *tess,
                      unsigned int n_i,
                      unsigned int n_j,
                      unsigned int n_k,
                      unsigned int min_i,
                      unsigned int min_j,
                      unsigned int min_k,
                      unsigned int max_i,
                      unsigned int max_j,
                      unsigned int max_k )
{
    glUseProgram( m_prog );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->cellGlobalIndexTexture() );
    glUniform3ui( m_loc_grid_dim, n_i, n_j, n_k );
    glUniform3ui( m_loc_index_min, min_i, min_j, min_k );
    glUniform3ui( m_loc_index_max, max_i, max_j, max_k );
    tess_subset->populateBuffer( tess );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glUseProgram( 0 );
    CHECK_GL;
}



// -----------------------------------------------------------------------------
HalfPlaneSelector::HalfPlaneSelector()
    : CellSelector( resources::halfplane_select_vs )
{
    glUseProgram( m_prog );
    m_loc_halfplane_eq = glGetUniformLocation( m_prog, "plane_equation" );
    glUniform1i( glGetUniformLocation( m_prog, "vertices" ), 0 );
    glUniform1i( glGetUniformLocation( m_prog, "cell_corner" ), 1 );
    glUseProgram( 0 );
    CHECK_GL;
}

void
HalfPlaneSelector::apply( GridTessSubset *tess_subset,
                          const GridTess *tess,
                          const float *equation )
{
    glUseProgram( m_prog );
    glUniform4fv( m_loc_halfplane_eq, 1, equation );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->vertexTexture() );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->cellCornerTexture() );
    tess_subset->populateBuffer( tess );
    glUseProgram( 0 );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    CHECK_GL;
}

// -----------------------------------------------------------------------------
PlaneSelector::PlaneSelector()
    : CellSelector( resources::plane_select_vs )
{
    glUseProgram( m_prog );
    m_loc_plane_eq = glGetUniformLocation( m_prog, "plane_equation" );
    glUniform1i( glGetUniformLocation( m_prog, "vertices" ), 0 );
    glUniform1i( glGetUniformLocation( m_prog, "cell_corner" ), 1 );
    glUseProgram( 0 );
    CHECK_GL;
}

void
PlaneSelector::apply( GridTessSubset *tess_subset,
                      const GridTess *tess,
                      const float *equation )
{
    glUseProgram( m_prog );
    glUniform4fv( m_loc_plane_eq, 1, equation );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->vertexTexture() );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->cellCornerTexture() );
    tess_subset->populateBuffer( tess );
    glUseProgram( 0 );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    CHECK_GL;
}
