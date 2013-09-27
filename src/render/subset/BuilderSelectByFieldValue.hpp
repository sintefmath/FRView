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


/** Selects all cells where a field value is within a certain range.
 *
 * See field_select_vs.glsl for the actual implementation.
 */
class BuilderSelectByFieldValue : public Builder
{
public:
    BuilderSelectByFieldValue();

    /** Populates a \ref GridTessSubset by selecting cells in a \ref GridTess where a field value is within a certain range.
     *
     * \param tess_subset  Data store for the resulting subset.
     * \param tess         Grid from which to select cells.
     * \param field        The field that provides the cell property.
     * \param minval       Minimum value for the range (inclusive).
     * \param maxval       Maximum value for the range (inclusive).
     */
    void
    apply( boost::shared_ptr<Representation>  tess_subset,
           boost::shared_ptr<const GridTess>  tess,
           boost::shared_ptr<const GridField> field,
           const float      minval,
           const float      maxval );

protected:
    GLint   m_loc_min_max;      ///< Uniform location of min and max value (vec2).
};
    
    } // of namespace subset
} // of namespace render
