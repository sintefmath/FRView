/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <string>
#include <limits>
#include <cmath>
#include <glm/glm.hpp>
#include <siut2/gl_utils/GLSLtools.hpp>
#include <siut2/io_utils/snarf.hpp>
#include "Logger.hpp"
#include "GridTessSurfBBoxFinder.hpp"

namespace resources {
    extern const std::string find_aabb_baselevel_v;
    extern const std::string find_aabb_reduction_v;
    extern const std::string find_aabb_baselevel_f;
    extern const std::string find_aabb_reduction_f;
    extern const std::string find_barycenter_baselevel_f;
    extern const std::string find_barycenter_reduction_f;
    extern const std::string find_covariance_baselevel_f;
    extern const std::string find_covariance_reduction_f;
}

/*

  Step 1: Find barycenter:
          sum (1/n)p

  Step 2: Find covariance-matrix, this matrix is hermittian so we only need
          upper triangular part

          a[6] = {0.f}
          w = sqrt( 1.f/n )
          foreach( p ) {
             p = w*(p-barycenter)
             a += { p.x*p.x, p.x*p.y, p.x*p.z, p.y*p.y, p.y*p.z, p.z*p.z }
          }



    // Build least squares problem for plane through origin. Matrix is
    // hermittian, so we only find the upper triangular part.
    float a[6] = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
    float c[6] = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };

    float w = sqrtf( 1.f/n );               // Normalization to improve accuracy
    for( Iterator i=first; i!=last; i++) {
        glm::vec3 p = w*( (*i) - barycenter);

        float y[6] = {
            p.x*p.x, p.x*p.y, p.x*p.z, p.y*p.y, p.y*p.z, p.z*p.z
        };
        for( unsigned int k=0; k<6; k++ ) { // Kahan compensated summation
            y[k] -= c[k];                   // add previous error
            float s = a[k] + y[k];          // do summation
            c[k] = (s-a[k]) - y[k];         // recreate y and determine error
            a[k] = s;                       // update sum
        }
    }
    leastSquaresFitPlaneCore( normal,
                              a[0], a[1], a[2],
                              a[3], a[4],
                              a[5] );
}

float
newtonsMethod( float r,
               const float c2,
               const float c1,
               const float c0 )
{
    for(unsigned int i=0; i<15; i++) {
        float f    = c1*r +     c2*r*r  +     r*r*r + c0;
        float df   = c1   + 2.f*c2*r    + 3.f*r*r ;
        float ddf  =             2.f*c2 + 6.f*r;
        float dddf =                      6.f;
        float d;
        if( fabsf(df)>std::numeric_limits<float>::epsilon() ) {
            d = f/df;
        }
        else if( fabsf(ddf)>std::numeric_limits<float>::epsilon() ) {
            d = df/ddf;
        }
        else {
            d = ddf/dddf;
        }
        r = r - d;
    }
    return r;
}


float
leastSquaresFitPlaneCore( glm::vec3&       direction,
                      const float a00, const float a01,  const float a02,
                      const float a11, const float a12,
                      const float a22 )
{


*/


using siut2::gl_utils::compileShader;
using siut2::gl_utils::linkProgram;
static const std::string package = "GridTessSurfBBoxFinder";

