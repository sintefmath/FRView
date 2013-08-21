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
#include <GL/glew.h>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include "render/ManagedGL.hpp"

namespace render {
    class GridTess;
    class GridField;
    namespace  surface {
        class GridTessSurf;

/** Corner-point grid renderer
 *
 * This class implements functionality to render corner-point grids (triangles
 * and lines) in various ways. The class does not contain any corner-point
 * geometry, but the public functions takes a GridTess (triangulated corner-
 * point grid) and a set of GridTessSurf (surface of a cell subset).
 */
class GridTessSurfRenderer : public boost::noncopyable
{
public:
    struct RenderItem {
        boost::shared_ptr<const GridTessSurf> m_surf;
        bool                m_field;
        bool                m_field_log_map;
        float               m_field_min;
        float               m_field_max;
        float               m_line_thickness;
        float               m_edge_color[4];
        float               m_face_color[4];
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
                 boost::shared_ptr<const GridTess>                 tess,
                 boost::shared_ptr<const GridField>                field,
                 const std::vector<RenderItem>&  render_items,
                 const unsigned int              quality );

    void
    draw( const GLfloat*                            modelview,
          const GLfloat*                            projection,
          const GLfloat*                            modelview_projection,
          const GLfloat*                            normal_transform,
          const GLsizei                             width,
          const GLsizei                             height,
          const boost::shared_ptr<const GridTess>   tess,
          const boost::shared_ptr<const GridField>  field,
          const std::vector<RenderItem>&            render_items );

    void
    drawSinglePass( const GLfloat*                  modelview,
          const GLfloat*                  projection,
          const GLfloat*                  modelview_projection,
          const GLfloat*                  normal_transform,
          const GLsizei                   width,
          const GLsizei                   height,
          boost::shared_ptr<const GridTess>                 tess,
          boost::shared_ptr<const GridField>                field,
          const std::vector<RenderItem>&  render_items );
    
    void
    drawTwoPass( const GLfloat*                  modelview,
          const GLfloat*                  projection,
          const GLfloat*                  modelview_projection,
          const GLfloat*                  normal_transform,
          const GLsizei                   width,
          const GLsizei                   height,
          boost::shared_ptr<const GridTess>                 tess,
          boost::shared_ptr<const GridField>                field,
          const std::vector<RenderItem>&  render_items,
          const bool                      solid_pass );
    

private:
    unsigned int    m_quality;
    GLsizei         m_width;
    GLsizei         m_height;
    GLuint          m_fbo_solid;
    GLuint          m_fbo_weighted_average_transparent;
    GLuint          m_fbo_weighted_sum_transparent;

    GLuint      m_depth_tex;
    GLuint      m_solid_color_tex;
    GLuint      m_transparent_color_tex;
    GLuint      m_transparent_complexity_tex;


    GLuint      m_gpgpu_quad_vertex_array;
    GLuint      m_gpgpu_quad_buffer;


    enum Pass {
        PASS_EDGE,
        PASS_SURFACE_ONEPASS,
        PASS_SURFACE_ONEPASS_PAINT,
        PASS_SURFACE_TWOPASS,
        PASS_SURFACE_TWOPASS_PAINT,
        //PASS_SURFACE_FRAG_LIST,
        //PASS_SURFACE_FRAG_LIST_PAINT,
        PASS_N
    };

    struct PassData {
        GLuint  m_program;
        GLint   m_loc_mvp;
        GLint   m_loc_mv;
        GLint   m_loc_nm;
        GLint   m_loc_surface_color;
        GLint   m_loc_edge_color;
        GLint   m_loc_screen_size;
    };
    PassData    m_pass_data[ PASS_N ];

    struct {
        GLuint      m_program;
    }           m_pass_weighted_sum_merge;


    GLuint      m_surface_crappy_prog;
    GLint       m_surface_crappy_loc_mvp;
    GLint       m_surface_crappy_loc_color;


    GLuint      m_deferred_prog;
    GLint       m_deferred_loc_use_field;
    GLint       m_deferred_loc_fill_opacities;
    GLint       m_deferred_loc_edge_opacities;
    GLint       m_deferred_loc_log_map;
    GLint       m_deferred_loc_linear_map;

    struct {
        GLuint      m_prog;
        GLint       m_loc_mvp;
        GLint       m_loc_color;
    }           m_geo_edge;


    void
    resizeBuffers();


    void
    drawCrappy( const GLfloat*                  modelview,
                const GLfloat*                  projection,
                const GLfloat*                  modelview_projection,
                const GLfloat*                  normal_transform,
                const GLsizei                   width,
                const GLsizei                   height,
                const boost::shared_ptr<const GridTess>                 tess,
                const boost::shared_ptr<const GridField>                field,
                const std::vector<RenderItem>&  render_items );





};

    } // of namespace surface
} // of namespace render
