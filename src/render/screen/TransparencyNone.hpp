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
            boost::shared_ptr<render::surface::GridTessSurfRenderer>    surface_renderer,
            const std::vector<RenderItem>&      items );    
protected:
    surface::Renderer   m_surface_renderer;
};
    
    
    } // of namespace screen
} // of namespace render
