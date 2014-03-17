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

#include <GL/glew.h>
#include <boost/utility.hpp>
#include "render/ManagedGL.hpp"

namespace render {
    //class GridTess;
    namespace subset {
        class Representation;
    }

    namespace  surface {
    

/** Contains indices for a surface of a subset of a corner-point grid tessellation. */
class GridTessSurf : public boost::noncopyable
{
public:
    GridTessSurf();

    ~GridTessSurf();

    /** Returns the texture name to buffer texture of cell index and normal index. */
    GLuint
    triangleCellTexture() const { return m_cell_texture.get(); }

    /** Returns the buffer name to triangle cornerpoint index buffer. */
    GLuint
    triangleCornerpointIndexBuffer() const { return m_cornerpoint_index_buffer.get(); }

    /** Returns the number of triangles in surface. */
    GLsizei
    triangleCount() const { return m_count; }



    /** Populate triangle buffers using currently set shader.
     *
     * Processes all triangles in tessellation using currently set shader,
     * and stores emitted triangles into triangle buffer using transform
     * feedback.
     *
     * The caller typically uses a geometry shader to only pass on geometry
     * that is part of the surface.
     *
     * - Input (point)
     *   - Abutting cell indices (uvec2, location 0).
     *   - Corner-point indices (uvec3, location 1).
     *   - Normal vector indices (uvec3, location 2).
     * - Output (point)
     *   - Cell index and normal vector indices (x and yzw of uvec4,
     *     location tri_cell_index).
     *   - Corner-point indices (uvec3, location tri_indices_index).
     *
     * In and output to shader pipeline are points.
     *
     * \param tri_cell_index     Transform feedback binding point index.
     * \param tri_indices_index  Transform feedback binding point index.
     */
#if 0
    void
    populateTriangleBuffer( const GridTess*        tess,
                            GLuint                 tri_cell_index    = 0u,
                            GLuint                 tri_indices_index = 1u );
#endif
    
    void
    populatePolygonBuffer( GLsizei                N);

    /** Sets the triangle count for this surface.
     *
     * It may resize buffers (tainting GL_TEXTURE_BUFFER and GL_ELEMENT_ARRAY_BUFFER
     * binding points), and in this case, buffer contents are discarded.
     *
     * \return True if buffers were resized and buffers must be refilled.
     */
    bool
    setTriangleCount( const GLsizei count );

    GLuint
    indicesTransformFeedbackObject() { return m_indices_xfb.get(); }

    GLuint
    indicesCountQuery() { return m_indices_count_qry.get(); }

protected:
    GLsizei             m_count;                    ///< The actual number of triangles in buffers.
    GLsizei             m_alloc;                    ///< The number of triangles that the buffer may hold.
    GLBuffer            m_cell_buffer;              ///< Buffer with cell id + normal vector ix (uvec4).
    GLTexture           m_cell_texture;             ///< Buffer texture for m_cell_buffer.
    GLBuffer            m_cornerpoint_index_buffer; ///< Corner indices for triangles (uvec3).
    GLQuery             m_indices_count_qry;        ///< Query object used to count primitives written.
    GLTransformFeedback m_indices_xfb;
};

    } // of namespace surface
} // of namespace render
