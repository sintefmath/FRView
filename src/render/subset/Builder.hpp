/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once
#include <memory>
#include <boost/shared_ptr.hpp>
#include <GL/glew.h>
#include <string>

namespace render {
    class GridTess;
    class GridField;
    namespace subset {
        class Representation;

/** Base class for cell subset selectors.
 *
 * Since subclasses mostly differ in which shaders they invoke, they subclasses
 * are very thin. Common to them is applying a shader program with a vertex
 * shader that is captured, and this class provides compiling and initializing
 * such a program.
 *
 */
class Builder
{
protected:
    GLuint  m_program;  ///< Shader program with only a vertex shader and output set up for capture.

    Builder( const std::string& vs );

    virtual
    ~Builder();
};

    } // of namespace subset
} // of namespace render
