#include "utils/Logger.hpp"
#include <siut2/gl_utils/GLSLtools.hpp>
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
