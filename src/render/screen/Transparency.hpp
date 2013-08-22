#pragma once
#include <GL/glew.h>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <render/RenderItem.hpp>
#include <render/wells/Renderer.hpp>

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
            const GLfloat*                      local_to_world,
            const GLfloat*                      modelview,
            const GLfloat*                      projection,
            boost::shared_ptr<const GridTess>   tess,
            boost::shared_ptr<const GridField>  field,
            const std::vector<RenderItem>&      items ) = 0;
    
    
protected:
    GLsizei     m_width;
    GLsizei     m_height;
    
    wells::WellRenderer     m_well_renderer;

    void
    renderMiscellaneous( const GLsizei                       width,
                         const GLsizei                       height,
                         const GLfloat*                      local_to_world,
                         const GLfloat*                      modelview,
                         const GLfloat*                      projection,
                         const std::vector<RenderItem>&      items );

};
    
    
    } // of namespace screen
} // of namespace render
