#pragma once
#include <GL/glew.h>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include "render/surface/GridTessSurfRenderer.hpp"

namespace render {
    class GridTess;
    class GridField;

    namespace screen {

class Transparency : public boost::noncopyable
{
public:
   
    virtual
    ~Transparency();
     
    virtual
    void
    render( GLuint                              fbo,
            const GLsizei                       width,
            const GLsizei                       height,
            const GLfloat*                      modelview,
            const GLfloat*                      projection,
            boost::shared_ptr<const GridTess>   tess,
            boost::shared_ptr<const GridField>  field,
            boost::shared_ptr<render::surface::GridTessSurfRenderer>    surface_renderer,
            const std::vector<RenderItem>&      items ) = 0;
    
    
protected:
    GLsizei     m_width;
    GLsizei     m_height;
    
};
    
    
    } // of namespace screen
} // of namespace render
