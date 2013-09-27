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