GridTessSurfBBoxFinder::GridTessSurfBBoxFinder()
    : m_size_l2( 4 ),
      m_size( 1<<m_size_l2 )
{

    GLenum drawbuffers2[2] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1
    };

    Logger log = getLogger( package + ".GridTessSurfBBoxFinder" );
    LOGGER_DEBUG( log, "size=" << m_size << ", log2(size)=" << m_size_l2 );

    // set up GPGPU quad
    static const GLfloat quad[ 4*4 ] = {
         1.f, -1.f, 0.f, 1.f,
         1.f,  1.f, 0.f, 1.f,
        -1.f, -1.f, 0.f, 1.f,
        -1.f,  1.f, 0.f, 1.f
    };
    glGenVertexArrays( 1, &m_gpgpu_quad_vertex_array );
    glBindVertexArray( m_gpgpu_quad_vertex_array );
    glGenBuffers( 1, &m_gpgpu_quad_buffer );
    glBindBuffer( GL_ARRAY_BUFFER, m_gpgpu_quad_buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 0, NULL );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );
    CHECK_GL;

    // --- barycenter reduction
    glGenTextures( 1, &m_barycenter_reduction_tex );
    glBindTexture( GL_TEXTURE_2D, m_barycenter_reduction_tex );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_RGB32F, m_size, m_size, 0,
                  GL_RGB, GL_FLOAT, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, 0 );
    m_barycenter_reduction_fbo.resize( m_size_l2 + 1 );
    glGenFramebuffers( m_barycenter_reduction_fbo.size(), m_barycenter_reduction_fbo.data() );
    for( GLsizei i=0; i<=m_size_l2; i++ ) {
        glBindFramebuffer( GL_FRAMEBUFFER, m_barycenter_reduction_fbo[i] );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_barycenter_reduction_tex, i );
        glDrawBuffer( GL_COLOR_ATTACHMENT0 );
        CHECK_FBO;
    }
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    CHECK_GL;


    // --- covariance reduction
    glGenTextures( 1, &m_covariance_reduction_0_tex );
    glBindTexture( GL_TEXTURE_2D, m_covariance_reduction_0_tex );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_RGB32F, m_size, m_size, 0,
                  GL_RGB, GL_FLOAT, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap( GL_TEXTURE_2D );
    glGenTextures( 1, &m_covariance_reduction_1_tex );
    glBindTexture( GL_TEXTURE_2D, m_covariance_reduction_1_tex );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_RGB32F, m_size, m_size, 0,
                  GL_RGB, GL_FLOAT, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, 0 );
    m_covariance_reduction_fbo.resize( m_size_l2 + 1 );
    glGenFramebuffers( m_covariance_reduction_fbo.size(), m_covariance_reduction_fbo.data() );
    for( GLsizei i=0; i<=m_size_l2; i++ ) {
        glBindFramebuffer( GL_FRAMEBUFFER, m_covariance_reduction_fbo[i] );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_covariance_reduction_0_tex, i );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_covariance_reduction_1_tex, i );
        glDrawBuffers( 2, drawbuffers2 );
        CHECK_FBO;
    }
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    CHECK_GL;

    // --- aabbox reduction
    glGenTextures( 1, &m_aabbox_reduction_min_tex );
    glBindTexture( GL_TEXTURE_2D, m_aabbox_reduction_min_tex );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_RGB32F, m_size, m_size, 0,
                  GL_RGB, GL_FLOAT, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap( GL_TEXTURE_2D );
    glGenTextures( 1, &m_aabbox_reduction_max_tex );
    glBindTexture( GL_TEXTURE_2D, m_aabbox_reduction_max_tex );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_RGB32F, m_size, m_size, 0,
                  GL_RGB, GL_FLOAT, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, 0 );
    m_aabbox_reduction_fbo.resize( m_size_l2 + 1 );
    glGenFramebuffers( m_aabbox_reduction_fbo.size(), m_aabbox_reduction_fbo.data() );
    for( GLsizei i=0; i<=m_size_l2; i++ ) {
        glBindFramebuffer( GL_FRAMEBUFFER, m_aabbox_reduction_fbo[i] );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_aabbox_reduction_min_tex, i );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_aabbox_reduction_max_tex, i );
        glDrawBuffers( 2, drawbuffers2 );
        CHECK_FBO;
    }
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    CHECK_GL;

    GLuint baselevel_v = compileShader( resources::find_aabb_baselevel_v, GL_VERTEX_SHADER );
    GLuint reduction_v = compileShader( resources::find_aabb_reduction_v, GL_VERTEX_SHADER );

    GLuint aabb_baselevel_f = compileShader( resources::find_aabb_baselevel_f, GL_FRAGMENT_SHADER );
    m_aabbox_baselevel_prog = glCreateProgram();
    glAttachShader( m_aabbox_baselevel_prog, baselevel_v );
    glAttachShader( m_aabbox_baselevel_prog, aabb_baselevel_f );
    linkProgram( m_aabbox_baselevel_prog );
    glDeleteShader( aabb_baselevel_f );
    glUseProgram( m_aabbox_baselevel_prog );
    glUniform1i( glGetUniformLocation( m_aabbox_baselevel_prog, "size_l2"), m_size_l2 );
    glUniform1i( glGetUniformLocation( m_aabbox_baselevel_prog, "size_mask" ), (1<<m_size_l2)-1 );
    glUniform1f( glGetUniformLocation( m_aabbox_baselevel_prog, "scale"), 2.f/m_size );
    glUseProgram( 0 );

    GLuint aabb_reduction_f = compileShader( resources::find_aabb_reduction_f, GL_FRAGMENT_SHADER );
    m_aabbox_reduction_prog = glCreateProgram();
    glAttachShader( m_aabbox_reduction_prog, reduction_v );
    glAttachShader( m_aabbox_reduction_prog, aabb_reduction_f );
    linkProgram( m_aabbox_reduction_prog );
    glDeleteShader( aabb_reduction_f );

    GLuint barycenter_baselevel_f = compileShader( resources::find_barycenter_baselevel_f, GL_FRAGMENT_SHADER );
    m_barycenter_baselevel_prog = glCreateProgram();
    glAttachShader( m_barycenter_baselevel_prog, baselevel_v );
    glAttachShader( m_barycenter_baselevel_prog, barycenter_baselevel_f );
    linkProgram( m_barycenter_baselevel_prog );
    glDeleteShader( barycenter_baselevel_f );
    glUseProgram( m_barycenter_baselevel_prog );
    glUniform1i( glGetUniformLocation( m_barycenter_baselevel_prog, "size_l2"), m_size_l2 );
    glUniform1i( glGetUniformLocation( m_barycenter_baselevel_prog, "size_mask" ), (1<<m_size_l2)-1 );
    glUniform1f( glGetUniformLocation( m_barycenter_baselevel_prog, "scale"), 2.f/m_size );
    glUseProgram( 0 );


    GLuint barycenter_reduction_f = compileShader( resources::find_barycenter_reduction_f, GL_FRAGMENT_SHADER );
    m_barycenter_reduction_prog = glCreateProgram();
    glAttachShader( m_barycenter_reduction_prog, reduction_v );
    glAttachShader( m_barycenter_reduction_prog, barycenter_reduction_f );
    linkProgram( m_barycenter_reduction_prog );
    glDeleteShader( barycenter_reduction_f );

    GLuint covariance_baselevel_f = compileShader( resources::find_covariance_baselevel_f, GL_FRAGMENT_SHADER );
    m_covariance_baselevel_prog = glCreateProgram();
    glAttachShader( m_covariance_baselevel_prog, baselevel_v );
    glAttachShader( m_covariance_baselevel_prog, covariance_baselevel_f );
    linkProgram( m_covariance_baselevel_prog );
    glDeleteShader( covariance_baselevel_f );
    glUseProgram( m_covariance_baselevel_prog );
    glUniform1i( glGetUniformLocation( m_covariance_baselevel_prog, "size_l2"), m_size_l2 );
    glUniform1i( glGetUniformLocation( m_covariance_baselevel_prog, "size_mask" ), (1<<m_size_l2)-1 );
    glUniform1f( glGetUniformLocation( m_covariance_baselevel_prog, "scale"), 2.f/m_size );
    glUseProgram( 0 );

    GLuint covariance_reduction_f = compileShader( resources::find_covariance_reduction_f, GL_FRAGMENT_SHADER );
    m_covariance_reduction_prog = glCreateProgram();
    glAttachShader( m_covariance_reduction_prog, reduction_v );
    glAttachShader( m_covariance_reduction_prog, covariance_reduction_f );
    linkProgram( m_covariance_reduction_prog );
    glDeleteShader( covariance_reduction_f );


    glDeleteShader( baselevel_v );
    glDeleteShader( reduction_v );

    // Performance queries
    glGenQueries( 1, &m_aabbox_baselevel_query );
    glGenQueries( 1, &m_aabbox_reduction_query );
    glGenQueries( 1, &m_barycenter_baselevel_query );
    glGenQueries( 1, &m_barycenter_reduction_query );
    glGenQueries( 1, &m_covariance_baselevel_query );
    glGenQueries( 1, &m_covariance_reduction_query );
}

