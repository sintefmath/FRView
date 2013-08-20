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
#include "render/subset/BuilderSelectAll.hpp"


namespace render {
    namespace subset {
        namespace glsl {
            extern std::string BuilderSelectAll_vs;
        }

BuilderSelectAll::BuilderSelectAll()
    : Builder( glsl::BuilderSelectAll_vs )
{
}

void
BuilderSelectAll::apply(boost::shared_ptr<Representation> tess_subset,
                   boost::shared_ptr<const GridTess> tess)
{
    glUseProgram( m_program );
    tess_subset->populateBuffer( tess );
    glUseProgram( 0 );
}


    } // of namespace subset
} // of namespace render
