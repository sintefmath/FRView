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
#include "render/GridTessBridge.hpp"
#include "render/GridFieldBridge.hpp"

namespace dataset {

class PolyhedralDataInterface
{
public:
    typedef render::GridTessBridge      Tessellation;   ///< \todo Rename type.
    typedef render::GridFieldBridge     Field;          ///< \todo Rename type.

    virtual
    ~PolyhedralDataInterface();

    /** Extract grid geometry from datasource. */
    virtual
    void
    tessellation( Tessellation&                                  tessellation,
                  boost::shared_ptr<tinia::model::ExposedModel>  model,
                  const std::string&                             progress_description_key,
                  const std::string&                             progress_counter_key ) = 0;

    virtual
    void
    field( Field& field,
           const size_t field_index,
           const size_t timestep_index ) const = 0;
    
    virtual
    size_t
    fields() const = 0;
    
    virtual
    size_t
    timesteps() const = 0;
    
    virtual
    const std::string&
    fieldName( unsigned int name_index ) const = 0;
    
    virtual
    size_t
    activeCells() const = 0;
    
};


} // of namespace input