GridTessSurfBBoxFinder::~GridTessSurfBBoxFinder()
{
    glDeleteVertexArrays( 1, &m_gpgpu_quad_vertex_array );
    glDeleteBuffers( 1, &m_gpgpu_quad_buffer );
    glDeleteFramebuffers( m_aabbox_reduction_fbo.size(), m_aabbox_reduction_fbo.data() );
    glDeleteTextures( 1, &m_aabbox_reduction_min_tex );
    glDeleteTextures( 1, &m_aabbox_reduction_max_tex );
    glDeleteProgram( m_aabbox_baselevel_prog );
    glDeleteProgram( m_aabbox_reduction_prog );
    glDeleteProgram( m_barycenter_baselevel_prog );
    glDeleteProgram( m_barycenter_reduction_prog );
    glDeleteProgram( m_covariance_baselevel_prog );
    glDeleteProgram( m_covariance_reduction_prog );
    glDeleteQueries( 1, &m_aabbox_baselevel_query );
    glDeleteQueries( 1, &m_aabbox_reduction_query );
    glDeleteQueries( 1, &m_barycenter_baselevel_query );
    glDeleteQueries( 1, &m_barycenter_reduction_query );
    glDeleteQueries( 1, &m_covariance_baselevel_query );
    glDeleteQueries( 1, &m_covariance_reduction_query );
    CHECK_GL;
}

