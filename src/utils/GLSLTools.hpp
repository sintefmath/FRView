#pragma once
#include <GL/glew.h>
#include <string>
#include <utils/Logger.hpp>
#include <stdexcept>

namespace utils {

void
checkFBO( Logger& log );


GLuint
compileShader( Logger& log, const std::string& source, const GLenum type );


void
linkProgram( Logger& log, GLuint program );


} // of namespace utils