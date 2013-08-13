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
#include <boost/utility.hpp>

class GridTess;

class GridTessSubset : public boost::noncopyable
{
public:
    GridTessSubset( );

    ~GridTessSubset();

    GLuint
    buffer() const { return m_subset_buffer; }

    GLuint
    texture() const { return m_subset_texture; }

    GLsizei
    selected() const { return m_cells_selected; }

    void
    populateBuffer( const GridTess* tess, GLuint transform_feedback_index = 0u );


private:
    GLsizei             m_cells_total;
    GLsizei             m_cells_selected;
    GLuint              m_primitive_count_query;
    GLuint              m_subset_buffer;
    GLuint              m_subset_texture;
};
