#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "render/screen/TransparencyNone.hpp"


namespace render {
    namespace screen {
        namespace glsl {
            extern const std::string TransparencyNone_geo_fs;
        }


TransparencyNone::TransparencyNone()
    : m_surface_renderer( glsl::TransparencyNone_geo_fs )
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
                      const std::vector<RenderItem>&      items )
{
    if( (m_width != width) || (m_height != height) ) {
        m_width = width;
        m_height = height;
    }
    
    glViewport( 0, 0, m_width, m_height );

    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );
    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );
    
    m_surface_renderer.draw( modelview, projection,
                             m_width, m_height,
                             tess, field, items );
   
}

    } // of namespace screen
} // of namespace render
