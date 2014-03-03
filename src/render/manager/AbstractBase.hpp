/* Copyright STIFTELSEN SINTEF 2013
 * 
 * This file is part of FRView.
 * FRView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * FRView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *  
 * You should have received a copy of the GNU Affero General Public License
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <GL/glew.h>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <render/RenderItem.hpp>
#include <render/wells/Renderer.hpp>
#include <models/Appearance.hpp>

namespace models {
    class Appearance;
}
namespace render {
    class GridTess;
    class GridField;
    
    namespace manager {

class AbstractBase : public boost::noncopyable
{
public:
    AbstractBase( const models::Appearance& appearance,
                  const GLsizei width,
                  const GLsizei height );
    
    virtual
    ~AbstractBase();
     
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
    
    /** Checks if object is compatible with current appearance settings.
     *
     * \returns True if object need to be re-created.
     */
    virtual
    bool
    expired( const models::Appearance& appearance );
    
protected:
    GLsizei                             m_width;
    GLsizei                             m_height;
    models::Appearance::Revision        m_appearance_revision;
    models::Appearance::ShadingModel    m_shading_model;
    
    wells::WellRenderer                 m_well_renderer;

    /** Return GLSL defines deduced from appearance model. */
    virtual
    const std::string
    defines() const;
    
    void
    renderMiscellaneous( const GLsizei                       width,
                         const GLsizei                       height,
                         const GLfloat*                      local_to_world,
                         const GLfloat*                      modelview,
                         const GLfloat*                      projection,
                         const std::vector<RenderItem>&      items );
    
    void
    renderOverlay( const GLsizei                       width,
                   const GLsizei                       height,
                   const GLfloat*                      local_to_world,
                   const GLfloat*                      modelview,
                   const GLfloat*                      projection,
                   const std::vector<RenderItem>&      items );
    
};
    
    
    } // of namespace screen
} // of namespace render
