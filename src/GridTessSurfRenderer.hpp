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
#include <vector>

class GridTess;
class GridTessSurf;
class GridField;

class GridTessSurfRenderer : public boost::noncopyable
{
public:
    struct RenderItem {
        const GridTessSurf* m_surf;
        float               m_opacity;
        float               m_edge_opacity;
        bool                m_field;
        bool                m_field_log_map;
        float               m_field_min;
        float               m_field_max;
        float               m_solid_color[3];
    };

    GridTessSurfRenderer();

    ~GridTessSurfRenderer();

    /**
      *
      *
      * Quality:
      * - 0 No transparency
      * - 1 Weigthed sum
      * - 2 Weighted sum with weighted alpha
      */
    void
    renderCells( GLuint                          fbo,
                 const GLsizei                   width,
                 const GLsizei                   height,
                 const GLfloat*                  modelview,
                 const GLfloat*                  projection,
                 const GridTess*                 tess,
                 const GridField*                field,
                 const std::vector<RenderItem>&  render_items,
                 const unsigned int              quality );



private:
    unsigned int    m_quality;
    GLsizei         m_width;
    GLsizei         m_height;
    GLuint          m_solid_pass_fbo;
    GLuint          m_transparent_pass_fbo;

    GLuint      m_depth_tex;
    GLuint      m_solid_color_tex;
    GLuint      m_transparent_color_tex;
    GLuint      m_transparent_complexity_tex;

    GLuint      m_gpgpu_quad_vertex_array;
    GLuint      m_gpgpu_quad_buffer;

    GLuint      m_surface_crappy_prog;
    GLint       m_surface_crappy_loc_mvp;
    GLint       m_surface_crappy_loc_color;

    GLuint      m_surface_twopass_prog;
    GLint       m_surface_twopass_loc_mvp;
    GLint       m_surface_twopass_loc_nm;
    GLint       m_surface_twopass_loc_screen_size;

    GLuint      m_surface_singlepass_prog;
    GLint       m_surface_singlepass_loc_mvp;
    GLint       m_surface_singlepass_loc_nm;
    GLint       m_surface_singlepass_loc_screen_size;

    GLuint      m_deferred_prog;
    GLint       m_deferred_loc_use_field;
    GLint       m_deferred_loc_fill_opacities;
    GLint       m_deferred_loc_edge_opacities;
    GLint       m_deferred_loc_log_map;
    GLint       m_deferred_loc_linear_map;


    void
    resizeBuffers();
    void
    drawCrappy( const GLfloat*                  modelview,
                const GLfloat*                  projection,
                const GLfloat*                  modelview_projection,
                const GLfloat*                  normal_transform,
                const GLsizei                   width,
                const GLsizei                   height,
                const GridTess*                 tess,
                const GridField*                field,
                const std::vector<RenderItem>&  render_items );


    void
    drawSinglePass( const GLfloat*                  modelview,
          const GLfloat*                  projection,
          const GLfloat*                  modelview_projection,
          const GLfloat*                  normal_transform,
          const GLsizei                   width,
          const GLsizei                   height,
          const GridTess*                 tess,
          const GridField*                field,
          const std::vector<RenderItem>&  render_items );

    void
    drawTwoPass( const GLfloat*                  modelview,
          const GLfloat*                  projection,
          const GLfloat*                  modelview_projection,
          const GLfloat*                  normal_transform,
          const GLsizei                   width,
          const GLsizei                   height,
          const GridTess*                 tess,
          const GridField*                field,
          const std::vector<RenderItem>&  render_items,
          const bool                      solid_pass );

};
