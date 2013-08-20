/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2013 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include "render/GridTess.hpp"
#include "render/GridField.hpp"
#include "render/subset/Representation.hpp"
#include "render/subset/BuilderSelectByFieldValue.hpp"

namespace render {
    namespace subset {
    namespace glsl {
        extern std::string BuilderSelectByFieldValue_vs;
    }

BuilderSelectByFieldValue::BuilderSelectByFieldValue()
    : Builder( glsl::BuilderSelectByFieldValue_vs )
{
    m_loc_min_max = glGetUniformLocation( m_program, "min_max" );
}

void
BuilderSelectByFieldValue::apply( boost::shared_ptr<Representation> tess_subset,
                     boost::shared_ptr<const GridTess> tess,
                     boost::shared_ptr<const GridField> field,
                     const float minval,
                     const float maxval)
{
    glUseProgram( m_program );
    glUniform2f( m_loc_min_max, minval, maxval );
    glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_BUFFER, field->texture() );
    tess_subset->populateBuffer( tess );
    glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glUseProgram( 0 );
}

    } // of namespace subset
} // of namespace render
