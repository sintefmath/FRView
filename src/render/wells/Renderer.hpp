/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once
#include <GL/glew.h>
#include <vector>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include "render/ManagedGL.hpp"

namespace render {
    namespace wells {
        class Representation;

class WellRenderer : public boost::noncopyable
{
public:

    WellRenderer();

    ~WellRenderer();

    void
    render( GLsizei                             width,
            GLsizei                             height,
            const GLfloat*                      projection,
            const GLfloat*                      camera_from_world,
            const GLfloat*                      world_from_model,
            boost::shared_ptr<Representation>   wells );


protected:
    GLProgram   m_well_prog;
};

    } // of namespace wells
} // of namespace render
