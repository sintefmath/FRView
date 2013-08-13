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
                        bool                    flip_faces );

    void
    buildSubsetBoundarySurface( GridTessSurf*          surf,
                                const GridTessSubset*  subset,
                                const GridTess*        tess,
                                bool                   flip_faces );

    void
    buildFaultSurface( GridTessSurf* surf,
                       const GridTess* tess,
                       bool flip_faces );

    void
    buildBoundarySurface( GridTessSurf*    surf,
                          const GridTess*  tess,
                          bool             flip_faces );

protected:
    GLuint  m_compact_subset_prog;
    GLint   m_compact_subset_loc_flip;
    GLuint  m_compact_subset_boundary_prog;
    GLint   m_compact_subset_boundary_loc_flip;
    GLuint  m_compact_fault_prog;
    GLint   m_compact_fault_loc_flip;
    GLuint  m_compact_boundary_prog;
    GLint   m_compact_boundary_loc_flip;
};
