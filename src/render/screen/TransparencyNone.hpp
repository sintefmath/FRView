#pragma once
#include <GL/glew.h>
#include "render/surface/Renderer.hpp"
#include "render/screen/Transparency.hpp"

namespace render {
    namespace screen {

class TransparencyNone : public Transparency
{
public:
    TransparencyNone();
   
    ~TransparencyNone();

    void
    render( GLuint                              fbo,
            const GLsizei                       width,
            const GLsizei                       height,
            const GLfloat*                      modelview,
            const GLfloat*                      projection,
            boost::shared_ptr<const GridTess>   tess,
            boost::shared_ptr<const GridField>  field,
            const std::vector<RenderItem>&      items );    
protected:
    surface::Renderer   m_surface_renderer;
};
    
    
    } // of namespace screen
} // of namespace render
