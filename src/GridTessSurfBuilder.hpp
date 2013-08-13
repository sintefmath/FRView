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
class GridTessSurf;
class GridTessSubset;

class GridTessSurfBuilder : public boost::noncopyable
{
public:
    GridTessSurfBuilder();

    ~GridTessSurfBuilder();

    void
    buildSubsetSurface( GridTessSurf*           surf,
                        const GridTessSubset*   subset,
                        const GridTess*         tess,
                        bool                    flip_faces,
                        bool                    geometric_edges );

    void
    buildSubsetBoundarySurface( GridTessSurf*          surf,
                                const GridTessSubset*  subset,
                                const GridTess*        tess,
                                bool                   flip_faces,
                                bool                   geometric_edges );

    void
    buildFaultSurface( GridTessSurf*    surf,
                       const GridTess*  tess,
                       bool             flip_faces,
                       bool             geometric_edges );

    void
    buildBoundarySurface( GridTessSurf*    surf,
                          const GridTess*  tess,
                          bool             flip_faces,
                          bool             geometric_edges );

protected:
    enum Pipeline {
        PIPELINE_COMPACT_SUBSET,
        PIPELINE_COMPACT_SUBSET_BOUNDARY,
        PIPELINE_COMPACT_FAULT,
        PIPELINE_COMPACT_BOUNDARY,
        PIPELINE_COMPACT_N
    };
    struct PipelineData {
        GLuint          m_edge_program;         ///< GS-program that pass through selected edges.
        GLuint          m_triangle_program;     ///< GS-program that pass through selected triangle.
        GLuint          m_triangle_loc_flip;    ///< True if triangle orientation should be reversed.
    };
    PipelineData    m_pipeline[ PIPELINE_COMPACT_N ];

    GLuint  m_polygon_prog;
    GLuint  m_polygon_loc_flip;

    GLuint  m_meta_prog;
    GLuint  m_meta_loc_flip;

    GLuint  m_meta_buf;
    GLsizei m_meta_buf_N;
    GLuint  m_meta_vao;

    enum Surfaces {
        SURFACE_SUBSET,
        SURFACE_SUBSET_BOUNDARY,
        SURFACE_FAULT,
        SURFACE_N
    };
    GLuint  m_meta_query[SURFACE_N];

    static
    void
    applyPipeline( PipelineData&          pipeline,
                   GridTessSurf*          surf,
                   const GridTessSubset*  subset,
                   const GridTess*        tess,
                   bool                   flip_faces,
                   bool                   geometric_edges );

    //GLuint  m_edge_compact_subset_prog;

};
