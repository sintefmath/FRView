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
#include "render/mesh/PolyhedralRepresentation.hpp"
#include "render/subset/Representation.hpp"
#include "render/surface/GridTessSurf.hpp"
#include "render/surface/GridTessSurfBuilder.hpp"
#include <sstream>

namespace resources {
}

namespace render {
    namespace  surface {
        namespace glsl {
            extern const std::string GridTessSurfBuilder_triangulate_vs;
            extern const std::string GridTessSurfBuilder_triangulate_gs;
            extern const std::string GridTessSurfBuilder_extract_vs;
            extern const std::string GridTessSurfBuilder_extract_gs;
        }

GridTessSurfBuilder::GridTessSurfBuilder()
    : m_meta_prog( "GridTessSurfBuilder.m_meta_prog"),
      m_triangulate_count( 0 ),
      m_triangulate_prog( "GridTessSurfBuilder.m_triangulate_prog" )
{
    Logger log = getLogger( "render.GridTessSurfBuilder.constructor" );

    // -------------------------------------------------------------------------
    // create polygon extract shader
    // -------------------------------------------------------------------------
    GLuint faces_vs = utils::compileShader( log, glsl::GridTessSurfBuilder_extract_vs,
                                     GL_VERTEX_SHADER );

    GLuint faces_gs = utils::compileShader( log, glsl::GridTessSurfBuilder_extract_gs,
                                     GL_GEOMETRY_SHADER );


    static const char* faces_feedback[] = { "meta_subset",
                                            "meta_boundary",
                                            "meta_fault" };
    glAttachShader( m_meta_prog.get(), faces_vs );
    glAttachShader( m_meta_prog.get(), faces_gs );
    glTransformFeedbackVaryings( m_meta_prog.get(), 3, faces_feedback, GL_SEPARATE_ATTRIBS );
    utils::linkProgram( log, m_meta_prog.get() );
    m_meta_loc_flip = glGetUniformLocation( m_meta_prog.get(), "flip_faces" );
    glDeleteShader( faces_vs );
    glDeleteShader( faces_gs );


    /*GLint counter_size;
    glGetActiveAtomicCounterBufferiv( m_meta_prog, 0, GL_ATOMIC_COUNTER_BUFFER_DATA_SIZE, &counter_size );

    glGenBuffers( 1, &m_meta_counters );
    glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, m_meta_counters );
    glBufferData( GL_ATOMIC_COUNTER_BUFFER,
                  counter_size,
                  NULL,
                  GL_STREAM_COPY );
    glClearBufferData( GL_ATOMIC_COUNTER_BUFFER,
                       GL_R32UI,
                       GL_RED,
                       GL_UNSIGNED_INT,
                       NULL );
    glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, 0 );

    std::cerr << "!!!!\n";
*/
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
GridTessSurfBuilder::rebuildTriangulationProgram( GLsizei max_vertices )
{
    Logger log = getLogger( "render.GridTessSurfBuilder.rebuildTriangulationProgram" );

    m_triangulate_count = max_vertices;
    m_triangulate_prog.reset();

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
                                                  glsl::GridTessSurfBuilder_triangulate_gs,
                                                  GL_GEOMETRY_SHADER );
    glAttachShader( m_triangulate_prog.get(), poly_vs );
    glAttachShader( m_triangulate_prog.get(), poly_subset_gs );
    glTransformFeedbackVaryings( m_triangulate_prog.get(), 2, triangle_feedback, GL_SEPARATE_ATTRIBS );
    utils::linkProgram( log, m_triangulate_prog.get() );
    glDeleteShader( poly_vs );
    glDeleteShader( poly_subset_gs );
}


void
GridTessSurfBuilder::buildSurfaces(boost::shared_ptr<GridTessSurf> surf_subset,
                                    boost::shared_ptr<GridTessSurf> surf_subset_boundary,
                                    boost::shared_ptr<GridTessSurf> surf_faults,
                                    boost::shared_ptr<const subset::Representation> subset,
                                    boost::shared_ptr<const mesh::PolyhedralRepresentation> mesh,
                                    bool                     flip_faces )
{
    Logger log = getLogger( "GridTessSurfBuilder.buildSurfaces" );
    GridTessSurf* surfaces[ SURFACE_N ];
    surfaces[ SURFACE_SUBSET          ] = surf_subset.get();
    surfaces[ SURFACE_SUBSET_BOUNDARY ] = surf_subset_boundary.get();
    surfaces[ SURFACE_FAULT ]           = surf_faults.get();

    if( mesh->polygonCount() == 0 ) {
        for( int i=0; i< SURFACE_N; i++ ) {
            if( surfaces[i] != NULL ) {
                surfaces[i]->setTriangleCount( 0 );
            }
        }
        return;
    }

    // Late shader builing; we don't know the max output until the tessellation
    // has been built.
    if( m_triangulate_count != mesh->polygonMaxPolygonSize() ) {
        rebuildTriangulationProgram( mesh->polygonMaxPolygonSize() );
    }



    // Find all polygons that we want to render

    glActiveTexture( GL_TEXTURE0 );
    if( subset.get() != NULL ) {
        glBindTexture( GL_TEXTURE_BUFFER, subset->subsetAsTexture() );
    }
    else {
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
    }
    glActiveTexture( GL_TEXTURE1 );  glBindTexture( GL_TEXTURE_BUFFER, mesh->polygonNormalIndexTexture() );
    glActiveTexture( GL_TEXTURE2 );  glBindTexture( GL_TEXTURE_BUFFER, mesh->polygonVertexIndexTexture() );
    glActiveTexture( GL_TEXTURE3 );  glBindTexture( GL_TEXTURE_BUFFER, mesh->vertexPositionsAsBufferTexture() );

    glEnable( GL_RASTERIZER_DISCARD );

    bool redo_meta = true;
    bool redo_triangulate = true;
    GLuint polygons_count[SURFACE_N];
    for( uint k=0; k<3 &&(redo_meta || redo_triangulate); k++ ) {

        // --- try to extract all triangles that should be part of the surfaces
        if( redo_meta ) {
            glBindVertexArray( mesh->polygonVertexArray() );
            glUseProgram( m_meta_prog.get() );
            glUniform1i( m_meta_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );

            glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, m_meta_xfb.get() );
            glBeginTransformFeedback( GL_POINTS );
            for( uint i=0; i<SURFACE_N; i++ ) {
                // We use GL_PRIMITIVES_GENERATED and not
                // GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN since we are
                // interested in the full count since we want to detect buffer
                // overflows.
                glBeginQueryIndexed( GL_PRIMITIVES_GENERATED, i, m_meta_query[i].get() );
            }
            glDrawArrays( GL_POINTS, 0, mesh->polygonCount() );
            for( uint i=0; i<SURFACE_N; i++ ) {
                glEndQueryIndexed( GL_PRIMITIVES_GENERATED, i );
            }
            glEndTransformFeedback();
            redo_meta = false;
            redo_triangulate = true;

        }

        // --- then, triangulate all the surfaces
        if( redo_triangulate ) {
            glUseProgram( m_triangulate_prog.get() );

            for( int i=0; i< SURFACE_N; i++ ) {
                if( surfaces[i] != NULL ) {
                    glBindVertexArray( m_meta_vao[ i ].get() );
                    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, surfaces[i]->indicesTransformFeedbackObject() );
                    glBeginQuery( GL_PRIMITIVES_GENERATED, surfaces[i]->indicesCountQuery() );
                    glBeginTransformFeedback( GL_POINTS );
                    
#if 0
                    // The following call seems to invoke 'GL_INVALID_OPERATION
                    // error generated. Transform feedback object not valid for
                    // draw.'. Not sure why. If replaced with drawArrays using
                    // count from queries, we get no error. Result seems to be
                    // sane regardless of error.
                    glDrawTransformFeedbackStream( GL_POINTS, m_meta_xfb.get(), i );
#else
                    GLuint count;
                    glGetQueryObjectuiv( m_meta_query[i].get(), GL_QUERY_RESULT, &count );
                    glDrawArrays( GL_POINTS, 0, std::min( m_meta_buf_N[i], (GLsizei)count ) );
#endif
                    
                    glEndTransformFeedback( );
                    glEndQuery( GL_PRIMITIVES_GENERATED );
                }
            }
            redo_triangulate = false;
        }

