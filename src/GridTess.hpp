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

class GridField;
class CellSelector;
class GridTessBridge;
class GridTessSubset;

class GridTess : public boost::noncopyable
{
    friend class GridTessBridge;
public:
    GridTess();

    GLsizei
    vertexCount() const { return m_vertices.m_N; }

    GLuint
    vertexVertexArrayObject() const { return m_vertices.m_vao; }

    GLuint
    vertexTexture() const { return m_vertices.m_texture; }

    GLuint
    cellGlobalIndexTexture() const { return m_cell_index.m_texture; }

    size_t
    activeCells() const { return m_cell_index.m_body.size(); }

    GLuint
    cellCornerTexture() const { return m_cell_corner.m_texture; }

    GLsizei
    triangleCount() const { return m_triangles.m_index_count/3; }

    GLuint
    triangleVertexArray() const { return m_triangle_vao; }

    GLuint
    triangleInfoTexture() const { return m_tri_info.m_texture; }

    GLuint
    triangleIndexBuffer() const { return m_triangles.m_ibo; }


    /** Maps the compacted cell indices to the 'real' cell indices. */
    const std::vector<unsigned int>&
    getCellIndices() const { return m_cell_index.m_body; }

    const float*
    scale() const { return m_scale; }

    const float*
    shift() const { return m_shift; }

    const float*
    minBBox() const { return m_bb_min; }

    const float*
    maxBBox() const { return m_bb_max; }

protected:
    float               m_scale[3];
    float               m_shift[3];
    float               m_bb_min[3];
    float               m_bb_max[3];

    GLuint              m_triangle_vao;

    struct {
        GLuint          m_buffer;
        GLuint          m_texture;
    }                   m_tri_info;
    struct {
        GLuint          m_ibo;
        GLsizei         m_index_count;
    }                   m_triangles;

    struct {
        std::vector<unsigned int>  m_body;
        GLuint          m_buffer;
        GLuint          m_texture;
    }                   m_cell_index;

    struct {
        GLuint          m_buffer;
        GLuint          m_texture;
    }                   m_cell_corner;

    struct {
        GLuint          m_vao;
        GLuint          m_vbo;
        GLuint          m_texture;
        GLsizei         m_N;
    }                   m_vertices;

    void
    import( GridTessBridge& bridge );

};
