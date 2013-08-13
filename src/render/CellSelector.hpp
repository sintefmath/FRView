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
#include <string>

namespace render {
    class GridTess;
    class GridField;
    class GridTessSubset;

/** Base class for cell subset selectors.
 *
 * Since subclasses mostly differ in which shaders they invoke, they subclasses
 * are very thin. Common to them is applying a shader program with a vertex
 * shader that is captured, and this class provides compiling and initializing
 * such a program.
 *
 */
class CellSelector
{
protected:
    GLuint  m_program;  ///< Shader program with only a vertex shader and output set up for capture.

    CellSelector( const std::string& vs );

    virtual
    ~CellSelector();
};


/** Selects all cells in a grid. */
class AllSelector : public CellSelector
{
public:
    AllSelector();

    /** Populates a \ref GridTessSubset by selecting all cells in a \ref GridTess. */
    void
    apply( std::shared_ptr<GridTessSubset> tess_subset,
           std::shared_ptr<GridTess const> tess );

protected:
};

/** Select all cells within a given index range.
 *
 * This only makes sense for grids originating from a corner-point grid. The
 * global index of cells is assumed to be the non-compacted indices that
 * linerizes the 3D grid.
 *
 * See index_select_vs.glsl for the actual implementation.
 */
class IndexSelector : public CellSelector
{
public:
    IndexSelector();

    /** Populates a \ref GridTessSubset by selecting cells in a \ref GridTess within a given index range.
     *
     * \param tess_subset  Data store for the resulting subset.
     * \param tess         Grid from which to select cells.
     * \param n_i          Number of global cells along the i-axis.
     * \param n_j          Number of global cells along the j-axis.
     * \param n_k          Number of global cells along the k-axis.
     * \param min_i        Minimum i of index range (inclusive).
     * \param min_j        Minimum j of index range (inclusive).
     * \param min_k        Minimum k of index range (inclusive).
     * \param max_i        Maximum i of index range (inclusive).
     * \param max_j        Maximum j of index range (inclusive).
     * \param max_k        Maximum k of index range (inclusive).
     */
    void
    apply( std::shared_ptr<GridTessSubset> tess_subset,
           std::shared_ptr<const GridTess> tess,
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

/** Selects all cells where a field value is within a certain range.
 *
 * See field_select_vs.glsl for the actual implementation.
 */
class FieldSelector : public CellSelector
{
public:
    FieldSelector();

    /** Populates a \ref GridTessSubset by selecting cells in a \ref GridTess where a field value is within a certain range.
     *
     * \param tess_subset  Data store for the resulting subset.
     * \param tess         Grid from which to select cells.
     * \param field        The field that provides the cell property.
     * \param minval       Minimum value for the range (inclusive).
     * \param maxval       Maximum value for the range (inclusive).
     */
    void
    apply( std::shared_ptr<GridTessSubset>  tess_subset,
           std::shared_ptr<const GridTess>  tess,
           std::shared_ptr<const GridField> field,
           const float      minval,
           const float      maxval );

protected:
    GLint   m_loc_min_max;      ///< Uniform location of min and max value (vec2).
};


/** Select all cells with geometry intersecting a plane.
 *
 * See plane_select_vs.glsl for the actual implementation.
 */
class PlaneSelector : public CellSelector
{
public:
    PlaneSelector();

    /** Populates a \ref GridTessSubset.
     *
     * \param tess_subset  Data store for the resulting subset.
     * \param tess         Grid from which to select cells.
     * \param equation     The plane equation to use (4 components).
     */
    void
    apply( std::shared_ptr<GridTessSubset>  tess_subset,
           std::shared_ptr<const GridTess>  tess,
           const float*     equation );

protected:
    GLint   m_loc_plane_eq;     ///< Uniform location of plane equation (vec4).
};

/** Select all cells with geometry partially or fully inside a half-plane.
 *
 * See halfplane_select_vs.glsl for the actual implementation.
 */
class HalfPlaneSelector : public CellSelector
{
public:
    HalfPlaneSelector();

    /** Populates a \ref GridTessSubset.
     *
     * \param tess_subset  Data store for the resulting subset.
     * \param tess         Grid from which to select cells.
     * \param equation     The plane equation to use (4 components).
     */
    void
    apply( std::shared_ptr<GridTessSubset>  tess_subset,
           std::shared_ptr<const GridTess>  tess,
           const float*     equation );

protected:
    GLint   m_loc_halfplane_eq;     ///< Uniform location of plane equation (vec4).
};

} // of namespace render
