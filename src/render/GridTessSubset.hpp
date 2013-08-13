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

/** GPU representation of a subset of the cells in a \ref GridTess.
 *
 * An object of this class is tied to a \ref GridTess, and contains the
 * representation of a cell subset. In particular, it contains a GPU buffer
 * with one bit per cell, where each bit denotes if the cell is part of the
 * subset (true) or not (false).
 *
 * The object is populated by a CellSelector derivative. The surface enclosing
 * the subset is built by \ref GridSurfBuilder.
 *
 */
class GridTessSubset : public boost::noncopyable
{
public:
    GridTessSubset( );

    ~GridTessSubset();

    /** GL buffer containing the bitmask. */
    GLuint
    subsetAsBuffer() const { return m_subset_buffer.get(); }

    /** GL texture containing the bitmask. */
    GLuint
    subsetAsTexture() const { return m_subset_texture.get(); }

    /** Populate the bitmask using the currently bound shader.
     *
     * The function draws (cells_total+31)/32 points and captures data from
     * transform_feedback_index (assumed to be uint's).
     *
     * To be invoked by \ref CellSelector derivatives.
     */
    void
    populateBuffer( std::shared_ptr<const GridTess> tess, GLuint transform_feedback_index = 0u );

private:
    GLsizei             m_cells_total;              ///< Number of cells in GridTess.
    GLBuffer            m_subset_buffer;
    GLTexture           m_subset_texture;
};

} // of namespace render
