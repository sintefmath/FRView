#include <iomanip>
#include "utils/GLSLTools.hpp"
#include <sstream>

namespace utils {


void
checkFBO( Logger& log )
{
    GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
    if( status == GL_FRAMEBUFFER_COMPLETE ) {
        return;
    }
    else {
        std::string status_str;
        switch( status ) {
        case GL_FRAMEBUFFER_UNDEFINED:
            status_str = "GL_FRAMEBUFFER_UNDEFINED";
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            status_str = "GL_FRAMEBUFFER_UNSUPPORTED";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            status_str = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            status_str = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            status_str = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            status_str = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            status_str = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
            break;
        default: status_str = "<unknown status>"; break;
        }
        LOGGER_ERROR( log, "Incomplete framebuffer: " << status_str );
        throw std::runtime_error( "GL incomplete framebuffer, see log" );
    }
}

void
dumpShaderSource( Logger& log, GLuint shader )
{
    GLint type;
    glGetShaderiv( shader, GL_SHADER_TYPE, &type );
    std::string type_str;
    switch( type ) {
    case GL_VERTEX_SHADER: type_str = "GL_VERTEX_SHADER"; break;
    case GL_TESS_CONTROL_SHADER: type_str = "GL_TESS_CONTROL_SHADER"; break;
    case GL_TESS_EVALUATION_SHADER: type_str = "GL_TESS_EVALUATION_SHADER"; break;
    case GL_GEOMETRY_SHADER: type_str = "GL_GEOMETRY_SHADER"; break;
    case GL_FRAGMENT_SHADER: type_str = "GL_FRAGMENT_SHADER"; break;
    default: type_str = "<unknown>"; break;
    }

    LOGGER_DEBUG( log, "Source of shader " << shader << " [" << type_str << "]:" );
    GLint src_len;
    glGetShaderiv( shader, GL_SHADER_SOURCE_LENGTH, &src_len );
    if (src_len < 1 ) {
        LOGGER_DEBUG( log, "<no source>" );
    }
    else {
        GLchar* source = new GLchar[ src_len ];
        glGetShaderSource( shader, src_len, NULL, source );
        std::stringstream stream( source );
        std::string t;
        for(int line=1; getline( stream, t); line++ ) {
            LOGGER_DEBUG( log, std::setw(3) << line << ' '  << t );
        }
        delete[] source;
    }
    
    GLint infolog_len;
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infolog_len );
    if( infolog_len <= 1 ) {
        LOGGER_DEBUG( log, "Shader " << shader << " has no infolog." );
    }
    else {
        LOGGER_WARN( log, "Infolog of shader " << shader << " [" << type_str << "]:" );
        GLchar* infolog = new GLchar[ infolog_len ];
        glGetShaderInfoLog( shader, infolog_len, NULL, infolog );
        std::stringstream stream( infolog );
        std::string t;
        while( getline( stream, t ) ) {
            LOGGER_WARN( log, t );
        }
        delete[] infolog;
    }
}


void
dumpProgramSources( Logger& log, GLuint program )
{
    GLuint shaders[16];
    GLsizei shaders_n = 0;
    glGetAttachedShaders( program, 16, &shaders_n, shaders );
    LOGGER_DEBUG( log, "Sources of shader program " << program << ':' );
    if( shaders_n == 0 ) {
        LOGGER_DEBUG( log, "No shaders attached to program." );
    }
    for(GLsizei i=0; i<shaders_n; i++ ) {
        dumpShaderSource( log, shaders[i] );
    }
    
    for(GLsizei i=0; i<shaders_n; i++ ) {
        LOGGER_DEBUG( log, "Program " << program << " has shader " << shaders[i] << " attached." );
    }
    
    
    GLint infolog_len;
    glGetProgramiv( program, GL_INFO_LOG_LENGTH, &infolog_len );
    if( infolog_len <= 1 ) {
        LOGGER_DEBUG( log, "Program " << program << " has no infolog." );
    }
    else {
        LOGGER_WARN( log, "Infolog of program " << program << ':' );
        GLchar* infolog = new GLchar[ infolog_len ];
        glGetProgramInfoLog( program, infolog_len, NULL, infolog );
        std::stringstream stream( infolog );
        std::string t;
        while( getline( stream, t ) ) {
            LOGGER_WARN( log, t );
        }
        delete[] infolog;
    }
}



GLuint
compileShader( Logger& log, const std::string& source, const GLenum type )
{
    const char* p = source.c_str();
    
    GLuint shader = glCreateShader( type );

    glShaderSource( shader, 1, &p, NULL );
    glCompileShader( shader );
    
    GLint status, infolog_len;
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infolog_len );
    glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
    if( (status != GL_TRUE ) || (infolog_len > 1 )  ) {
        dumpShaderSource( log, shader );

        if( status != GL_TRUE ) {
            throw std::runtime_error( "GLSL compile error, see log." );
        }
    }
    return shader;
}

void
linkProgram( Logger& log, GLuint program )
{
    glLinkProgram( program );
    
    GLint status, infolog_len;
    glGetProgramiv( program, GL_LINK_STATUS, &status );
    glGetProgramiv( program, GL_INFO_LOG_LENGTH, &infolog_len );
    if( (status != GL_TRUE ) || (infolog_len > 1 ) ) {
        dumpProgramSources( log, program );
        if( status != GL_TRUE ) {
            throw std::runtime_error( "GLSL link error, see log." );
        }
    }
    
}


} // of namespace utils
