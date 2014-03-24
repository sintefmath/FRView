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
#include <limits>
#include <GL/glew.h>
#include "utils/Logger.hpp"

namespace render {

class GLBuffer
{
public:
    GLBuffer( const std::string& name = "" )
    {
        glGenBuffers( 1, &m_gl_name );
        if( !name.empty() ) {
            Logger log = getLogger( "GLBuffer" );
            LOGGER_DEBUG( log, "buf " << m_gl_name << ": " << name );
        }
    }

    ~GLBuffer()
    {
        glDeleteBuffers( 1, &m_gl_name );
    }

    GLuint
    get() const { return m_gl_name; }


    /** Dump contents of GPU-side buffer into a CPU-side buffer.
     *
     * \param dst     CPU-side buffer into where to store the data.
     * \param count   Maximum number of elements to copy.
     * \param target  OpenGL buffer target to use.
     * \return        Number of elements copied.
     */
    template<typename T>
    size_t
    contents( std::vector<T>& dst,
              size_t count = std::numeric_limits<size_t>::max(),
              GLenum target = GL_PIXEL_PACK_BUFFER )
    {
        glBindBuffer( target, m_gl_name );
        GLint64 bytes;        
        glGetBufferParameteri64v( target, GL_BUFFER_SIZE, &bytes );
        size_t N = std::min( count, static_cast<size_t>(bytes/sizeof(T)) );
        dst.resize( N );
        glGetBufferSubData(	target, 0, sizeof(T)*N, dst.data() );
        return N;
    }
    
    
protected:
    GLuint  m_gl_name;
};

class GLTexture
{
public:
    GLTexture( const std::string& name = "" )
    {
        glGenTextures( 1, &m_gl_name );
        if( !name.empty() ) {
            Logger log = getLogger( "GLTexture" );
            LOGGER_DEBUG( log, "tex " << m_gl_name << ": " << name );
        }
    }

    ~GLTexture()
    {
        glDeleteTextures( 1, &m_gl_name );
    }

    GLuint
    get() const { return m_gl_name; }

protected:
    GLuint  m_gl_name;
};

class GLQuery
{
public:
    GLQuery( const std::string& name = "" )
    {
        glGenQueries( 1, &m_gl_name );
        if( !name.empty() ) {
            Logger log = getLogger( "GLQuery" );
            LOGGER_DEBUG( log, "qry " << m_gl_name << ": " << name );
        }
    }

    ~GLQuery()
    {
        glDeleteQueries( 1, &m_gl_name );
    }

    GLuint
    get() const { return m_gl_name; }

protected:
    GLuint  m_gl_name;
};

class GLProgram
{
public:
    GLProgram( const std::string& name = "" )
        : m_name( name )
    {
        create();
    }

    ~GLProgram()
    {
        glDeleteProgram( m_gl_name );
    }

    void
    reset() {
        glDeleteProgram( m_gl_name );
        create();
    }

    GLuint
    get() const {
        return m_gl_name;
    }

protected:
    void
    create()
    {
        m_gl_name = glCreateProgram();
        if( !m_name.empty() ) {
            Logger log = getLogger( "GLProgram" );
            LOGGER_DEBUG( log, "prg " << m_gl_name << ": " << m_name );
        }
    }

    const std::string   m_name;
    GLuint              m_gl_name;
};


class GLTransformFeedback
{
public:
    GLTransformFeedback( const std::string& name = "" )
    {
        glGenTransformFeedbacks( 1, &m_gl_name );
        if( !name.empty() ) {
            Logger log = getLogger( "GLTransformFeedback" );
            LOGGER_DEBUG( log, "xfb " << m_gl_name << ": " << name );
        }
    }

    ~GLTransformFeedback()
    {
        glDeleteTransformFeedbacks( 1, &m_gl_name );
    }

    GLuint
    get() const { return m_gl_name; }

protected:
    GLuint  m_gl_name;
};


class GLFramebuffer
{
public:
    GLFramebuffer( const std::string& name = "" )
    {
        glGenFramebuffers( 1, &m_gl_name );
        if( !name.empty() ) {
            Logger log = getLogger( "GLFramebuffer" );
            LOGGER_DEBUG( log, "fbo " << m_gl_name << ": " << name );
        }
    }
    
    ~GLFramebuffer()
    {
        glDeleteFramebuffers( 1, &m_gl_name );
    }

    GLuint
    get() const { return m_gl_name; }
    
protected:
    GLuint  m_gl_name;
    
};

class GLVertexArrayObject
{
public:
    GLVertexArrayObject( const std::string& name = "" )
    {
        glGenVertexArrays( 1, &m_gl_name );
        if( !name.empty() ) {
            Logger log = getLogger( "GLVertexArrayObject" );
            LOGGER_DEBUG( log, "vao " << m_gl_name << ": " << name );
        }
    }

    ~GLVertexArrayObject()
    {
        glDeleteVertexArrays( 1, &m_gl_name );
    }

    GLuint
    get() const { return m_gl_name; }

protected:
    GLuint  m_gl_name;

};

} // of namespace render
