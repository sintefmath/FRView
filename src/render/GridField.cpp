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

#include <GL/glew.h>
#include <iostream>
#include "utils/Logger.hpp"
#include "GridTess.hpp"
#include "GridField.hpp"
#include "GridFieldBridge.hpp"

namespace render {

GridField::GridField(boost::shared_ptr<GridTess> grid )
    : m_grid( grid ),
      m_has_data( false ),
      m_buffer( "GridField.m_buffer" ),
      m_texture( "GridField.m_texture" )
{
}



void
GridField::import( GridFieldBridge& bridge )
{
    Logger log = getLogger( "GridField.import" );

    LOGGER_DEBUG( log, "bridge.count=" << bridge.count() << ", cellCount=" << m_grid->cellCount() );

    if( true || bridge.count() == m_grid->cellCount() ) {
        // compacted data
        glBindBuffer( GL_TEXTURE_BUFFER, m_buffer.get() );
        glBufferData( GL_TEXTURE_BUFFER,
                      sizeof(float)*bridge.count(),
                      bridge.values(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_TEXTURE_BUFFER, 0 );

        m_min_value = bridge.minimum();
        m_max_value = bridge.maximum();
    }
    else {
        // assume that we are dealing with a uncompacted field
        LOGGER_DEBUG( log, "Encountered uncompacted data" );

        std::vector<GLfloat> values( m_grid->cellCount() );
        const std::vector<GLuint>& indices = m_grid->cellGlobalIndicesInHostMemory();

        float* data = bridge.values();

        m_min_value =  std::numeric_limits<float>::max();
        m_max_value = -std::numeric_limits<float>::max();
        for(size_t i=0; i<values.size(); i++ ) {
            values[i] = data[ indices[ i ] ];
            m_min_value = std::min( values[i], m_min_value );
            m_max_value = std::max( values[i], m_max_value );
        }

        glBindBuffer( GL_TEXTURE_BUFFER, m_buffer.get() );
        glBufferData( GL_TEXTURE_BUFFER,
                      sizeof(float)*values.size(),
                      values.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_TEXTURE_BUFFER, 0 );
    }

    glBindTexture( GL_TEXTURE_BUFFER, m_texture.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32F, m_buffer.get() );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );

    m_has_data = true;



}

} // of namespace render
