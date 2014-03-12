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

#include <sstream>
#include "dataset/PolyhedralDataInterface.hpp"

namespace dataset {

PolyhedralDataInterface::~PolyhedralDataInterface()
{
}

const std::string
PolyhedralDataInterface::timestepDescription( size_t timestep_index ) const
{
    std::stringstream o;
    o << timestep_index;
    return o.str();
}

const std::string
PolyhedralDataInterface::fieldName( unsigned int name_index ) const
{
    std::stringstream o;
    o << name_index;
    return o.str();
}


} // of namespace dataset
