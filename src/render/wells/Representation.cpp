/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2013 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

#include <glm/glm.hpp>
#include "utils/Logger.hpp"
#include "render/wells/Representation.hpp"

namespace render {
    namespace wells {
        static const std::string package = "render.wells.Representation";

    
Representation::Representation()
    : m_do_upload( false ),
      m_attribs_vao( package + ".m_attribs_vao" ),
      m_attribs_buf( package + ".m_attribs_buf" ),
      m_indices_buf( package + ".m_indices_buf" )
{
    
}

void
Representation::clear()
{
    m_attribs.clear();
    m_indices.clear();
}

void
Representation::addSegments( const std::vector<float>& positions,
                             const std::vector<float>& colors )
{
    if( positions.size() < 2 ) {
        return;
    }
    Logger log = getLogger( package + ".addSegments" );

    size_t N = positions.size()/3;

    std::vector<float> tangent( 3*N );

    for( unsigned int i=0; i<3; i++ ) {
        tangent[ i ] = positions[ 3 + i ] - positions[i];
    }


    glm::vec3 pn;

    if( (fabsf( tangent[0] ) > fabs( tangent[1] ) ) && (fabsf( tangent[1] ) > fabs( tangent[2] ) ) ) {
        pn = glm::vec3( 0.f, 0.f, 1.f );
    }
    else if( fabsf( tangent[1] ) > fabs( tangent[2] ) ) {
        pn = glm::vec3( 0.f, 0.f, 1.f );
    }
    else {
        pn = glm::vec3( 1.f, 0.f, 0.f );
    }

    GLuint start = m_attribs.size()/12;

    m_attribs.reserve( 12*N );
    for( size_t i=0; i<N; i++ ) {
        size_t ia = std::max( (size_t)1, i ) - 1u;
        size_t ib = std::min( N-1u, i+1 );
        glm::vec3 pa( positions[ 3*ia+0 ], positions[ 3*ia+1 ], positions[ 3*ia+2 ] );
        glm::vec3 pb( positions[ 3*ib+0 ], positions[ 3*ib+1 ], positions[ 3*ib+2 ] );
        glm::vec3 t = (1.f/(ib-ia))*(pb-pa);

        if( glm::dot(t,t) < std::numeric_limits<float>::epsilon() ) {
            continue;
        }


        glm::vec3 n = glm::normalize( pn - (glm::dot(pn, t)/glm::dot( t, t ) )*t );

        /*
        LOGGER_DEBUG( log, (m_attribs.size()/12) << " pos: " <<
                      positions[ 3*i+0 ] << ", " <<
                      positions[ 3*i+1 ] << ", " <<
                      positions[ 3*i+2 ] << ", tan: " <<
                      t.x << ", " << t.y << ", " << t.z << ", nrm: " <<
                      n.x << ", " << n.y << ", " << n.z
                      );
*/
        m_attribs.push_back( positions[ 3*i+0 ] );
        m_attribs.push_back( positions[ 3*i+1 ] );
        m_attribs.push_back( positions[ 3*i+2 ] );

        m_attribs.push_back( t.x );
        m_attribs.push_back( t.y );
        m_attribs.push_back( t.z );

        m_attribs.push_back( n.x );
        m_attribs.push_back( n.y );
        m_attribs.push_back( n.z );

        m_attribs.push_back( colors[ 3*i+0 ] );
        m_attribs.push_back( colors[ 3*i+1 ] );
        m_attribs.push_back( colors[ 3*i+2 ] );

        pn = n;
    }

    N = m_attribs.size()/12 - start;

    m_indices.reserve( 2*(N-1) );
    for( size_t i=0; i<(N-1); i++ ) {
        m_indices.push_back( start + i );
        m_indices.push_back( start + i + 1 );
        //LOGGER_DEBUG( log, (start+i) << ", " << (start+i+1) );
    }
    m_do_upload = true;

}

void
Representation::upload()
{
    if( !m_do_upload ) {
        return;
    }
    m_do_upload = false;

    glBindBuffer( GL_ARRAY_BUFFER, m_attribs_buf.get() );
    glBufferData( GL_ARRAY_BUFFER,
                  sizeof(GLfloat)*m_attribs.size(),
                  m_attribs.data(),
                  GL_STATIC_DRAW );
    
    glBindVertexArray( m_attribs_vao.get() );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*12, reinterpret_cast<const GLvoid*>( sizeof(GLfloat)*3*0 ) );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*12, reinterpret_cast<const GLvoid*>( sizeof(GLfloat)*3*1 ) );
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*12, reinterpret_cast<const GLvoid*>( sizeof(GLfloat)*3*2 ) );
    glEnableVertexAttribArray( 2 );
    glVertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*12, reinterpret_cast<const GLvoid*>( sizeof(GLfloat)*3*3 ) );
    glEnableVertexAttribArray( 3 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indices_buf.get() );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                  sizeof(GLuint)*m_indices.size(),
                  m_indices.data(),
                  GL_STATIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}


    } // of namespace wells
} // of namespace render
