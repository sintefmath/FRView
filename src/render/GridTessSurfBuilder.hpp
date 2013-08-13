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
#include "render/ManagedGL.hpp"

namespace render {
    class GridTess;
    class GridTessSurf;
    class GridTessSubset;

class GridTessSurfBuilder : public boost::noncopyable
{
public:
    GridTessSurfBuilder();

    ~GridTessSurfBuilder();

    void
    buildSurfaces( std::shared_ptr<GridTessSurf>            surf_subset,
                   std::shared_ptr<GridTessSurf>            surf_subset_boundary,
                   std::shared_ptr<GridTessSurf>            surf_faults,
                   std::shared_ptr<const GridTessSubset>    subset,
                   std::shared_ptr<const GridTess>          tess,
                   bool                     flip_faces );

protected:
    enum Surfaces {
        SURFACE_SUBSET,
        SURFACE_SUBSET_BOUNDARY,
        SURFACE_FAULT,
        SURFACE_N
    };
    GLProgram           m_meta_prog;
    GLint               m_meta_loc_flip;
    GLsizei             m_triangulate_count;
    GLProgram           m_triangulate_prog;
    GLTransformFeedback m_meta_xfb;                 ///< Transform feedback object with SURFACE_N streams
    GLsizei             m_meta_buf_N[SURFACE_N];
    GLBuffer            m_meta_buf[SURFACE_N];
    GLVertexArrayObject m_meta_vao[SURFACE_N];
    GLQuery             m_meta_query[SURFACE_N];

    void
    rebuildTriangulationProgram( GLsizei max_vertices );

    //GLuint  m_meta_counters;
};

} // of namespace render
