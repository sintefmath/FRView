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

#include "utils/GLSLTools.hpp"
#include "utils/Logger.hpp"
#include "render/mesh/PolyhedralMeshGPUModel.hpp"
#include "render/mesh/PolygonMeshGPUModel.hpp"
#include "render/subset/Representation.hpp"
#include "render/surface/GridTessSurf.hpp"
#include "render/surface/TriangleSoup.hpp"
#include "render/surface/GridTessSurfBuilder.hpp"
#include <sstream>

namespace resources {
}

namespace render {
    namespace  surface {
        namespace glsl {
            extern const std::string GridTessSurfBuilder_triangulate_vs;
            extern const std::string GridTessSurfBuilder_triangulate_gs;
            extern const std::string GridTessSurfBuilder_triangulate_indexed_gs;
            extern const std::string GridTessSurfBuilder_triangulate_trisoup_gs;
            extern const std::string GridTessSurfBuilder_extract1_vs;
            extern const std::string GridTessSurfBuilder_extract1_gs;
            extern const std::string GridTessSurfBuilder_extract2_vs;
            extern const std::string GridTessSurfBuilder_extract2_gs;
        }
        using boost::shared_ptr;
        using boost::dynamic_pointer_cast;
        using render::mesh::AbstractMeshGPUModel;
        using render::mesh::PolyhedralMeshGPUModel;
        using render::mesh::PolygonMeshGPUModel;
        using render::mesh::VertexPositionInterface;
        using render::mesh::PolygonSetInterface;

GridTessSurfBuilder::GridTessSurfBuilder()
    : m_meta1_prog( "GridTessSurfBuilder.m_meta1_prog"),
      m_meta2_prog( "GridTessSurfBuilder.m_meta2_prog"),
      m_triangulate_indexed_count( 0 ),
      m_triangulate_indexed_prog( "GridTessSurfBuilder.riangulate_indexed_prog" ),
      m_triangulate_trisoup_count( 0 ),
      m_triangulate_trisoup_prog( "GridTessSurfBuilder.triangulate_trisoup_prog" )
{
    Logger log = getLogger( "render.GridTessSurfBuilder.constructor" );

    static const char* faces_feedback[] = { "meta_subset",
                                            "meta_boundary",
                                            "meta_fault" };
    GLuint faces_vs, faces_gs;

    // --- surface meta extract ------------------------------------------------
    faces_vs = utils::compileShader( log, glsl::GridTessSurfBuilder_extract1_vs, GL_VERTEX_SHADER );
    faces_gs = utils::compileShader( log, glsl::GridTessSurfBuilder_extract1_gs, GL_GEOMETRY_SHADER );
    glAttachShader( m_meta1_prog.get(), faces_vs );
    glAttachShader( m_meta1_prog.get(), faces_gs );
    glTransformFeedbackVaryings( m_meta1_prog.get(), 2, faces_feedback, GL_SEPARATE_ATTRIBS );
    utils::linkProgram( log, m_meta1_prog.get() );
    m_meta1_loc_flip = glGetUniformLocation( m_meta1_prog.get(), "flip_faces" );
    glDeleteShader( faces_vs );
    glDeleteShader( faces_gs );

    // --- volume surace meta extract ------------------------------------------
    faces_vs = utils::compileShader( log, glsl::GridTessSurfBuilder_extract2_vs, GL_VERTEX_SHADER );
    faces_gs = utils::compileShader( log, glsl::GridTessSurfBuilder_extract2_gs, GL_GEOMETRY_SHADER );
    glAttachShader( m_meta2_prog.get(), faces_vs );
    glAttachShader( m_meta2_prog.get(), faces_gs );
    glTransformFeedbackVaryings( m_meta2_prog.get(), 3, faces_feedback, GL_SEPARATE_ATTRIBS );
    utils::linkProgram( log, m_meta2_prog.get() );
    m_meta2_loc_flip = glGetUniformLocation( m_meta2_prog.get(), "flip_faces" );
    glDeleteShader( faces_vs );
    glDeleteShader( faces_gs );

    // -------------------------------------------------------------------------
    // create meta buffers
    // -------------------------------------------------------------------------
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, m_meta_xfb.get() );
    for(int i=0; i<SURFACE_N; i++ ) {
        m_meta_buf_N[i] = 102400;
        glBindBuffer( GL_ARRAY_BUFFER, m_meta_buf[i].get() );
        glBufferData( GL_ARRAY_BUFFER,
                      sizeof(unsigned int)*4*m_meta_buf_N[i],
                      NULL,
                      GL_DYNAMIC_COPY );
        glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, i, m_meta_buf[i].get() );