void
GridTessSurfBBoxFinder::find( float                (&transform)[16],
                              float                (&aabb_min)[3],
                              float                (&aabb_max)[3],
                              const GridTess*      tess,
                              const GridTessSurf*  surf )
{
    static const GLfloat huge = std::numeric_limits<float>::max();

    Logger log = getLogger( package + ".find" );
    GLfloat clear_sum[4] = { 0.f, 0.f, 0.f, 0.f };
    GLfloat clear_min[4] = { huge, huge, huge, huge };
    GLfloat clear_max[4] = { -huge, -huge, -huge, -huge };
    GLsizei baselevel_passes = (surf->triangles() + (m_size*m_size-1))/(m_size*m_size);
    LOGGER_DEBUG( log, "baselevel_passes=" << baselevel_passes );
    glPointSize( 1.f );

    // --- barycenter
    glBeginQuery( GL_TIME_ELAPSED, m_barycenter_baselevel_query );
    glBindFramebuffer( GL_FRAMEBUFFER, m_barycenter_reduction_fbo[ 0 ] );
    glViewport( 0, 0, m_size, m_size );
    glClearBufferfv( GL_COLOR, 0, clear_sum );
    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );
    glBlendEquation( GL_FUNC_ADD );
    glUseProgram( m_barycenter_baselevel_prog );
    glUniform1f( glGetUniformLocation( m_barycenter_baselevel_prog, "weight" ), 1.f/(3.f*surf->triangles()) );
    glBindVertexArray( tess->vertexVertexArrayObject() );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, surf->indexBuffer() );
    glDrawElements( GL_POINTS, 3*surf->triangles(), GL_UNSIGNED_INT, NULL );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glDisable( GL_BLEND );
    glEndQuery( GL_TIME_ELAPSED );
    CHECK_FBO;
    CHECK_GL;

    glBeginQuery( GL_TIME_ELAPSED, m_barycenter_reduction_query );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, m_barycenter_reduction_tex );
    glUseProgram( m_barycenter_reduction_prog );
    glBindVertexArray( m_gpgpu_quad_vertex_array );
    for( GLsizei i=1; i<=m_size_l2; i++ ) {
        glBindFramebuffer( GL_FRAMEBUFFER, m_barycenter_reduction_fbo[i] );
        glViewport( 0, 0, m_size>>i, m_size>>i );
        glUniform1i( glGetUniformLocation( m_barycenter_reduction_prog, "level" ), i-1 );
        glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    }
    glEndQuery( GL_TIME_ELAPSED );
    CHECK_FBO;
    CHECK_GL;

    GLfloat barycenter[3];
    GLfloat moo[3*m_size*m_size];
    glGetTexImage( GL_TEXTURE_2D, m_size_l2, GL_RGB, GL_FLOAT, barycenter );
    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, moo );
    glBindTexture( GL_TEXTURE_2D, 0 );

    GLfloat boo[3] = {0.f, 0.f, 0.f };
    for( int j=0; j<m_size*m_size; j++ ) {
        for(int i=0; i<3; i++) {
            boo[i] += moo[3*j+i];
        }
    }

    LOGGER_DEBUG( log, "moo = [" << boo[0] << ", " << boo[1] << ", " << boo[2] << "] " );

    LOGGER_DEBUG( log, "barycenter = [" << barycenter[0] << ", " << barycenter[1] << ", " << barycenter[2] << "]");

    // --- covariance matrix
    glBeginQuery( GL_TIME_ELAPSED, m_covariance_baselevel_query );
    glBindFramebuffer( GL_FRAMEBUFFER, m_covariance_reduction_fbo[ 0 ] );
    glViewport( 0, 0, m_size, m_size );
    glClearBufferfv( GL_COLOR, 0, clear_sum );
    glClearBufferfv( GL_COLOR, 1, clear_sum );
    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );
    glBlendEquation( GL_FUNC_ADD );
    glUseProgram( m_covariance_baselevel_prog );
    glUniform3f( glGetUniformLocation( m_covariance_baselevel_prog, "barycenter" ),
                 barycenter[0],
                 barycenter[1],
                 barycenter[2] );
    glUniform1f( glGetUniformLocation( m_covariance_baselevel_prog, "weight" ), sqrtf(1.f/(3.f*surf->triangles())) );
    glBindVertexArray( tess->vertexVertexArrayObject() );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, surf->indexBuffer() );
    glDrawElements( GL_POINTS, 3*surf->triangles(), GL_UNSIGNED_INT, NULL );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBlendEquationi( 0, GL_FUNC_ADD );
    glBlendEquationi( 1, GL_FUNC_ADD );
    glDisable( GL_BLEND );
    glEndQuery( GL_TIME_ELAPSED );
    CHECK_FBO;
    CHECK_GL;
    glBeginQuery( GL_TIME_ELAPSED, m_covariance_reduction_query );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, m_covariance_reduction_0_tex );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, m_covariance_reduction_1_tex );
    glUseProgram( m_covariance_reduction_prog );
    glBindVertexArray( m_gpgpu_quad_vertex_array );
    for( GLsizei i=1; i<=m_size_l2; i++ ) {
        glBindFramebuffer( GL_FRAMEBUFFER, m_covariance_reduction_fbo[i] );
        glViewport( 0, 0, m_size>>i, m_size>>i );
        glUniform1i( glGetUniformLocation( m_covariance_reduction_prog, "level" ), i-1 );
        glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    }
    glEndQuery( GL_TIME_ELAPSED );

    GLfloat cov0[3];
    GLfloat cov1[3];
    glActiveTexture( GL_TEXTURE1 );
    glGetTexImage( GL_TEXTURE_2D, m_size_l2, GL_RGB, GL_FLOAT, cov1);
    glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE0 );
    glGetTexImage( GL_TEXTURE_2D, m_size_l2, GL_RGB, GL_FLOAT, cov0 );
    glBindTexture( GL_TEXTURE_2D, 0 );

    float scale = std::max( fabsf(cov0[0]),
                            std::max( fabsf(cov0[1]),
                                      std::max( fabsf(cov0[2]),
                                                std::max( fabsf(cov1[0]),
                                                          std::max( fabsf( cov1[1]),
                                                                    fabsf( cov1[2] ) ) ) ) ) );
    cov0[0] = cov0[0]/scale;
    cov0[1] = cov0[1]/scale;
    cov0[2] = cov0[2]/scale;
    cov1[0] = cov1[0]/scale;
    cov1[1] = cov1[1]/scale;
    cov1[2] = cov1[2]/scale;

    LOGGER_DEBUG( log, "cov = { {"
                  << cov0[0] << ", "
                  << cov0[1] << ", "
                  << cov0[2] << "}, {"
                  << cov0[1] << ", "
                  << cov1[0] << ", "
                  << cov1[1] << "}, {"
                  << cov0[2] << ", "
                  << cov1[1] << ", "
                  << cov1[2] << "} }" );

    double r0, r1, r2;
    eigenValues( r0, r1, r2,
                 cov0[0], cov0[1], cov0[2],
                 cov1[0], cov1[1],
                 cov1[2] );
    LOGGER_DEBUG( log, "roots={" << r0 << ", " << r1 << ", " << r2 << "}" );




    float u[3] = { 1.f, 0.f, 0.f };
    float v[3] = { 0.f, 1.f, 0.f };
    float w[3] = { 0.f, 0.f, 1.f };
    if( (0.9*r0 > r1) && (0.9*r1 > r2 ) ) {
        eigenVector( u, r0,
                     cov0[0], cov0[1], cov0[2],
                     cov1[0], cov1[1],
                     cov1[2] );
        eigenVector( v, r1,
                     cov0[0], cov0[1], cov0[2],
                     cov1[0], cov1[1],
                     cov1[2] );
        w[0] = u[1]*v[2]-u[2]*v[1];
        w[1] = u[2]*v[0]-u[0]*v[2];
        w[2] = u[0]*v[1]-u[1]*v[0];
    }
    transform[ 0  ] = u[0];
    transform[ 1  ] = u[1];
    transform[ 2  ] = u[2];
    transform[ 3  ] = 0.f;

    transform[ 4  ] = v[0];
    transform[ 5  ] = v[1];
    transform[ 6  ] = v[2];
    transform[ 7  ] = 0.f;

    transform[ 8  ] = w[0];
    transform[ 9  ] = w[1];
    transform[ 10 ] = w[2];
    transform[ 11 ] = 0.f;

    transform[ 12 ] = barycenter[0];
    transform[ 13 ] = barycenter[1];
    transform[ 14 ] = barycenter[2];
    transform[ 15 ] = 1.f;

    GLfloat inv_transform[16];
    inv_transform[ 0  ] = u[0];
    inv_transform[ 1  ] = v[0];
    inv_transform[ 2  ] = w[0];
    inv_transform[ 3  ] = 0.f;

    inv_transform[ 4  ] = u[1];
    inv_transform[ 5  ] = v[1];
    inv_transform[ 6  ] = w[1];
    inv_transform[ 7  ] = 0.f;

    inv_transform[ 8  ] = u[2];
    inv_transform[ 9  ] = v[2];
    inv_transform[ 10 ] = w[2];
    inv_transform[ 11 ] = 0.f;

    inv_transform[ 12 ] = -(u[0]*barycenter[0] + u[1]*barycenter[1] + u[2]*barycenter[2]);
    inv_transform[ 13 ] = -(v[0]*barycenter[0] + v[1]*barycenter[1] + v[2]*barycenter[2]);
    inv_transform[ 14 ] = -(w[0]*barycenter[0] + w[1]*barycenter[1] + w[2]*barycenter[2]);
    inv_transform[ 15 ] = 1.f;


    if( (r0 > r1) && (r1 > 0.0) ) {
        float v0[3], v1[3], v2[3];
        eigenVector( v0, r0,
                     cov0[0], cov0[1], cov0[2],
                     cov1[0], cov1[1],
                     cov1[2] );
        eigenVector( v1, r1,
                     cov0[0], cov0[1], cov0[2],
                     cov1[0], cov1[1],
                     cov1[2] );
        eigenVector( v2, r2,
                     cov0[0], cov0[1], cov0[2],
                     cov1[0], cov1[1],
                     cov1[2] );
        LOGGER_DEBUG( log, "v0=[" << v0[0] << ", " << v0[1] << ", " << v0[2] << "]" );
        LOGGER_DEBUG( log, "v1=[" << v1[0] << ", " << v1[1] << ", " << v1[2] << "]" );
        LOGGER_DEBUG( log, "v2=[" << v2[0] << ", " << v2[1] << ", " << v2[2] << "]" );
    }

    /*

*/

