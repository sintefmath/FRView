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

/** Select all cells with geometry partially or fully inside a half-plane.
 *
 * See halfplane_select_vs.glsl for the actual implementation.
 */
class BuilderSelectInsideHalfplane : public Builder
{
public:
    BuilderSelectInsideHalfplane();

    /** Populates a \ref GridTessSubset.
     *
     * \param tess_subset  Data store for the resulting subset.
     * \param tess         Grid from which to select cells.
     * \param equation     The plane equation to use (4 components).
     */
    void
    apply( boost::shared_ptr<Representation>  tess_subset,
           boost::shared_ptr<const GridTess>  tess,
           const float*     equation );

protected:
    GLint   m_loc_halfplane_eq;     ///< Uniform location of plane equation (vec4).
};    
    } // of namespace subset
} // of namespace render
