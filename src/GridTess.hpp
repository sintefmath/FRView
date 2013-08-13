#pragma once
#include <vector>
#include <GL/glew.h>
#include <boost/utility.hpp>

class GridField;
class CellSelector;
class GridTessBridge;

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

    size_t
    activeCells() const { return m_cell_index.m_body.size(); }

    GLuint
    cellCornerTexture() const { return m_cell_corner.m_texture; }

    GLsizei
    triangleCount() const { return m_triangles.m_index_count/3; }

    GLuint
    triangleInfoTexture() const { return m_tri_info.m_texture; }

    GLuint
    triangleIndexBuffer() const { return m_triangles.m_ibo; }

    GLsizei
    triangleCompactCount() const { return m_triangle_compact_count; }

    GLuint
    triangleCompactIndexBuffer() const { return m_triangle_compact_indices_buffer; }

    GLuint
    triangleCompactCellTexture() const { return m_triangle_compact_cell_texture; }


    GLsizei
    cellCount() const { return m_cells; }

    GLuint
    cellSubsetBuffer() const { return m_cell_subset_buffer; }

    GLuint
    cellSubsetTexture() const { return m_cell_subset_texture; }


    /** Maps the compacted cell indices to the 'real' cell indices. */
    const std::vector<unsigned int>&
    getCellIndices() const { return m_cell_index.m_body; }

    void
    update( unsigned int cells );

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


    GLsizei             m_cells;
    GLuint              m_cell_subset_buffer;
    GLuint              m_cell_subset_texture;

    GLuint              m_triangle_compact_prog;
    GLuint              m_triangle_compact_vs;
    GLuint              m_triangle_compact_gs;

    GLuint              m_triangle_vao;
    GLuint              m_triangle_compact_cell_buffer;
    GLuint              m_triangle_compact_cell_texture;
    GLuint              m_triangle_compact_indices_buffer;
    GLuint              m_triangle_compact_query;
    GLsizei             m_triangle_compact_count;
    GLsizei             m_triangle_compact_allocated;

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

    void
    allocTriangleCompactBuffers();

    void
    triangleCompactionPass();

};
