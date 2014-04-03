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

#include <sstream>
#include <iomanip>
#include "utils/GLSLTools.hpp"
#include "utils/Logger.hpp"
#include "render/ManagedGL.hpp"

namespace {

const std::string package = "render.ManagedGL";

const std::string
shaderStageAsString( GLuint shader )
{
    if( !glIsShader( shader ) ) {
        return "";
    }

    GLint type;
    glGetShaderiv( shader, GL_SHADER_TYPE, &type );

    switch( type ) {
    case GL_VERTEX_SHADER:          return "GL_VERTEX_SHADER";          break;
    case GL_TESS_CONTROL_SHADER:    return "GL_TESS_CONTROL_SHADER";    break;
    case GL_TESS_EVALUATION_SHADER: return "GL_TESS_EVALUATION_SHADER"; break;
    case GL_GEOMETRY_SHADER:        return "GL_GEOMETRY_SHADER";        break;
    case GL_FRAGMENT_SHADER:        return "GL_FRAGMENT_SHADER";        break;
    default:
        break;
    }
    return "<UNKNOWN SHADER TYPE>";
}

const std::string
shaderSource( GLuint shader )
{
    if( !glIsShader( shader ) ) {
        return "";
    }

    // retrieve source and pretty-print it
    GLint src_len;
    glGetShaderiv( shader, GL_SHADER_SOURCE_LENGTH, &src_len );
    if (src_len < 1 ) {
        return "    <no source>";
    }
    else {
        std::vector<GLchar> src( src_len );
        glGetShaderSource( shader, src_len, NULL, src.data() );

        std::string tmp;
        std::stringstream o;
        std::stringstream src_stream( src.data() );
        for(int line=1; getline( src_stream, tmp ); line++ ) {
            o << std::setw(3) << line << ' ' << tmp;
        }
        return o.str();
    }
}

const std::string
shaderInfoLog( GLuint shader )
{
    if( !glIsShader( shader ) ) {
        return "";
    }

    GLint log_len;
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &log_len );
    if( log_len <= 1 ) {
        return "    <no infolog>";
    }
    else {
        std::vector<GLchar> log( log_len );
        glGetShaderInfoLog( shader, log_len, NULL, log.data() );

        std::string tmp;
        std::stringstream o;
        std::stringstream log_stream( log.data() );
        while( getline( log_stream, tmp ) ) {
            o << "   " << tmp;
        }
        return o.str();
    }
}

const std::string
programInfoLog( GLuint program )
{
    if( !glIsProgram( program ) ) {
        return "";
    }

    GLint log_len;
    glGetProgramiv( program, GL_INFO_LOG_LENGTH, &log_len );
    if( log_len <= 1 ) {
        return "    <no infolog>";
    }
    else {
        std::vector<GLchar> log( log_len );
        glGetProgramInfoLog( program, log_len, NULL, log.data() );

        std::string tmp;
        std::stringstream o;
        std::stringstream log_stream( log.data() );
        while( getline( log_stream, tmp ) ) {
            o << "   " << tmp;
        }
        return o.str();
    }
}


}

namespace render {



void
GLProgram::addShaderStage( const std::string& source, GLenum type )
{
    Logger log = getLogger( package + ".addShaderStage" );
    const char* p = source.c_str();

    GLuint shader = glCreateShader( type );
        
    glShaderSource( shader, 1, &p, NULL );
    glCompileShader( shader );
            
    GLint status, infolog_len;
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infolog_len );
    glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
    if( status == GL_TRUE ) {
        glAttachShader( m_gl_name, shader );
        if( infolog_len > 1 ) {
            std::stringstream o;
            o << "Possible warnings while compiling " << m_name << ' ' << shaderStageAsString(shader) << ":" << std::endl;
            o << "--- " << m_name << ' ' << shaderStageAsString(shader) << " shader source:" << std::endl;
            o << shaderSource( shader ) << std::endl;
            o << "--- " << m_name << ' ' << shaderStageAsString(shader) << " shader info log:" << std::endl;
            o << shaderInfoLog(shader);
            
            LOGGER_WARN( log, o.str() );
        }
        glDeleteShader( shader );
    }
    else {
        std::stringstream o;
        o << "Failed to compile " << m_name << ' ' << shaderStageAsString(shader) << ":" << std::endl;
        o << "--- " << m_name << ' ' << shaderStageAsString(shader) << " shader source:" << std::endl;
        o << shaderSource( shader ) << std::endl;
        o << "--- " << m_name << ' ' << shaderStageAsString(shader) << " shader info log:" << std::endl;
        o << shaderInfoLog(shader);
        
        LOGGER_ERROR( log, o.str() );
        
        glDeleteShader( shader );
        throw std::runtime_error( "Failed to compile shader, see log for details." );
    }
}

GLint
GLProgram::uniformLocation( const std::string& uniform )
{
    return glGetUniformLocation( m_gl_name, uniform.c_str() );
}

void
GLProgram::link()
{
    Logger log = getLogger( package + ".link" );
    
    glLinkProgram( m_gl_name );

    GLint status, infolog_len;
    glGetProgramiv( m_gl_name, GL_LINK_STATUS, &status );
    glGetProgramiv( m_gl_name, GL_INFO_LOG_LENGTH, &infolog_len );

    if( status == GL_TRUE ) {
        if( infolog_len > 1 ) {
            std::stringstream o;
            o << "Possible warnings while linking " << m_name << ":" << std::endl;
            o << programInfoLog( m_gl_name );
            LOGGER_WARN( log, o.str() );
        }
    }
    else {
        std::stringstream o;
        o << "Error while linking " << m_name << ":" << std::endl;
        
        std::vector<GLuint> shaders(16);
        GLsizei shaders_n = 0;
        glGetAttachedShaders( m_gl_name, shaders.size(), &shaders_n, shaders.data() );
        if( shaders_n == 0 ) {
            o << "    <No attached shaders>";
        }
        for( GLsizei i=0; i<shaders_n; i++ ) {
            o << shaderSource( shaders[i] );
        }
        o << "--- " << m_name << ": Program info log:" << std::endl;
        o << programInfoLog( m_gl_name );
        
        LOGGER_ERROR( log, o.str() );
        
        throw std::runtime_error( "Failed to link shader program, see log for details." );
    }
}


void
GLProgram::create()
{
    m_gl_name = glCreateProgram();
    if( !m_name.empty() ) {
        Logger log = getLogger( "GLProgram" );
        LOGGER_DEBUG( log, "prg " << m_gl_name << ": " << m_name );
    }
}


} // of namespace render