/*
    solveCubicWithThreeNonNegativeRoots( r0, r1, r2, c2, c1, c0 );




    return r2;
}
*/


    // --- axis-aligned bounding box
    glBeginQuery( GL_TIME_ELAPSED, m_aabbox_baselevel_query );
    glBindFramebuffer( GL_FRAMEBUFFER, m_aabbox_reduction_fbo[ 0 ] );
    glViewport( 0, 0, m_size, m_size );
    glClearBufferfv( GL_COLOR, 0, clear_min );
    glClearBufferfv( GL_COLOR, 1, clear_max );
    glEnable( GL_BLEND );
    glBlendEquationi( 0, GL_MIN );
    glBlendEquationi( 1, GL_MAX );
    glUseProgram( m_aabbox_baselevel_prog );
    glUniformMatrix4fv( glGetUniformLocation( m_aabbox_baselevel_prog, "inv_transform"),
                        1, GL_FALSE, inv_transform );
    glBindVertexArray( tess->vertexVertexArrayObject() );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, surf->indexBuffer() );
    glDrawElements( GL_POINTS, 3*surf->triangles(), GL_UNSIGNED_INT, NULL );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBlendEquationi( 0, GL_FUNC_ADD );
    glBlendEquationi( 1, GL_FUNC_ADD );
    glDisable( GL_BLEND );
    glEndQuery( GL_TIME_ELAPSED );
    CHECK_FBO;
    CHECK_GL;
    glBeginQuery( GL_TIME_ELAPSED, m_aabbox_reduction_query );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, m_aabbox_reduction_min_tex );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, m_aabbox_reduction_max_tex );
    glUseProgram( m_aabbox_reduction_prog );
    glBindVertexArray( m_gpgpu_quad_vertex_array );
    for( GLsizei i=1; i<=m_size_l2; i++ ) {
        glBindFramebuffer( GL_FRAMEBUFFER, m_aabbox_reduction_fbo[i] );
        glViewport( 0, 0, m_size>>i, m_size>>i );
        glUniform1i( glGetUniformLocation( m_aabbox_reduction_prog, "level" ), i-1 );
        glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    }
    glEndQuery( GL_TIME_ELAPSED );
    // ----

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glUseProgram( 0 );
    glBindVertexArray( 0 );


    glActiveTexture( GL_TEXTURE1 );
    glGetTexImage( GL_TEXTURE_2D, m_size_l2, GL_RGB, GL_FLOAT, aabb_max );
    glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE0 );
    glGetTexImage( GL_TEXTURE_2D, m_size_l2, GL_RGB, GL_FLOAT, aabb_min );
    glBindTexture( GL_TEXTURE_2D, 0 );

    LOGGER_DEBUG( log, "bb: ["
                  << (aabb_min[0]-barycenter[0] ) << ", "
                  << (aabb_min[1]-barycenter[1] ) << ", "
                  << (aabb_min[2]-barycenter[2] ) << "] X ["
                  << (aabb_max[0]-barycenter[0] ) << ", "
                  << (aabb_max[1]-barycenter[1] ) << ", "
                  << (aabb_max[2]-barycenter[2] ) << "]"
                  );

    LOGGER_DEBUG( log, "bb: ["
                  << (aabb_max[0]-aabb_min[0] ) << ", "
                  << (aabb_max[1]-aabb_min[1] ) << ", "
                  << (aabb_max[2]-aabb_min[2] ) << "] "
                  );

    GLuint64 baselevel_time, reduction_time;
    glGetQueryObjectui64v( m_aabbox_baselevel_query, GL_QUERY_RESULT, &baselevel_time );
    glGetQueryObjectui64v( m_aabbox_reduction_query, GL_QUERY_RESULT, &reduction_time );

    LOGGER_DEBUG( log, "tris=" << surf->triangles()
                  << ", baselevel=" << ((1e-6)*baselevel_time)
                  << "ms, reduction=" << ((1e-6)*reduction_time)
                  << "ms, total=" << ((1e-6)*(baselevel_time+reduction_time))
                  << "ms");
    CHECK_GL;

}


