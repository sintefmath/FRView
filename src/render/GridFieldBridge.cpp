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

#include <stdexcept>
#include "utils/Logger.hpp"
#include "GridField.hpp"
#include "GridFieldBridge.hpp"

namespace render {

GridFieldBridge::GridFieldBridge()
    : m_count( 0 )
{
    m_memory = (REAL*)0xDEADBEEF;
}

GridFieldBridge::~GridFieldBridge()
{
}

void
GridFieldBridge::init( size_t count )
{
    m_count = count;
    m_values.clear();
    m_values.resize( sizeof(REAL)*count + 16 );
    m_memory = reinterpret_cast<REAL*>( 16*((reinterpret_cast<size_t>(m_values.data())+15)/16) );
    
}


GridFieldBridge::REAL*
GridFieldBridge::values()
{
    if( m_memory == (REAL*)0xDEADBEEF ) {
        throw std::runtime_error( "GridFieldBridge has not been initialized!" );
    }
    
    return m_memory;
}





} // of namespace render
