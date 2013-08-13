#include <iostream>
#include <siut/gl_utils/GLSLtools.hpp>
#include "GridTess.hpp"
#include "GridField.hpp"
#include "GridFieldBridge.hpp"


GridField::GridField( )
    : m_has_data( false )
{
    glGenBuffers( 1, &m_buffer );
    glGenTextures( 1, &m_texture );
}


GridFieldBridge::GridFieldBridge(GridField &owner, GridTess &specifier)
    : m_owner( owner ),
      m_specifier( specifier )
{
}

GridFieldBridge::~GridFieldBridge()
{
    m_owner.import( *this );
}

void
GridField::import( GridFieldBridge& bridge )
{
    m_has_data = false;
    if( bridge.values().empty() ) {
        return;
    }

    if( bridge.m_values.size() == bridge.m_specifier.activeCells() ) {
        glBindBuffer( GL_TEXTURE_BUFFER, m_buffer );
        glBufferData( GL_TEXTURE_BUFFER,
                      sizeof(float)*bridge.m_values.size(),
                      bridge.m_values.data(),
                      GL_STATIC_DRAW );
        glBindBuffer( GL_TEXTURE_BUFFER, 0 );


        m_min_value = m_max_value = bridge.m_values[0];
        for( auto it = bridge.m_values.begin(); it!=bridge.m_values.end(); ++it ) {
            m_min_value = std::min( m_min_value, *it );
            m_max_value = std::max( m_max_value, *it );
        }
        m_has_data = true;
    }

    if( m_has_data ) {
        glBindTexture( GL_TEXTURE_BUFFER, m_texture );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_R32F, m_buffer );
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
    }
}
