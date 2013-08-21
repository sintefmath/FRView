#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "render/screen/Transparency.hpp"


namespace render {
    namespace screen {


Transparency::~Transparency()
{
}


void
Transparency::renderMiscellaneous( const GLsizei                       width,
                                   const GLsizei                       height,
                                   const GLfloat*                      modelview,
                                   const GLfloat*                      projection,
                                   const std::vector<RenderItem>&      items )
{

/*    for( size_t i=0; i<items.size(); i++ ) {
        if( items[i].m_renderer == RenderItem::RENDERER_WELL ) {
            m_well_renderer->render( width,
                                     height,
                                     projection,
                                     modelview,
                                     glm::value_ptr( m_local_to_world ),
                                     m_wells );
        }
    }
*/    
}



    } // of namespace screen
} // of namespace render
