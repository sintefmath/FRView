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
    apply( boost::shared_ptr<Representation>                cell_subset,
           boost::shared_ptr<const mesh::CellSetInterface>  cell_set,
           const float*                                     equation );

protected:
    GLint   m_loc_halfplane_eq;     ///< Uniform location of plane equation (vec4).
};    
    } // of namespace subset
} // of namespace render
