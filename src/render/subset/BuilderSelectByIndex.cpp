/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2013 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include "render/GridTess.hpp"
#include "render/subset/Representation.hpp"
#include "render/subset/BuilderSelectByIndex.hpp"

namespace render {
    namespace subset {
        namespace glsl {
            extern std::string BuilderSelectByIndex_vs;
        }

BuilderSelectByIndex::BuilderSelectByIndex()
        : Builder( glsl::BuilderSelectByIndex_vs )
{
    glUseProgram( m_program );
    m_loc_grid_dim = glGetUniformLocation( m_program, "grid_dim" );
    m_loc_index_min = glGetUniformLocation( m_program, "index_min" );
    m_loc_index_max = glGetUniformLocation( m_program, "index_max" );
    glUniform1i( glGetUniformLocation( m_program, "cell_global_index"), 0 );
    glUseProgram( 0 );
}

void
BuilderSelectByIndex::apply( boost::shared_ptr<Representation> tess_subset,
                      boost::shared_ptr<const GridTess> tess,
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
    glUseProgram( m_program );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->cellGlobalIndexTexture() );
    glUniform3ui( m_loc_grid_dim, n_i, n_j, n_k );
    glUniform3ui( m_loc_index_min, min_i, min_j, min_k );
    glUniform3ui( m_loc_index_max, max_i, max_j, max_k );
    tess_subset->populateBuffer( tess );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glUseProgram( 0 );
}

    } // of namespace subset
} // of namespace render
