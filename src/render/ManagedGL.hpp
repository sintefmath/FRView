#pragma once
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
