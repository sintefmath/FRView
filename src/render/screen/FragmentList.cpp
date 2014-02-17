/* Copyright STIFTELSEN SINTEF 2014
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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "utils/Logger.hpp"
#include "render/screen/FragmentList.hpp"

namespace render {
    namespace screen {
        namespace glsl {
            extern const std::string FragmentList_geo_fs;
        }
        static const std::string package = "render.screen.FragmentList";


FragmentList::FragmentList(const GLsizei width, const GLsizei height)
    : m_surface_renderer( glsl::FragmentList_geo_fs )
{
    Logger log = getLogger( package + ".constructor" );
    m_width = width;
    m_height = height;
    
    GLint major=0;
    GLint minor=0;
    glGetIntegerv( GL_MAJOR_VERSION, &major );
    glGetIntegerv( GL_MINOR_VERSION, &minor );

    // Atomic counter buffer requires 4.2
    

    glGetIntegerv( GL_MAX_TEXTURE_BUFFER_SIZE, &m_texbuf_max_texels );
    LOGGER_DEBUG( log, "GL_MAX_TEXTURE_BUFFER_SIZE=" << m_texbuf_max_texels );


    m_fragment_alloc_loc = glGetUniformLocation( m_surface_renderer.program().get(),
                                                 "fragment_alloc" );
    
    glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, m_fragment_counter_buf.get() );
    glBufferData( GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, 0 );

    m_fragment_alloc = 10000;
    resizeFragmentBuffers();
    resizeScreenSizedBuffers();

    
}
    
FragmentList::~FragmentList()
{
}


void
FragmentList::render( GLuint                              fbo,
                          const GLsizei                       width,
                          const GLsizei                       height,
                          const GLfloat*                      local_to_world,
                          const GLfloat*                      modelview,
                          const GLfloat*                      projection,
                          boost::shared_ptr<const GridTess>   tess,
                          boost::shared_ptr<const GridField>  field,
                          const std::vector<RenderItem>&      items )
{
    Logger log = getLogger( package + ".render" );
    if(  (m_width != width) || (m_height != height) ) {
        m_width = width;
        m_height = height;
        resizeScreenSizedBuffers();
    }
    

    GLint ost = 42;
    glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, m_fragment_counter_buf.get() );
    glBufferData( GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &ost, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, 0 );
    

    bool done;
    do {
        done = true;

        // init fragment list        
        glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, m_fragment_counter_buf.get() );
        glClearBufferData( GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, NULL );

        glBindFramebuffer( GL_FRAMEBUFFER, m_fragment_head_fbo.get() );
        GLint clear[4] = { -1, -1, -1, -1 };
        glClearBufferiv( GL_COLOR, 0, clear );
        glBindFramebuffer( GL_FRAMEBUFFER, 0 );
        // glClearTexImage is 4.4

        
        glProgramUniform1i( m_surface_renderer.program().get(),
                            m_fragment_alloc_loc,
                            m_fragment_alloc );
        
        glBindBufferBase( GL_ATOMIC_COUNTER_BUFFER, 0, m_fragment_counter_buf.get() );

        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_BUFFER, m_fragment_rgba_tex.get() );

        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_BUFFER, m_fragment_depth_tex.get() );
        
        glActiveTexture( GL_TEXTURE2 );
        glBindTexture( GL_TEXTURE_BUFFER, m_fragment_next_tex.get() );

        glActiveTexture( GL_TEXTURE3 );
        glBindTexture( GL_TEXTURE_2D, m_fragment_head_buf.get() );

        
        glm::mat4 M = glm::make_mat4( modelview ) * glm::make_mat4( local_to_world );
        
        glViewport( 0, 0, m_width, m_height );
        
        glBindFramebuffer( GL_FRAMEBUFFER, fbo );
        
        glEnable( GL_DEPTH_TEST );
        glDepthFunc( GL_LESS );
        glDepthMask( GL_TRUE );
        glDisable( GL_BLEND );
        
        m_surface_renderer.draw( glm::value_ptr( M ), projection,
                                 m_width, m_height,
                                 tess, field, items );
        renderMiscellaneous( width, height,
                             local_to_world, modelview, projection,
                             items );

        
        GLint fragments = 0;
        glGetBufferSubData( GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLint), &fragments );
        if( (m_fragment_alloc < fragments) && (m_fragment_alloc < m_texbuf_max_texels) ) {
            m_fragment_alloc = std::min( (m_texbuf_max_texels+(m_texbuf_max_texels>>4)), fragments );
            resizeFragmentBuffers();
            done = false;
        }
    } while(!done);
    glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, 0 );
}

void
FragmentList::resizeFragmentBuffers( )
{
    Logger log = getLogger( package + ".resizeFragmentBuffers" );
    LOGGER_DEBUG( log, "resized fragment buffers to hold " << m_fragment_alloc << " fragments." );

    glBindBuffer( GL_TEXTURE_BUFFER, m_fragment_rgba_buf.get() );
    glBufferData( GL_TEXTURE_BUFFER, 4*sizeof(GLfloat)*m_fragment_alloc, NULL, GL_DYNAMIC_DRAW );
    glBindTexture( GL_TEXTURE_BUFFER, m_fragment_rgba_tex.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, m_fragment_rgba_buf.get() );
    
    glBindBuffer( GL_TEXTURE_BUFFER, m_fragment_depth_buf.get() );
    glBufferData( GL_TEXTURE_BUFFER, sizeof(GLfloat)*m_fragment_alloc, NULL, GL_DYNAMIC_DRAW );
    glBindTexture( GL_TEXTURE_BUFFER, m_fragment_depth_tex.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32F, m_fragment_depth_buf.get() );
    
    glBindBuffer( GL_TEXTURE_BUFFER, m_fragment_next_buf.get() );
    glBufferData( GL_TEXTURE_BUFFER, sizeof(GLfloat)*m_fragment_alloc, NULL, GL_DYNAMIC_DRAW );
    glBindTexture( GL_TEXTURE_BUFFER, m_fragment_next_tex.get() );
    glTexBuffer( GL_TEXTURE_BUFFER, GL_R32F, m_fragment_next_buf.get() );
    
    glBindBuffer( GL_TEXTURE_BUFFER, 0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
}

void
FragmentList::resizeScreenSizedBuffers()
{
    glBindTexture( GL_TEXTURE_2D, m_fragment_head_buf.get() );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_R32I, m_width, m_height, 0, GL_RED_INTEGER, GL_INT, NULL );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 );
    glBindTexture( GL_TEXTURE_2D, 0 );
    
    glBindFramebuffer( GL_FRAMEBUFFER, m_fragment_head_fbo.get() );
    glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_fragment_head_buf.get(), 0 );
    glDrawBuffer( GL_COLOR_ATTACHMENT0 );
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

}



    } // of namespace screen
} // of namespace render
