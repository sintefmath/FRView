#pragma once
/* Copyright STIFTELSEN SINTEF 2014
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
#include <GL/glew.h>
#include <string>
#include <vector>
#include <boost/utility.hpp>

namespace render {
namespace mesh {

class PolygonSetInterface
{
public:
    virtual
    ~PolygonSetInterface();

    /** Number of cells that use a polygon.
     *
     * \return 1 for polygon meshes (where a polygon touches exactly one
     * cells, and 2 for polyhedral meshes (where a polygon touches one or two
     * cells).
     */
    virtual
    GLuint
    polygonCellCount() const = 0;
    
    /** Returns a per-polygon vertex array object. */
    virtual
    GLuint
    polygonVertexArray() const = 0;

    virtual
    GLuint
    polygonVertexIndexTexture() const = 0;

    virtual
    GLuint
    polygonVertexIndexBuffer() const = 0;

    virtual
    GLuint
    polygonNormalIndexTexture() const = 0;

    /** Returns the number of polygon faces. */
    virtual
    GLsizei
    polygonCount() const = 0;

    /** Returns the max number of vertices in a polygon face. */
    virtual
    GLsizei
    polygonMaxPolygonSize() const = 0;

    /** Returns the number of triangles needed to triangulate all polygons in grid. */
    virtual
    GLsizei
    polygonTriangulatedCount() const = 0;
    
};


} // of namespace mesh
} // of namespace render