        glBindVertexArray( m_meta_vao[i].get() );
        glVertexAttribIPointer( 0, 4, GL_UNSIGNED_INT, 0, NULL );
        glEnableVertexAttribArray( 0 );
        glBindVertexArray( 0 );

    }
    glBindBuffer( GL_ARRAY_BUFFER,0 );
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
}

GridTessSurfBuilder::~GridTessSurfBuilder()
{
}

void
GridTessSurfBuilder::rebuildIndexedTriangulationProgram( GLsizei max_vertices )
{
    Logger log = getLogger( "render.GridTessSurfBuilder.rebuildIndexedTriangulationProgram" );

    m_triangulate_indexed_count = max_vertices;
    m_triangulate_indexed_prog.reset();

    static const char* triangle_feedback[2] = {
        "cell",
        "indices"
    };

    GLsizei max_out = std::max( (GLsizei)3, max_vertices )-2;
    std::stringstream o;
    o << "#define MAX_OUT " << max_out << "\n";
    GLuint poly_vs = utils::compileShader( log,
                                           glsl::GridTessSurfBuilder_triangulate_vs,
                                           GL_VERTEX_SHADER );
    GLuint poly_subset_gs = utils::compileShader( log,
                                                  o.str() +
                                                  glsl::GridTessSurfBuilder_triangulate_indexed_gs +
                                                  glsl::GridTessSurfBuilder_triangulate_gs,
                                                  GL_GEOMETRY_SHADER );
    glAttachShader( m_triangulate_indexed_prog.get(), poly_vs );
    glAttachShader( m_triangulate_indexed_prog.get(), poly_subset_gs );
    glTransformFeedbackVaryings( m_triangulate_indexed_prog.get(), 2, triangle_feedback, GL_SEPARATE_ATTRIBS );
    utils::linkProgram( log, m_triangulate_indexed_prog.get() );
    glDeleteShader( poly_vs );
    glDeleteShader( poly_subset_gs );
}

void
GridTessSurfBuilder::rebuildTriSoupTriangulationProgram( GLsizei max_vertices )
{
    Logger log = getLogger( "render.GridTessSurfBuilder.rebuildTriSoupTriangulationProgram" );

    m_triangulate_trisoup_count = max_vertices;
    m_triangulate_trisoup_prog.reset();

    static const char* triangle_feedback[3] = {
        "out_color",
        "out_normal",
        "out_position"
    };

    GLsizei max_out = std::max( (GLsizei)3, max_vertices )-2;
    std::stringstream o;
    o << "#define MAX_OUT " << (3*max_out) << "\n";
    GLuint poly_vs = utils::compileShader( log,
                                           glsl::GridTessSurfBuilder_triangulate_vs,
                                           GL_VERTEX_SHADER );
    GLuint poly_subset_gs = utils::compileShader( log,
                                                  o.str() +
                                                  glsl::GridTessSurfBuilder_triangulate_trisoup_gs +
                                                  glsl::GridTessSurfBuilder_triangulate_gs,
                                                  GL_GEOMETRY_SHADER );
    glAttachShader( m_triangulate_trisoup_prog.get(), poly_vs );
    glAttachShader( m_triangulate_trisoup_prog.get(), poly_subset_gs );
    glTransformFeedbackVaryings( m_triangulate_trisoup_prog.get(), 3, triangle_feedback, GL_INTERLEAVED_ATTRIBS );
    utils::linkProgram( log, m_triangulate_trisoup_prog.get() );
    glDeleteShader( poly_vs );
    glDeleteShader( poly_subset_gs );
}