void
GridTessSurfBBoxFinder::eigenValues( double& r0, double& r1, double& r2,
                                     const double a00, const double a01, const double a02,
                                     const double a11, const double a12,
                                     const double a22 )
{
    double c2 = -(a00 + a11 + a22); // always negative
    double c1 = a00*a11
            + a00*a22
            + a11*a22
            - a01*a01
            - a02*a02
            - a12*a12;
    double c0 = a00*a12*a12
            + a11*a02*a02
            + a22*a01*a01
            - a00*a11*a22
            - 2.0*a02*a01*a12;

    double p = c2*c2 - 3.0*c1;
    double q = c2*( (3.0/2.0)*c1 - p) - (27.0/2.0)*c0;
    double num2 = 27.0*( 0.25*c1*c1*(p-c1) + c0*( q + (27.0/4.0)*c0 ) );
    double num = sqrtf( fabsf(num2) );
    double theta = (1.0/3.0)*atan2( num, q );
    double sqrt_p = sqrt( fabs(p) );
    double c = sqrt_p*cos( theta );
    double s = sqrt_p*sqrt(3.0)*sinf( theta );
    r0 = (1.0/3.0)*(2.0*c - c2);
    r1 = (1.0/3.0)*(   -c + s - c2);
    r2 = (1.0/3.0)*(   -c - s - c2);
    if( r0 < r1 ) {
        std::swap( r0, r1 );
    }
    if( r1 < r2 ) {
        std::swap( r1, r2 );
    }
    if( r0 < r1 ) {
        std::swap( r0, r1 );
    }
}

