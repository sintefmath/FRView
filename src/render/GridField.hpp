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
#include "render/ManagedGL.hpp"
#include "render/GridTess.hpp"
#include "render/GridFieldBridge.hpp"

namespace render {
    class GridTess;

/** Represent a single property at a single report step for all cells in a grid. */
class GridField
{
    friend class GridFieldBridge;
public:
    GridField( boost::shared_ptr<GridTess> grid );

    /** Get texture buffer sampling the field (indexed by local cell indices). */
    GLuint
    texture() const
    { return m_texture.get(); }

    /** Get minimum value of property over all cells. */
    const float
    minValue() const
    { return m_min_value; }

    /** Get maximum value of property over all cells. */
    const float
    maxValue() const
    { return m_max_value; }

    /** True if object is populated with any data. */
    bool
    hasData() const { return m_has_data; }

    void
    import( GridFieldBridge& bridge );

protected:
    boost::shared_ptr<GridTess>   m_grid;
    bool                        m_has_data;
    GLBuffer                    m_buffer;
    GLTexture                   m_texture;
    GLsizei                     m_count;
    float                       m_min_value;
    float                       m_max_value;
};

} // of namespace render
