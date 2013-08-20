/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2013 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once
#include "render/subset/Builder.hpp"

namespace render {
    namespace subset {

/** Selects all cells in a grid. */
class BuilderSelectAll : public Builder
{
public:
    BuilderSelectAll();

    /** Populates a \ref GridTessSubset by selecting all cells in a \ref GridTess. */
    void
    apply( boost::shared_ptr<Representation> tess_subset,
           boost::shared_ptr<GridTess const> tess );

protected:
};
    
    } // of namespace subset
} // of namespace render
