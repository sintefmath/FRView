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
#include "render/mesh/CellSetInterface.hpp"
#include "render/subset/Representation.hpp"

namespace render {
    namespace subset {


Representation::Representation()
    : m_cells_total( 0 ),
      m_subset_buffer( "GridTessSubset.m_subset_buffer" ),
      m_subset_texture( "GridTessSubset.m_subset_texture" )
{
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_subset_buffer.get() );
    glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                  sizeof(GLuint)*1024,
                  NULL,
                  GL_STREAM_DRAW );
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );

    glBindTexture( GL_TEXTURE_BUFFER, m_subset_texture.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32UI, m_subset_buffer.get() );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
}

Representation::~Representation()
{
}

void
Representation::populateBuffer( boost::shared_ptr<const mesh::CellSetInterface> cell_set,
                                GLuint transform_feedback_index )
{
    if( cell_set->cellCount() == 0 ) {
        return;
    }
    if( m_cells_total != (GLsizei)cell_set->cellCount() ) {
        m_cells_total = cell_set->cellCount();
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_subset_buffer.get() );
        glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                      sizeof(GLuint)*((m_cells_total+31)/32),
                      NULL,
                      GL_STREAM_DRAW );
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );
    }

    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER,
                      transform_feedback_index,
                      m_subset_buffer.get() );
    glEnable( GL_RASTERIZER_DISCARD );
    glBeginTransformFeedback( GL_POINTS );
    glDrawArrays( GL_POINTS, 0, (m_cells_total+31)/32 );
    glEndTransformFeedback( );
    glDisable( GL_RASTERIZER_DISCARD );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0 );
}

    } // of namespace subset
} // of namespace render

