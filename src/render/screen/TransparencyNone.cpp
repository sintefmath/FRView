#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "render/screen/TransparencyNone.hpp"


namespace render {
    namespace  surface {
        namespace glsl {
            extern const std::string GridTessSurfRenderer_singlepass_fs;
        }
    }
    namespace screen {


TransparencyNone::TransparencyNone()
    : m_surface_renderer( surface::glsl::GridTessSurfRenderer_singlepass_fs )
{}
    
TransparencyNone::~TransparencyNone()
{
}


void
TransparencyNone::render( GLuint                              fbo,
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
        m_width = width;
        m_height = height;
        // resizebuffers
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


    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );
    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );
    
    m_surface_renderer.draw( modelview, projection,
                             m_width, m_height,
                             tess, field, items );
    /*
    surface_renderer->drawSinglePass( modelview,
                            projection,
                            glm::value_ptr( MVP ),
                            nm3,
                            m_width, m_height,
                            tess, field, items );
*/
    
}

    } // of namespace screen
} // of namespace render
