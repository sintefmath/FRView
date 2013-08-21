#pragma once
#include <GL/glew.h>
#include "render/ManagedGL.hpp"
#include "render/screen/Transparency.hpp"

namespace render {
    namespace screen {

class TransparencyWeightedAverage : public Transparency
{
public:
    TransparencyWeightedAverage( const GLsizei width, const GLsizei height );
   
    ~TransparencyWeightedAverage();
    
    void
    render( GLuint                              fbo,
            const GLsizei                       width,
            const GLsizei                       height,
            const GLfloat*                      modelview,
            const GLfloat*                      projection,
            boost::shared_ptr<const GridTess>   tess,
            boost::shared_ptr<const GridField>  field,
            boost::shared_ptr<render::surface::GridTessSurfRenderer>    surface_renderer,
            const std::vector<render::surface::GridTessSurfRenderer::RenderItem>&      items );
    
protected:
    GLFramebuffer       m_fbo_solid;
    GLFramebuffer       m_fbo_weighted_average_transparent;
    
    GLTexture           m_solid_color_tex;
    GLTexture           m_transparent_color_tex;
    GLTexture           m_transparent_complexity_tex;
    GLTexture           m_depth_tex;

    GLVertexArrayObject m_fsq_vao;
    GLBuffer            m_fsq_buf;

    GLProgram           m_merge_passes;

    void
    buildShaders();
    
    void
    resize( const GLsizei width, const GLsizei height );

};
    
    
    } // of namespace screen
} // of namespace render
