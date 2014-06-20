#pragma once
/* Copyright STIFTELSEN SINTEF 2014
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
#include "bridge/FieldBridge.hpp"


namespace dataset {

/** Interface that describes sources that provides fields. */
class FieldDataInterface
{
public:

    virtual
    ~FieldDataInterface();
    
    /** Extract field from datasource. */
    virtual
    void
    field( boost::shared_ptr<bridge::FieldBridge>  bridge,
           const size_t                            field_index,
           const size_t                            timestep_index ) const = 0;

    /** Number of fields in datasource. */
    virtual
    size_t
    fields() const = 0;
    
    virtual
    bool
    validFieldAtTimestep( size_t field_index, size_t timestep_index ) const = 0;
    
    /** Number of timesteps in datasource. */
    virtual
    size_t
    timesteps() const = 0;

    /** Describes a timestep (elapsed seconds, date, etc).
     *
     * Default implementation just returns timestep_index as a string.
     */
    virtual
    const std::string
    timestepDescription( size_t timestep_index ) const;
    
    /** Returns a descriptive name of a field.
     *
     * Default implementation returns name_index as a string.
     */
    virtual
    const std::string
    fieldName( unsigned int name_index ) const;
    
};



} // of namespace dataset