void
GridTessSurfBuilder::buildSurfaces( boost::shared_ptr<GridTessSurf> surf_subset,
                                    boost::shared_ptr<TriangleSoup> surf_subset_soup,
                                    boost::shared_ptr<GridTessSurf> surf_subset_boundary,
                                    boost::shared_ptr<TriangleSoup> surf_subset_boundary_soup,
                                    boost::shared_ptr<GridTessSurf> surf_faults,
                                    boost::shared_ptr<TriangleSoup> surf_faults_soup,
                                    boost::shared_ptr<const subset::Representation> subset,
                                    boost::shared_ptr<const mesh::AbstractMeshGPUModel> mesh,
                                    bool                     flip_faces )
{
    Logger log = getLogger( "GridTessSurfBuilder.buildSurfaces" );

    shared_ptr<const PolygonSetInterface> polygon_set
            = dynamic_pointer_cast<const PolygonSetInterface>( mesh );
    if( !polygon_set ) {
        LOGGER_ERROR( log, "mesh does not implement the polygon set interface." );
        return;
    }
    shared_ptr<const VertexPositionInterface> vertex_positions
            = dynamic_pointer_cast<const VertexPositionInterface>( mesh );
    if( !vertex_positions ) {
        LOGGER_ERROR( log, "mesh does not implement the vertex positions interface." );
        return;
    }
    
    bool any_indexed_surfaces = false;
    GridTessSurf* indexed_surfaces[ SURFACE_N ];
    indexed_surfaces[ SURFACE_SUBSET          ] = surf_subset.get();
    indexed_surfaces[ SURFACE_SUBSET_BOUNDARY ] = surf_subset_boundary.get();
    indexed_surfaces[ SURFACE_FAULT ]           = surf_faults.get();
    for( int i=0; i<SURFACE_N; i++ ) {
        if( indexed_surfaces[i] != NULL ) {
            any_indexed_surfaces = true;
        }
    }

    bool any_trisoup_surfaces = false;
    TriangleSoup* trisoup_surfaces[ SURFACE_N ];
    trisoup_surfaces[ SURFACE_SUBSET          ] = surf_subset_soup.get();
    trisoup_surfaces[ SURFACE_SUBSET_BOUNDARY ] = surf_subset_boundary_soup.get();
    trisoup_surfaces[ SURFACE_FAULT ]           = surf_faults_soup.get();
    for( int i=0; i<SURFACE_N; i++ ) {
        if( trisoup_surfaces[i] != NULL ) {
            any_trisoup_surfaces = true;
        }
    }

    if( !(any_indexed_surfaces || any_trisoup_surfaces) ) {
        // nothing to do
        return;
    }


    if( polygon_set->polygonCount() == 0 ) {
        for( int i=0; i< SURFACE_N; i++ ) {
            if( indexed_surfaces[i] != NULL ) {
                indexed_surfaces[i]->setTriangleCount( 0 );
            }
        }
        return;
    }

    bool redo_meta = true;
    bool redo_triangulate = true;
    for( uint k=0; k<3 &&(redo_meta || redo_triangulate); k++ ) {

        // --- try to extract all triangles that should be part of the surfaces
        if( redo_meta ) {
            runMetaPass( polygon_set, subset, flip_faces );
            redo_meta = false;
            redo_triangulate = true;
        }

        // --- then, triangulate all the surfaces
        if( redo_triangulate ) {
            if( any_indexed_surfaces ) {
                runIndexedTriangulatePasses( indexed_surfaces, polygon_set, vertex_positions );
            }
            if( any_trisoup_surfaces ) {
                runTriSoupTriangulatePasses( trisoup_surfaces, polygon_set, vertex_positions );
            }
            redo_triangulate = false;
        }

        redo_meta = resizeMetabufferIfNeeded();
        
        // --- check if triangle buffers were large enough
        if( any_indexed_surfaces ) {
            for( int i=0; i< SURFACE_N; i++ ) {
                if( indexed_surfaces[i] != NULL ) {
                    GLuint result;
                    glGetQueryObjectuiv( indexed_surfaces[i]->indicesCountQuery(),
                                         GL_QUERY_RESULT,
                                         &result );
                    if( indexed_surfaces[i]->setTriangleCount( result ) ) {
                        LOGGER_DEBUG( log, "Triangle buffer " << i
                                      << " resized to accomodate " << result
                                      << " triangles." );
                        redo_triangulate = true;
                    }
                    else {
                        LOGGER_DEBUG( log, "Triangle surface " << i << " has " << result << " triangles" );
                    }
                }
            }
        }
        if( any_trisoup_surfaces ) {

            for( int i=0; i< SURFACE_N; i++ ) {
                if( trisoup_surfaces[i] != NULL ) {
                    GLuint result;
                    glGetQueryObjectuiv( trisoup_surfaces[i]->primitiveCountQuery(),
                                         GL_QUERY_RESULT,
                                         &result );

                    LOGGER_DEBUG( log, result );
                    if( trisoup_surfaces[i]->setTriangleCount( result ) ) {
                        LOGGER_DEBUG( log, "Triangle buffer " << i
                                      << " resized to accomodate " << result
                                      << " triangles." );
                        redo_triangulate = true;
                    }
                    else {
                        LOGGER_DEBUG( log, "Triangle surface " << i << " has " << result << " triangles" );
                    }
                }
            }

        }
    }
}