void
GridTessSurfBBoxFinder::eigenVector( float (&v)[3], const double r,
                                     const double a00, const double a01, const double a02,
                                     const double a11, const double a12,
                                     const double a22 )
{
    glm::dvec3 e1 = glm::dvec3( a00, a01, a02 ) - glm::dvec3( r, 0.f, 0.f );
    glm::dvec3 e2 = glm::dvec3( a01, a11, a12 ) - glm::dvec3( 0.f, r, 0.f );

    /*
    glm::dvec3 E1 = glm::normalize( e1 );
    glm::dvec3 E2 = glm::normalize( e2 );
    if( glm::dot(E1,E1) > 0.0 && glm::dot(E2,E2) && fabs(glm::dot(E1,E2) ) < 0.9 ) {
        glm::dvec3 d = glm::normalize( glm::cross( E1, E2) );
        v[0] = d.x;
        v[1] = d.y;
        v[2] = d.z;
    }
    else {
        double mu = glm::length( e1 )/glm::length(e2);
        double g = 1.0/sqrt( 1.0+mu*mu);
        v[0] = g;
        v[1] = -g*mu;
        v[2] = 0.0;
    }

*/


    glm::dvec3 e3 = glm::dvec3( a02, a12, a22 ) - glm::dvec3( 0.f, 0.f, r );




    double mi_12 = glm::dot(e1,e2)/glm::dot(e1,e1);
    double mu_12 = glm::dot(e1,e2)/glm::dot(e2,e2);
    double co_12 = mu_12*mi_12;
    double mu_13 = glm::dot(e1,e3)/glm::dot(e3,e3);
    double mi_13 = glm::dot(e1,e3)/glm::dot(e1,e1);
    double co_13 = mu_13*mi_13;
    double mu_23 = glm::dot(e2,e3)/glm::dot(e3,e3);
    double mi_23 = glm::dot(e2,e3)/glm::dot(e2,e2);
    double co_23 = mu_23*mi_23;
    const double alt_threshold = 0.999999f;



    glm::dvec3 dir;
    if( (co_12 < co_13) && (co_12 < co_23) ) {
        if( co_12 < alt_threshold ) {
            dir = glm::normalize( glm::cross( e1, e2 ) );
        }
        else {
            dir = (1.0/sqrt(1.0+mu_12*mu_12))*glm::dvec3( 1.0, -mu_12, 0.0 );
        }
    }
    else if( co_13 < co_23 ) {
        if( co_13 < alt_threshold ) {
            dir = glm::normalize( glm::cross( e1, e3 ) );
        }
        else {
            dir = (1.f/sqrt(1.f+mu_13*mu_13))*glm::dvec3( 1.0, 0.0, -mu_13 );
        }
    }
    else {
        if( co_23 < alt_threshold ) {
            dir = glm::normalize( glm::cross( e2, e3 ) );
        }
        else {
            dir = (1.f/sqrt(1.f+mu_23*mu_23))*glm::dvec3( 0.0, 1.0, -mu_23 );
        }
    }
    v[0] = dir.x;
    v[1] = dir.y;
    v[2] = dir.z;
}
