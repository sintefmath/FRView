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
#include "GridTess.hpp"
#include "GridTessSurf.hpp"

class GridTessSurfBBoxFinder
{
public:
    GridTessSurfBBoxFinder();

    ~GridTessSurfBBoxFinder();

    void
    find( float                (&transform)[16],
          float                (&aabb_min)[3],
          float                (&aabb_max)[3],
          const GridTess*      tess,
          const GridTessSurf*  surf );


protected:

    void
    eigenValues( double& r0, double& r1, double& r2,
                 const double a00, const double a01, const double a02,
                 const double a11, const double a12,
                 const double a22 );

    void
    eigenVector( float (&v)[3], const double r,
                 const double a00, const double a01, const double a02,
                 const double a11, const double a12,
                 const double a22 );



    GLsizei                 m_size_l2;
    GLsizei                 m_size;

    GLuint                  m_barycenter_reduction_tex;
    std::vector<GLuint>     m_barycenter_reduction_fbo;

    GLuint                  m_covariance_reduction_0_tex;
    GLuint                  m_covariance_reduction_1_tex;
    std::vector<GLuint>     m_covariance_reduction_fbo;

    GLuint                  m_aabbox_reduction_min_tex;
    GLuint                  m_aabbox_reduction_max_tex;
    std::vector<GLuint>     m_aabbox_reduction_fbo;

    GLuint                  m_gpgpu_quad_vertex_array;
    GLuint                  m_gpgpu_quad_buffer;


    GLuint                  m_aabbox_baselevel_prog;
    GLuint                  m_aabbox_baselevel_query;

    GLuint                  m_aabbox_reduction_prog;
    GLuint                  m_aabbox_reduction_query;

    GLuint                  m_barycenter_baselevel_prog;
    GLuint                  m_barycenter_baselevel_query;

    GLuint                  m_barycenter_reduction_prog;
    GLuint                  m_barycenter_reduction_query;

    GLuint                  m_covariance_baselevel_prog;
    GLuint                  m_covariance_baselevel_query;

    GLuint                  m_covariance_reduction_prog;
    GLuint                  m_covariance_reduction_query;

};
