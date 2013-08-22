#pragma once
#include <GL/glew.h>
#include "render/ManagedGL.hpp"
#include "render/surface/Renderer.hpp"
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
            const GLfloat*                      local_to_world,
            const GLfloat*                      modelview,
            const GLfloat*                      projection,
            boost::shared_ptr<const GridTess>   tess,
            boost::shared_ptr<const GridField>  field,
            const std::vector<RenderItem>&      items );
    
protected:
    surface::Renderer   m_surface_renderer;
    GLint               m_surface_renderer_solid_pass;
    
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
