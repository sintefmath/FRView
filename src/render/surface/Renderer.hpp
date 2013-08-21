#pragma once
#include <GL/glew.h>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include "render/RenderItem.hpp"
#include "render/ManagedGL.hpp"

namespace render {
    class GridTess;
    class GridField;
    namespace surface {

    
class Renderer
{
public:
    Renderer( const std::string& fragment_source );
    
    void
    draw( const GLfloat*                                        modelview,
          const GLfloat*                                        projection,
          const GLsizei                                         width,
          const GLsizei                                         height,
          const boost::shared_ptr<const GridTess>               tess,
          const boost::shared_ptr<const GridField>              field,
          const std::vector<RenderItem>&  render_items );

    GLProgram&
    program() { return m_main; }
    
protected:
    GLProgram   m_main;
    GLint       m_loc_mvp;
    GLint       m_loc_mv;
    GLint       m_loc_nm;
    GLint       m_loc_surface_color;
    GLint       m_loc_edge_color;
    GLint       m_loc_screen_size;
};
    

    } // of namespace surface
} // of namespace render
