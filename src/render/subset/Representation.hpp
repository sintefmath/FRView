/* Copyright STIFTELSEN SINTEF 2013
 * 
 * This file is part of FRView.
 * FRView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * FRView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *  
 * You should have received a copy of the GNU Affero General Public License
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <memory>
#include <boost/shared_ptr.hpp>
#include <GL/glew.h>
#include <boost/utility.hpp>
#include "render/ManagedGL.hpp"

namespace render {
    namespace mesh {
        class CellSetInterface;
    }

    namespace subset {

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
class Representation : public boost::noncopyable
{
public:
    Representation( );

    ~Representation();

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
    populateBuffer( boost::shared_ptr<const render::mesh::CellSetInterface> cell_set,
                    GLuint transform_feedback_index = 0u );

private:
    GLsizei             m_cells_total;              ///< Number of cells in GridTess.
    GLBuffer            m_subset_buffer;
    GLTexture           m_subset_texture;
};

    } // of namespace subset
} // of namespace render