void
GridTessSurfBuilder::runMetaPass( boost::shared_ptr<const mesh::PolygonSetInterface>  polygon_set,
                                  boost::shared_ptr<const subset::Representation>     subset,
                                  bool                                                flip_faces)
{

    // layout(binding=0)   uniform usamplerBuffer  cell_subset;
    glActiveTexture( GL_TEXTURE0 );
    if( subset.get() != NULL ) {
        glBindTexture( GL_TEXTURE_BUFFER, subset->subsetAsTexture() );
    }
    else {
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
    }
    
    // layout(location=0) in [uint|uint2]  cell_ix;
    // layout(location=1) in uint          offset_a;
    // layout(location=2) in uint          offset_b;
    glBindVertexArray( polygon_set->polygonVertexArray() );

    if( polygon_set->polygonCellCount() == 1 ) {
        glUseProgram( m_meta1_prog.get() );
        glUniform1i( m_meta1_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );
    }
    else if( polygon_set->polygonCellCount() == 2 ) {
        glUseProgram( m_meta2_prog.get() );
        glUniform1i( m_meta2_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );
    }
    else {
        throw std::logic_error( "Illegal polygon cell count" );
    }
        
    // layout(stream=0) out uvec4  meta_subset;
    // layout(stream=1) out uvec4  meta_boundary;
    // layout(stream=2) out uvec4  meta_fault;
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, m_meta_xfb.get() );
    glBeginTransformFeedback( GL_POINTS );
    for( uint i=0; i<SURFACE_N; i++ ) {
        
        // We use GL_PRIMITIVES_GENERATED and not
        // GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN since we are
        // interested in the full count since we want to detect buffer
        // overflows.
        glBeginQueryIndexed( GL_PRIMITIVES_GENERATED, i, m_meta_query[i].get() );
    }

    glEnable( GL_RASTERIZER_DISCARD );
    glDrawArrays( GL_POINTS, 0, polygon_set->polygonCount() );
    glDisable( GL_RASTERIZER_DISCARD );

    for( uint i=0; i<SURFACE_N; i++ ) {
        glEndQueryIndexed( GL_PRIMITIVES_GENERATED, i );
    }
    glEndTransformFeedback();
 
    glBindVertexArray( 0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
}

void
GridTessSurfBuilder::drawMetaStream( int index )
{
#if 0
            // The following call seems to invoke 'GL_INVALID_OPERATION
            // error generated. Transform feedback object not valid for
            // draw.'. Not sure why. If replaced with drawArrays using
            // count from queries, we get no error. Result seems to be
            // sane regardless of error.
            glDrawTransformFeedbackStream( GL_POINTS, m_meta_xfb.get(), index );
#else
            GLuint count;
            glGetQueryObjectuiv( m_meta_query[index].get(), GL_QUERY_RESULT, &count );
            glDrawArrays( GL_POINTS, 0, std::min( m_meta_buf_N[index], (GLsizei)count ) );

            //Logger log = getLogger( "foo");
            //LOGGER_DEBUG( log, "meta_count = " << count );
#endif
}

bool
GridTessSurfBuilder::resizeMetabufferIfNeeded()
{
    Logger log = getLogger( "GridTessSurfBuilder.resizeMetabufferIfNeeded" );

    bool was_resized = false;
    for(int i=0; i<SURFACE_N; i++ ) {
        GLuint polygon_count;

        glGetQueryObjectuiv( m_meta_query[i].get(),
                             GL_QUERY_RESULT,
                             &polygon_count );
        if( m_meta_buf_N[i] < (GLsizei)(polygon_count) ) {
            m_meta_buf_N[i] = 1.1f*polygon_count;
            glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_meta_buf[i].get() );
            glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                          sizeof(unsigned int)*4*m_meta_buf_N[i],
                          NULL,
                          GL_DYNAMIC_COPY );
            glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );
            was_resized = true;
            LOGGER_DEBUG( log, "Meta buffer " << i
                          << " resized to accomodate " << polygon_count
                          << " polygons." );
        }
    }
    return was_resized;
}


