#pragma once
#include <GL/glew.h>
#include <boost/utility.hpp>

class GridTess;
class GridField;

class GridTessRenderer : public boost::noncopyable
{
public:
    GridTessRenderer();

    ~GridTessRenderer();

    void
    magic();

    void
    renderCells( GLuint           fbo,
                 const GLsizei    width,
                 const GLsizei    height,
                 const GLfloat*   modelview,
                 const GLfloat*   projection,
                 const GridTess&  tess,
                 const bool       wireframe );

    void
    renderCells( GLuint            fbo,
                 const GLsizei     width,
                 const GLsizei     height,
                 const GLfloat*    modelview,
                 const GLfloat*    projection,
                 const GridTess&   tess,
                 const GridField&  field,
                 const bool        wireframe );


private:
    bool m_magic;

    void
    resizeGBuffer( const GLsizei width, const GLsizei height );

    void
    populateGBuffer( const GLfloat*   modelview,
                     const GLfloat*   projection,
                     const GridTess&  gridtess );

    struct {
        GLuint      m_vertex_array;
        GLuint      m_vertex_buffer;
    }               m_gpgpu_quad;

    struct {
        GLuint      m_fbo;
        GLuint      m_cell_id_texture;
        GLuint      m_depth_texture;
        GLuint      m_normal_texture;
        GLsizei     m_width;
        GLsizei     m_heigth;
    }               m_gbuffer;

    struct {
        GLuint      m_prog;
        GLuint      m_vs;
        GLuint      m_fs;
        GLuint      m_loc_wireframe;
    }               m_screen_solid;

    struct {
        GLuint      m_prog;
        GLuint      m_vs;
        GLuint      m_fs;
        GLint       m_loc_wireframe;
        GLint       m_loc_linear_map;
    }               m_screen_field;

    struct {
        GLuint      m_prog;
        GLuint      m_vs;
        GLuint      m_gs;
        GLuint      m_fs;
        GLint       m_loc_mvp;
        GLint       m_loc_nm;
    }               m_cell_gbuffer_shader;

    struct {
        GLuint      m_prog;
        GLuint      m_vs;
        GLuint      m_gs;
        GLuint      m_fs;
        GLint       m_loc_mvp;
        GLint       m_loc_nm;
    }               m_cell_gbuffer_compact_shader;

};
