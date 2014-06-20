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
#include <string>
#include <vector>

namespace dataset {

/** Interface for data sources that provides wells.
 *
 * \todo This interface is ugly and rather hackish, and should redone.
 */
class WellDataInterace
{
public:
    virtual
    ~WellDataInterace();
    
    struct Well
    {
        bool                                        m_defined;
        std::string                                 m_name;
        float                                       m_head[3];
        std::vector< std::vector<float> >           m_branches;
    };

    /** Return the number of wells in the dataset. */    
    virtual
    const unsigned int
    wellCount() const = 0;

    /** Return the well descriptions for a given timestep.
     *
     * \todo Replace by a bridge API.
     */
    virtual
    const std::vector<Well>&
    wells( const unsigned int step ) = 0;
    
    /** Return the name of a particular well. */
    virtual
    const std::string&
    wellName( unsigned int well_ix ) const = 0;

    /** Check if a well is defined at a given time step. */
    virtual
    const bool
    wellDefined( const unsigned int report_step_ix,
                 const unsigned int well_ix ) const = 0;

    virtual
    const float*
    wellHeadPosition( const unsigned int report_step_ix,
                      const unsigned int well_ix ) const = 0;

    virtual
    const unsigned int
    wellBranchCount( const unsigned int report_step_ix,
                     const unsigned int well_ix ) const = 0;

    virtual
    const std::vector<float>&
    wellBranchPositions( const unsigned int report_step_ix,
                         const unsigned int well_ix,
                         const unsigned int branch_ix ) = 0;
    
};

} // of namespace dataset
