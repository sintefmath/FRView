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

#include "utils/Logger.hpp"
#include "GridField.hpp"
#include "GridFieldBridge.hpp"

namespace render {

GridFieldBridge::GridFieldBridge( size_t count )
    : m_count( count )
{
    m_values.resize( sizeof(REAL)*m_count + 16 );
    m_memory = reinterpret_cast<REAL*>( 16*((reinterpret_cast<size_t>(m_values.data())+15)/16) );

    /*

    m_values.resize( m_specifier.cellCount() );

    glBindBuffer( GL_TEXTURE_BUFFER, m_owner.m_buffer );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GridFieldBridge::REAL)*m_specifier.cellCount(),
                  NULL,
                  GL_STATIC_DRAW );
    m_memory =
            reinterpret_cast<GridFieldBridge::REAL*>(
                glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY  )
                );
    if( m_memory == NULL ) {
        throw std::runtime_error( "Failed to map buffer" );
    }

    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
*/
}

GridFieldBridge::~GridFieldBridge()
{
/*
    glBindBuffer( GL_TEXTURE_BUFFER, m_owner.m_buffer );
    if( glUnmapBuffer( GL_TEXTURE_BUFFER ) == GL_FALSE) {
        Logger log = getLogger( "GridFieldBridge.~GridFieldBridge" );
        LOGGER_WARN( log, "Failed to unmap buffer." );
    }
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
    m_owner.import( *this );
*/
}

} // of namespace render
