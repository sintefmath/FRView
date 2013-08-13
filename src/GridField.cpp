/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <GL/glew.h>
#include <iostream>
#include <siut2/gl_utils/GLSLtools.hpp>
#include "Logger.hpp"
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
    m_values.resize( m_specifier.activeCells() );

    glBindBuffer( GL_TEXTURE_BUFFER, m_owner.m_buffer );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(GridFieldBridge::REAL)*m_specifier.activeCells(),
                  NULL,
                  GL_STATIC_DRAW );
    CHECK_GL;
    m_memory =
            reinterpret_cast<GridFieldBridge::REAL*>(
                glMapBuffer( GL_TEXTURE_BUFFER, GL_WRITE_ONLY  )
                );
    if( m_memory == NULL ) {
        throw std::runtime_error( "Failed to map buffer" );
    }

    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
    CHECK_GL;
}

GridFieldBridge::~GridFieldBridge()
{
    glBindBuffer( GL_TEXTURE_BUFFER, m_owner.m_buffer );
    if( glUnmapBuffer( GL_TEXTURE_BUFFER ) == GL_FALSE) {
        Logger log = getLogger( "GridFieldBridge.~GridFieldBridge" );
        LOGGER_WARN( log, "Failed to unmap buffer." );
    }
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
    m_owner.import( *this );
}



void
GridField::import( GridFieldBridge& bridge )
{
/*
    glBindBuffer( GL_TEXTURE_BUFFER, m_buffer );
    glBufferData( GL_TEXTURE_BUFFER,
                  sizeof(float)*bridge.m_values.size(),
                  bridge.m_values.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
*/
    m_min_value = bridge.minimum();
    m_max_value = bridge.maximum();

    glBindTexture( GL_TEXTURE_BUFFER, m_texture );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32F, m_buffer );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );

    m_has_data = true;
}
