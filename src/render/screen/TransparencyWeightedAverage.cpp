#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "utils/Logger.hpp"
#include "utils/GLSLTools.hpp"
#include "render/screen/TransparencyWeightedAverage.hpp"


namespace render {
    namespace screen {
        namespace glsl {
            extern const std::string TransparencyWeightedAverage_vs;
            extern const std::string TransparencyWeightedAverage_fs;
        }
        static const std::string package = "render.screen.TransparencyWeightedAverage";

TransparencyWeightedAverage::TransparencyWeightedAverage( const GLsizei width,
                                                          const GLsizei height )
    : m_fbo_solid( package + ".m_fbo_solid" ),
      m_fbo_weighted_average_transparent( package + ".wgt_avg_transparent" ),
      m_solid_color_tex( package + ".m_solid_color" ),
      m_transparent_color_tex( package + ".m_transparent_color" ),
      m_transparent_complexity_tex( package + ".m_transparent_complexity" ),
      m_depth_tex( package + ".m_depth" ),
      m_fsq_vao( package + ".m_fsq_vao" ),
      m_fsq_buf( package + ".m_fsq_buf" ),
      m_merge_passes( package + ".m_merge_passes" )
{
    static const GLfloat quad[ 4*4 ] = {
         1.f, -1.f, 0.f, 1.f,
         1.f,  1.f, 0.f, 1.f,
        -1.f, -1.f, 0.f, 1.f,
        -1.f,  1.f, 0.f, 1.f
    };
    glBindVertexArray( m_fsq_vao.get() );
    glBindBuffer( GL_ARRAY_BUFFER, m_fsq_buf.get() );
    glBufferData( GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 0, NULL );
    glEnableVertexAttribArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );

    resize( width, height );
    buildShaders();
}

    
TransparencyWeightedAverage::~TransparencyWeightedAverage()
{
}

void
TransparencyWeightedAverage::buildShaders()
{
    Logger log = getLogger( package + ".buildShaders" );
    
    GLuint screen_common_vs = utils::compileShader( log, glsl::TransparencyWeightedAverage_vs, GL_VERTEX_SHADER );
    GLuint avg_alpha_fs = utils::compileShader( log, glsl::TransparencyWeightedAverage_fs, GL_FRAGMENT_SHADER );
    glAttachShader( m_merge_passes.get(), screen_common_vs );
    glAttachShader( m_merge_passes.get(), avg_alpha_fs );
    utils::linkProgram( log, m_merge_passes.get() );
    glDeleteShader( screen_common_vs );
    glDeleteShader( avg_alpha_fs );
}


void
TransparencyWeightedAverage::resize( const GLsizei width, const GLsizei height )
{
    Logger log = getLogger( package + ".resize" );
    if( (width > 0) && (height>0) ) {
        m_width = width;
        m_height = height;
    }
    else {
        m_width = 1;
        m_height = 1;
    }
    
    
    GLenum drawbuffers[2] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1
    };

    glBindTexture( GL_TEXTURE_2D, m_depth_tex.get() );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_DEPTH_COMPONENT, m_width, m_height, 0,
                  GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture( GL_TEXTURE_2D, m_solid_color_tex.get() );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_RGBA, m_width, m_height, 0,
                  GL_RGBA, GL_FLOAT, NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture( GL_TEXTURE_2D, m_transparent_color_tex.get() );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_RGBA16F, m_width, m_height, 0,
                  GL_RGBA, GL_FLOAT, NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture( GL_TEXTURE_2D, m_transparent_complexity_tex.get() );
    glTexImage2D( GL_TEXTURE_2D,
                  0, GL_R16F, m_width, m_height, 0,
                  GL_RED, GL_FLOAT, NULL );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture( GL_TEXTURE_2D, 0 );

    glBindFramebuffer( GL_FRAMEBUFFER, m_fbo_solid.get() );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, m_solid_color_tex.get(), 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_2D, m_depth_tex.get(), 0 );
    glDrawBuffers( 1, drawbuffers );
