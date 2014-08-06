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
#include <GL/glew.h>
#include <boost/utility.hpp>
#include "render/ManagedGL.hpp"

namespace render {
    class PolyhedralRepresentation;
    namespace mesh {
        class AbstractMeshGPUModel;
        class PolyhedralMeshGPUModel;
        class PolygonSetInterface;
    }
    namespace subset {
        class Representation;
    }
    namespace  surface {
        class GridTessSurf;
        class TriangleSoup;

class GridTessSurfBuilder : public boost::noncopyable
{
public:
    GridTessSurfBuilder();

    ~GridTessSurfBuilder();

    void
    buildSurfaces( boost::shared_ptr<TriangleSoup>                      surf_subset,
                   boost::shared_ptr<TriangleSoup>                      surf_subset_boundary,
                   boost::shared_ptr<TriangleSoup>                      surf_faults,
                   boost::shared_ptr<const subset::Representation>      subset,
                   boost::shared_ptr<const mesh::AbstractMeshGPUModel>  mesh,
                   bool                                                 flip_faces );

    void
    buildSurfaces( boost::shared_ptr<GridTessSurf>            surf_subset,
                   boost::shared_ptr<GridTessSurf>            surf_subset_boundary,
                   boost::shared_ptr<GridTessSurf>            surf_faults,
                   boost::shared_ptr<const subset::Representation>    subset,
                   boost::shared_ptr<const mesh::AbstractMeshGPUModel>  mesh,
                   bool                     flip_faces );

protected:
    enum Surfaces {
        SURFACE_SUBSET,
        SURFACE_SUBSET_BOUNDARY,
        SURFACE_FAULT,
        SURFACE_N
    };
    GLProgram           m_meta1_prog;
    GLint               m_meta1_loc_flip;
    GLProgram           m_meta2_prog;
    GLint               m_meta2_loc_flip;
    GLsizei             m_triangulate_count;
    GLProgram           m_triangulate_prog;
    GLTransformFeedback m_meta_xfb;                 ///< Transform feedback object with SURFACE_N streams
    GLsizei             m_meta_buf_N[SURFACE_N];
    GLBuffer            m_meta_buf[SURFACE_N];
    GLVertexArrayObject m_meta_vao[SURFACE_N];
    GLQuery             m_meta_query[SURFACE_N];

    void
    rebuildTriangulationProgram( GLsizei max_vertices );

    void
    runMetaPass( boost::shared_ptr<const mesh::PolygonSetInterface>  polygon_set,
                 boost::shared_ptr<const subset::Representation>     subset,
                 bool                                                flip_faces );
    //GLuint  m_meta_counters;
};


    } // of namespace surface
} // of namespace render
