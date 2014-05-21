#pragma once
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

#include <boost/shared_ptr.hpp>
#include <tinia/model/ExposedModel.hpp>

namespace bridge {
    class PolygonMeshBridge;
    class FieldBridge;
}

namespace dataset {

/** Interface that describes polygonal tessellations of surfaces. */
class PolygonDataInterface
{
public:

    virtual
    ~PolygonDataInterface();

    /** Extract grid geometry from datasource. */
    virtual
    void
    geometry( boost::shared_ptr<bridge::PolygonMeshBridge>   mesh_bridge,
              boost::shared_ptr<tinia::model::ExposedModel>  model,
              const std::string&                             progress_description_key,
              const std::string&                             progress_counter_key ) = 0;

};


} // of namespace input
