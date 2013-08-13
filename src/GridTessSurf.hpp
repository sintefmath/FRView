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
class GridTessSubset;

class GridTessSurf : public boost::noncopyable
{
public:
    GridTessSurf();

    ~GridTessSurf();

    GLuint
    cellTexture() const { return m_tri_cell_texture; }

    GLuint
    indexBuffer() const { return m_tri_indices_buffer; }

    GLsizei
    triangles() const { return m_tri_count; }

/*
    void
    populateBuffer( const GridTessSubset*  subset,
                    const GridTess*        tess,
                    GLuint                 tri_cell_index    = 0u,
                    GLuint                 tri_indices_index = 1u );
*/
    void
    populateBuffer( const GridTess*        tess,
                    GLuint                 tri_cell_index    = 0u,
                    GLuint                 tri_indices_index = 1u );

protected:
    GLuint      m_render_vao;
    GLuint      m_tri_cell_buffer;
    GLuint      m_tri_cell_texture;
    GLuint      m_tri_indices_buffer;
    GLuint      m_tri_count_query;
    GLsizei     m_tri_count;
    GLsizei     m_tri_alloc;
};
