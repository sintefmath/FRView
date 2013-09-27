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
#include <vector>
#include <boost/utility.hpp>

namespace render {
    class GridTess;
    class GridField;

class GridFieldBridge : public boost::noncopyable
{
    friend class GridField;
public:
    typedef float REAL;

    GridFieldBridge( size_t count );

//    GridFieldBridge( GridField& owner, GridTess& specifier );

    ~GridFieldBridge();

    REAL*
    values() { return m_memory; }

    //std::vector<REAL>&
    //values() { return m_values; }

    size_t
    count() const { return m_count; }

    REAL&
    minimum() { return m_min_value; }

    REAL&
    maximum() { return m_max_value; }

protected:
//    GridField&          m_owner;
//    GridTess&           m_specifier;
    size_t                      m_count;
    std::vector<unsigned char>  m_values;
    REAL*                       m_memory;
    REAL                        m_min_value;
    REAL                        m_max_value;
};

} // of namespace render