//    glDrawBuffer( GL_COLOR_ATTACHMENT0 );
    utils::checkFBO( log );

    /*
    glBindFramebuffer( GL_FRAMEBUFFER, m_fbo_weighted_sum_transparent );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, m_transparent_color_tex, 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_2D, m_depth_tex, 0 );
    utils::checkFBO( log );
*/

    glBindFramebuffer( GL_FRAMEBUFFER, m_fbo_weighted_average_transparent.get() );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, m_transparent_color_tex.get(), 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                            GL_TEXTURE_2D, m_transparent_complexity_tex.get(), 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_TEXTURE_2D, m_depth_tex.get(), 0 );
    glDrawBuffers( 2, drawbuffers );
//    glDrawBuffer( GL_COLOR_ATTACHMENT0 );
    utils::checkFBO( log );    
}


void
TransparencyWeightedAverage::render( GLuint                              fbo,
                      const GLsizei                       width,
                      const GLsizei                       height,
                      const GLfloat*                      modelview,
                      const GLfloat*                      projection,
                      boost::shared_ptr<const GridTess>   tess,
                      boost::shared_ptr<const GridField>  field,
                      boost::shared_ptr<render::surface::GridTessSurfRenderer>    surface_renderer,
                      const std::vector<render::surface::GridTessSurfRenderer::RenderItem>&      items )
{
    if( (m_width != width) || (m_height != height) ) {
        resize( width, height );
    }
    
    glViewport( 0, 0, m_width, m_height );
    
    glm::mat4 M(modelview[0], modelview[1], modelview[ 2], modelview[3],
                modelview[4], modelview[5], modelview[ 6], modelview[7],
                modelview[8], modelview[9], modelview[10], modelview[11],
                modelview[12], modelview[13], modelview[14], modelview[15] );
    glm::mat4 P(projection[0], projection[1], projection[ 2], projection[3],
                projection[4], projection[5], projection[ 6], projection[7],
                projection[8], projection[9], projection[10], projection[11],
                projection[12], projection[13], projection[14], projection[15] );
    glm::mat4 MVP = P*M;

    glm::mat4 NM = glm::transpose( glm::inverse( M ) );
    const GLfloat* nm4 = glm::value_ptr( NM );
    const GLfloat nm3[9] = { nm4[0], nm4[1], nm4[2],
                             nm4[4], nm4[5], nm4[6],
                             nm4[8], nm4[9], nm4[10] };

    // Weighted average forumla, Bavoil & Myers, 'Order Independent
    // transparency with dual depth peeling', NVIDIA whitepaper.

    glBindFramebuffer( GL_READ_FRAMEBUFFER, fbo );
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_fbo_solid.get() );
    glBlitFramebuffer( 0, 0, m_width, m_height,
                       0, 0, m_width, m_height,
                       GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                       GL_NEAREST );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );
    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );
    surface_renderer->drawTwoPass( modelview,
                                   projection,
                                   glm::value_ptr( MVP ),
                                   nm3,
                                   m_width, m_height,
                                   tess, field, items, true );
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, m_fbo_weighted_average_transparent.get() );
    glClearColor( 0.f, 0.f, 0.f, 0.f );
    glClear( GL_COLOR_BUFFER_BIT );
    glDepthMask( GL_FALSE );
    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );  // alpha * color done in geometry shader
    surface_renderer->drawTwoPass( modelview,
                                   projection,
                                   glm::value_ptr( MVP ),
                                   nm3,
                                   m_width, m_height,
                                   tess, field, items, false );

    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );

    // alpha average pass
    glBindFramebuffer( GL_DRAW_FRAMEBUFFER, fbo );
    glUseProgram( m_merge_passes.get() );
    glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_2D, m_solid_color_tex.get() );
    glActiveTexture( GL_TEXTURE1 ); glBindTexture( GL_TEXTURE_2D, m_transparent_color_tex.get() );
    glActiveTexture( GL_TEXTURE2 ); glBindTexture( GL_TEXTURE_2D, m_transparent_complexity_tex.get() );

    glBindVertexArray( m_fsq_vao.get() );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );    

    glBindVertexArray( 0 );
    glUseProgram( 0 );
    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glActiveTexture( GL_TEXTURE2 ); glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE1 ); glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE0 ); glBindTexture( GL_TEXTURE_2D, 0 );
    glEnable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );
    glDepthMask( GL_TRUE );
}

    } // of namespace screen
} // of namespace render
