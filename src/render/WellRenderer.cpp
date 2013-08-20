/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "utils/GLSLTools.hpp"
#include "utils/Logger.hpp"
#include "WellRenderer.hpp"

namespace resources {
    extern const std::string well_tube_vs;
    extern const std::string well_tube_cs;
    extern const std::string well_tube_es;
    extern const std::string well_tube_fs;
}

namespace render {
    static const std::string package = "render.WellRenderer";

WellRenderer::WellRenderer()
    : m_do_upload( false ),
      m_attribs_vao( 0u ),
      m_attribs_buf( 0u ),
      m_indices_buf( 0u )
{
    Logger log = getLogger( package + ".constructor" );
    
    glGenVertexArrays( 1, &m_attribs_vao );
    glGenBuffers( 1, &m_attribs_buf );
    glGenBuffers( 1, &m_indices_buf );


    GLuint well_vs = utils::compileShader( log, resources::well_tube_vs, GL_VERTEX_SHADER );
    GLuint well_cs = utils::compileShader( log, resources::well_tube_cs, GL_TESS_CONTROL_SHADER );
    GLuint well_es = utils::compileShader( log, resources::well_tube_es, GL_TESS_EVALUATION_SHADER );
    GLuint well_fs = utils::compileShader( log, resources::well_tube_fs, GL_FRAGMENT_SHADER );

    m_well_prog = glCreateProgram();
    glAttachShader( m_well_prog, well_vs );
    glAttachShader( m_well_prog, well_cs );
    glAttachShader( m_well_prog, well_es );
    glAttachShader( m_well_prog, well_fs );
    utils::linkProgram( log, m_well_prog );

    glDeleteShader( well_vs );
    glDeleteShader( well_cs );
    glDeleteShader( well_es );
    glDeleteShader( well_fs );


    /*
    std::vector<float> positions;
    std::vector<float> colors;

    float scale = 1.f/10.f;
    for( unsigned int i=0; i<20; i++ ) {
        float t = scale*( i );
        positions.push_back( 0.5f*cosf( t )*sinf( 3*t )+0.3f + 0.1*cos(11.f*t) );
        positions.push_back( 0.5f*cosf(2.f*t)*sinf(t)+0.3f + 0.1*sin(13.f*t) );
        positions.push_back( 0.5f*sinf(t)+0.3f );

        colors.push_back( (i & 0x1) == 0 ? 1.f : 0.5f );
        colors.push_back( (i & 0x2) == 0 ? 1.f : 0.5f );
        colors.push_back( (i & 0x4) == 0 ? 1.f : 0.5f );
    }
    addSegments( positions, colors );
*/
}

WellRenderer::~WellRenderer()
{
    glDeleteVertexArrays( 1, &m_attribs_vao );
    glDeleteBuffers( 1, &m_attribs_buf );
    glDeleteBuffers( 1, &m_indices_buf );
    glDeleteProgram( m_well_prog );
}

void
WellRenderer::clear()
{
    m_attribs.clear();
    m_indices.clear();
    m_do_upload = false;
}

void
WellRenderer::addSegments( const std::vector<float>& positions,
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
WellRenderer::render( GLsizei           width,
                      GLsizei           height,
                      const GLfloat*    projection,
                      const GLfloat*    camera_from_world,
                      const GLfloat*    world_from_model )
{
    if( m_indices.empty() ) {
        return;
    }
    Logger log = getLogger( package + ".render" );

    glBindVertexArray( m_attribs_vao );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indices_buf );
    if( m_do_upload ) {
        glBindBuffer( GL_ARRAY_BUFFER, m_attribs_buf );
        glBufferData( GL_ARRAY_BUFFER,
                      sizeof(GLfloat)*m_attribs.size(),
                      m_attribs.data(),
                      GL_STATIC_DRAW );
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*12, reinterpret_cast<const GLvoid*>( sizeof(GLfloat)*3*0 ) );
        glEnableVertexAttribArray( 0 );
        glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*12, reinterpret_cast<const GLvoid*>( sizeof(GLfloat)*3*1 ) );
        glEnableVertexAttribArray( 1 );
        glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*12, reinterpret_cast<const GLvoid*>( sizeof(GLfloat)*3*2 ) );
        glEnableVertexAttribArray( 2 );
        glVertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*12, reinterpret_cast<const GLvoid*>( sizeof(GLfloat)*3*3 ) );
        glEnableVertexAttribArray( 3 );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER,
                      sizeof(GLuint)*m_indices.size(),
                      m_indices.data(),
                      GL_STATIC_DRAW );
    }


    /*
    glm::mat4 cfw = glm::transpose( glm::inverse( glm::mat4( camera_from_world[0], camera_from_world[1],  camera_from_world[2],  camera_from_world[3],
                                                             camera_from_world[4], camera_from_world[5],  camera_from_world[6],  camera_from_world[7],
                                                             camera_from_world[8], camera_from_world[9],  camera_from_world[10], camera_from_world[11],
                                                             camera_from_world[12],camera_from_world[13], camera_from_world[14], camera_from_world[15] ) ) );
    const GLfloat* cfw4 = glm::value_ptr( cfw );
    const GLfloat cfw3[9] = { cfw4[0], cfw4[1], cfw4[2],
                             cfw4[4], cfw4[5], cfw4[6],
                             cfw4[8], cfw4[9], cfw4[10] };
*/
    glm::mat3 w_f_m_l( world_from_model[0], world_from_model[1], world_from_model[2],
                       world_from_model[4], world_from_model[5], world_from_model[6],
                       world_from_model[8], world_from_model[9], world_from_model[10] );
    glm::mat3 w_f_m_nm = glm::inverse( w_f_m_l );


    /*


    glm::mat4 w_f_m( world_from_model[0], world_from_model[1],  world_from_model[2],  world_from_model[3],
                     world_from_model[4], world_from_model[5],  world_from_model[6],  world_from_model[7],
                     world_from_model[8], world_from_model[9],  world_from_model[10], world_from_model[11],
                     world_from_model[12],world_from_model[13], world_from_model[14], world_from_model[15]);
    glm::mat4 wfl = glm::transpose( glm::inverse( glm::mat4( world_from_model[0], world_from_model[1],  world_from_model[2],  world_from_model[3],
                                                             world_from_model[4], world_from_model[5],  world_from_model[6],  world_from_model[7],
                                                             world_from_model[8], world_from_model[9],  world_from_model[10], world_from_model[11],
                                                             world_from_model[12],world_from_model[13], world_from_model[14], world_from_model[15] ) ) );
    const GLfloat* wfl4 = glm::value_ptr( wfl );
    const GLfloat world_from_model_nm[9] = { wfl4[0], wfl4[1], wfl4[2],
                                             wfl4[4], wfl4[5], wfl4[6],
                                             wfl4[8], wfl4[9], wfl4[10] };

*/
//    LOGGER_DEBUG( log, glm::to_string(w_f_m_l) );
//    LOGGER_DEBUG( log, glm::to_string(w_f_m_nm) );

    glUseProgram( m_well_prog );
    glPatchParameteri( GL_PATCH_VERTICES, 2 );

    glUniformMatrix4fv( glGetUniformLocation( m_well_prog, "projection" ), 1, GL_FALSE, projection );
    glUniformMatrix4fv( glGetUniformLocation( m_well_prog, "modelview" ), 1, GL_FALSE, camera_from_world );

    glUniformMatrix4fv( glGetUniformLocation( m_well_prog, "world_from_model" ), 1, GL_FALSE, world_from_model );
    glUniformMatrix3fv( glGetUniformLocation( m_well_prog, "world_from_model_nm" ), 1, GL_FALSE, glm::value_ptr(w_f_m_nm) );

    glDrawElements( GL_PATCHES, m_indices.size(), GL_UNSIGNED_INT, NULL );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray( m_attribs_vao );

}


} // of namespace render
