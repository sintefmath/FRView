/* Copyright STIFTELSEN SINTEF 2013
 * 
 * This file is part of FRView.
 * FRView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * FRView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *  
 * You should have received a copy of the GNU Affero General Public License
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */

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
    apply( boost::shared_ptr<Representation>                cell_subset,
           boost::shared_ptr<const mesh::CellSetInterface>  cell_set,
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
