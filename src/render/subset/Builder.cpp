/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include "utils/Logger.hpp"
#include "utils/GLSLTools.hpp"
#include "render/GridTess.hpp"
#include "render/GridField.hpp"
#include "render/subset/Representation.hpp"
#include "render/subset/Builder.hpp"

namespace resources {
    extern const std::string all_select_vs;
    extern const std::string field_select_vs;
    extern const std::string index_select_vs;
    extern const std::string halfplane_select_vs;
    extern const std::string plane_select_vs;
}
namespace render {
    namespace subset {

Builder::Builder( const std::string& vs )
{
    Logger log = getLogger( "render.CellSelector.constructor" );
    m_program = glCreateProgram();
    GLuint v = utils::compileShader( log, vs, GL_VERTEX_SHADER );
    glAttachShader( m_program, v );
    const char* varyings[1] = {
        "selected"
    };
    glTransformFeedbackVaryings( m_program, 1, varyings, GL_INTERLEAVED_ATTRIBS );
    utils::linkProgram( log, m_program );
    glDeleteShader( v );
}

Builder::~Builder()
{
    glDeleteProgram( m_program );
}


    } // of namespace subset
} // of namespace render
