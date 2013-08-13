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
#include <string>

class GridTess;
class GridField;
class GridTessSubset;


class CellSelector
{
protected:
    GLuint  m_prog;
    GLuint  m_vs;
    GLuint  m_query;

    CellSelector( const std::string& vs );
};


class AllSelector : public CellSelector
{
public:
    AllSelector();

    void
    apply(GridTessSubset* tess_subset, const GridTess* tess );

protected:
};

class IndexSelector : public CellSelector
{
public:
    IndexSelector();

    void
    apply( GridTessSubset* tess_subset,
           const GridTess* tess,
           unsigned int n_i,
           unsigned int n_j,
           unsigned int n_k,
           unsigned int min_i,
           unsigned int min_j,
           unsigned int min_k,
           unsigned int max_i,
           unsigned int max_j,
           unsigned int max_k );

protected:
    GLint       m_loc_grid_dim;
    GLint       m_loc_index_min;
    GLint       m_loc_index_max;

};

class FieldSelector : public CellSelector
{
public:
    FieldSelector();

    void
    apply( GridTessSubset*  tess_subset,
           const GridTess*  tess,
           const GridField* field,
           const float      minval,
           const float      maxval );

protected:
    GLint   m_loc_min_max;
};


class PlaneSelector : public CellSelector
{
public:
    PlaneSelector();

    void
    apply( GridTessSubset*  tess_subset,
           const GridTess*  tess,
           const float*     equation );

protected:
    GLint   m_loc_plane_eq;
};

class HalfPlaneSelector : public CellSelector
{
public:
    HalfPlaneSelector();

    void
    apply( GridTessSubset*  tess_subset,
           const GridTess*  tess,
           const float*     equation );

protected:
    GLint   m_loc_halfplane_eq;
};