void
GridTessSurfBuilder::runIndexedTriangulatePasses( GridTessSurf** surfaces,
                                           boost::shared_ptr<const mesh::PolygonSetInterface>  polygon_set,
                                           boost::shared_ptr<const mesh::VertexPositionInterface> vertex_positions )
{

    // Late shader builing; we don't know the max output until the tessellation
    // has been built.
    if(  m_triangulate_indexed_count < polygon_set->polygonMaxPolygonSize() ) {
        rebuildIndexedTriangulationProgram( polygon_set->polygonMaxPolygonSize() );
    }
    
    glEnable( GL_RASTERIZER_DISCARD );
    glUseProgram( m_triangulate_indexed_prog.get() );
    glActiveTexture( GL_TEXTURE1 );  glBindTexture( GL_TEXTURE_BUFFER, polygon_set->polygonNormalIndexTexture() );
    glActiveTexture( GL_TEXTURE2 );  glBindTexture( GL_TEXTURE_BUFFER, polygon_set->polygonVertexIndexTexture() );
    glActiveTexture( GL_TEXTURE3 );  glBindTexture( GL_TEXTURE_BUFFER, vertex_positions->vertexPositionsAsBufferTexture() );
    

    for( int i=0; i<SURFACE_N; i++ ) {
        if( surfaces[i] != NULL ) {
            glBindVertexArray( m_meta_vao[ i ].get() );
            glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, surfaces[i]->indicesTransformFeedbackObject() );
            glBeginQuery( GL_PRIMITIVES_GENERATED, surfaces[i]->indicesCountQuery() );
            glBeginTransformFeedback( GL_POINTS );
            drawMetaStream( i );
            glEndTransformFeedback( );
            glEndQuery( GL_PRIMITIVES_GENERATED );
        }
    }

    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
    glActiveTexture( GL_TEXTURE3 );  glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE2 );  glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE1 );  glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glDisable( GL_RASTERIZER_DISCARD );
    glUseProgram( 0 );
    glBindVertexArray( 0 );
}    

void
GridTessSurfBuilder::runTriSoupTriangulatePasses( TriangleSoup** surfaces,
                                           boost::shared_ptr<const mesh::PolygonSetInterface>  polygon_set,
                                           boost::shared_ptr<const mesh::VertexPositionInterface> vertex_positions )
{
    if(  m_triangulate_trisoup_count < polygon_set->polygonMaxPolygonSize() ) {
        rebuildTriSoupTriangulationProgram( polygon_set->polygonMaxPolygonSize() );
    }
    glEnable( GL_RASTERIZER_DISCARD );
    glUseProgram( m_triangulate_trisoup_prog.get() );
    glActiveTexture( GL_TEXTURE1 );  glBindTexture( GL_TEXTURE_BUFFER, polygon_set->polygonNormalIndexTexture() );
    glActiveTexture( GL_TEXTURE2 );  glBindTexture( GL_TEXTURE_BUFFER, polygon_set->polygonVertexIndexTexture() );
    glActiveTexture( GL_TEXTURE3 );  glBindTexture( GL_TEXTURE_BUFFER, vertex_positions->vertexPositionsAsBufferTexture() );

    for( int i=0; i<SURFACE_N; i++ ) {
        if( surfaces[i] != NULL ) {
            glBindVertexArray( m_meta_vao[ i ].get() );
            glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, surfaces[i]->vertexAttributesTransformFeedback() );
            glBeginQuery( GL_PRIMITIVES_GENERATED, surfaces[i]->primitiveCountQuery() );
            glBeginTransformFeedback( GL_TRIANGLES );
            drawMetaStream( i );
            glEndTransformFeedback( );
            glEndQuery( GL_PRIMITIVES_GENERATED );
        }
    }

    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
    glActiveTexture( GL_TEXTURE3 );  glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE2 );  glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE1 );  glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glDisable( GL_RASTERIZER_DISCARD );
    glUseProgram( 0 );
    glBindVertexArray( 0 );


}

} // of namespace surface
} // of namespace render