        // --- check if meta buffers were large enough
        for(int i=0; i<SURFACE_N; i++ ) {
            glGetQueryObjectuiv( m_meta_query[i].get(),
                                 GL_QUERY_RESULT,
                                 &polygons_count[i] );
            if( m_meta_buf_N[i] < (GLsizei)(polygons_count[i]) ) {
                m_meta_buf_N[i] = 1.1f*polygons_count[i];
                glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_meta_buf[i].get() );
                glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                              sizeof(unsigned int)*4*m_meta_buf_N[i],
                              NULL,
                              GL_DYNAMIC_COPY );
                glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );
                redo_meta = true;
                LOGGER_DEBUG( log, "Meta buffer " << i
                              << " resized to accomodate " << polygons_count[i]
                              << " polygons." );
            }
        }
        // --- check if triangle buffers were large enough
        for( int i=0; i< SURFACE_N; i++ ) {
            if( surfaces[i] != NULL ) {
                GLuint result;
                glGetQueryObjectuiv( surfaces[i]->indicesCountQuery(),
                                     GL_QUERY_RESULT,
                                     &result );
                if( surfaces[i]->setTriangleCount( result ) ) {
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
    glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, 0 );
    glActiveTexture( GL_TEXTURE3 );  glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE2 );  glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE1 );  glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE0 );  glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glDisable( GL_RASTERIZER_DISCARD );
    glUseProgram( 0 );
    glBindVertexArray( 0 );
}

    } // of namespace surface
} // of namespace render
