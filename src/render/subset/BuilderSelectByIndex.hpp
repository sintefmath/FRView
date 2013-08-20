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

/** Select all cells within a given index range.
 *
 * This only makes sense for grids originating from a corner-point grid. The
 * global index of cells is assumed to be the non-compacted indices that
 * linerizes the 3D grid.
 *
 * See index_select_vs.glsl for the actual implementation.
 */
class BuilderSelectByIndex : public Builder
{
public:
    BuilderSelectByIndex();

    /** Populates a \ref GridTessSubset by selecting cells in a \ref GridTess within a given index range.
     *
     * \param tess_subset  Data store for the resulting subset.
     * \param tess         Grid from which to select cells.
     * \param n_i          Number of global cells along the i-axis.
     * \param n_j          Number of global cells along the j-axis.
     * \param n_k          Number of global cells along the k-axis.
     * \param min_i        Minimum i of index range (inclusive).
     * \param min_j        Minimum j of index range (inclusive).
     * \param min_k        Minimum k of index range (inclusive).
     * \param max_i        Maximum i of index range (inclusive).
     * \param max_j        Maximum j of index range (inclusive).
     * \param max_k        Maximum k of index range (inclusive).
     */
    void
    apply( boost::shared_ptr<Representation> tess_subset,
           boost::shared_ptr<const GridTess> tess,
           unsigned int n_i,
           unsigned int n_j,
           unsigned int n_k,
           unsigned int min_i,
           unsigned int min_j,
           unsigned int min_k,
           unsigned int max_i,
           unsigned int max_j,
           unsigned int max_k );

protected:
    GLint       m_loc_grid_dim;
    GLint       m_loc_index_min;
    GLint       m_loc_index_max;

};
    
    } // of namespace subset
} // of namespace render
