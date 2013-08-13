/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <siut2/gl_utils/GLSLtools.hpp>

#include "Logger.hpp"
#include "GridTess.hpp"
#include "GridTessSubset.hpp"
#include "GridTessSurf.hpp"
#include "GridTessSurfBuilder.hpp"

namespace resources {
    extern const std::string triangulate_poly_vs;
    extern const std::string triangulate_poly_gs;
    extern const std::string extract_faces_vs;
    extern const std::string extract_faces_gs;
}

namespace render {

GridTessSurfBuilder::GridTessSurfBuilder()
    : m_meta_prog( "GridTessSurfBuilder.m_meta_prog"),
      m_triangulate_count( 0 ),
      m_triangulate_prog( "GridTessSurfBuilder.m_triangulate_prog" )
{
    using siut2::gl_utils::compileShader;

    // -------------------------------------------------------------------------
    // create polygon extract shader
    // -------------------------------------------------------------------------
    GLuint faces_vs = compileShader( resources::extract_faces_vs,
                                     GL_VERTEX_SHADER, true );

    GLuint faces_gs = compileShader( resources::extract_faces_gs,
                                     GL_GEOMETRY_SHADER, true );


    static const char* faces_feedback[] = { "meta_subset",
                                            "meta_boundary",
                                            "meta_fault" };
    glAttachShader( m_meta_prog.get(), faces_vs );
    glAttachShader( m_meta_prog.get(), faces_gs );
    glTransformFeedbackVaryings( m_meta_prog.get(), 3, faces_feedback, GL_SEPARATE_ATTRIBS );
    siut2::gl_utils::linkProgram( m_meta_prog.get() );
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
    m_triangulate_count = max_vertices;
    m_triangulate_prog.reset();

    static const char* triangle_feedback[2] = {
        "cell",
        "indices"
    };

    GLsizei max_out = std::max( (GLsizei)3, max_vertices )-2;
    std::stringstream o;
    o << "#define MAX_OUT " << max_out << "\n";
    GLuint poly_vs = siut2::gl_utils::compileShader( resources::triangulate_poly_vs, GL_VERTEX_SHADER, true );
    GLuint poly_subset_gs = siut2::gl_utils::compileShader( o.str() +
                                                            resources::triangulate_poly_gs,
                                                            GL_GEOMETRY_SHADER, true );
    glAttachShader( m_triangulate_prog.get(), poly_vs );
    glAttachShader( m_triangulate_prog.get(), poly_subset_gs );
    glTransformFeedbackVaryings( m_triangulate_prog.get(), 2, triangle_feedback, GL_SEPARATE_ATTRIBS );
    siut2::gl_utils::linkProgram( m_triangulate_prog.get() );
    glDeleteShader( poly_vs );
    glDeleteShader( poly_subset_gs );
}


void
GridTessSurfBuilder::buildSurfaces( boost::shared_ptr<GridTessSurf> surf_subset,
                                    boost::shared_ptr<GridTessSurf> surf_subset_boundary,
                                    boost::shared_ptr<GridTessSurf> surf_faults,
                                    boost::shared_ptr<const GridTessSubset> subset,
                                    boost::shared_ptr<const GridTess> tess,
                                    bool                     flip_faces )
{
    Logger log = getLogger( "GridTessSurfBuilder.buildSurfaces" );
    GridTessSurf* surfaces[ SURFACE_N ];
    surfaces[ SURFACE_SUBSET          ] = surf_subset.get();
    surfaces[ SURFACE_SUBSET_BOUNDARY ] = surf_subset_boundary.get();
    surfaces[ SURFACE_FAULT ]           = surf_faults.get();

    if( tess->polygonCount() == 0 ) {
        for( int i=0; i< SURFACE_N; i++ ) {
            if( surfaces[i] != NULL ) {
                surfaces[i]->setTriangleCount( 0 );
            }
        }
        return;
    }

    // Late shader builing; we don't know the max output until the tessellation
    // has been built.
    if( m_triangulate_count != tess->polygonMaxPolygonSize() ) {
        rebuildTriangulationProgram( tess->polygonMaxPolygonSize() );
    }



    // Find all polygons that we want to render

    glActiveTexture( GL_TEXTURE0 );
    if( subset.get() != NULL ) {
        glBindTexture( GL_TEXTURE_BUFFER, subset->subsetAsTexture() );
    }
    else {
        glBindTexture( GL_TEXTURE_BUFFER, 0 );
    }
    glActiveTexture( GL_TEXTURE1 );  glBindTexture( GL_TEXTURE_BUFFER, tess->polygonNormalIndexTexture() );
    glActiveTexture( GL_TEXTURE2 );  glBindTexture( GL_TEXTURE_BUFFER, tess->polygonVertexIndexTexture() );
    glActiveTexture( GL_TEXTURE3 );  glBindTexture( GL_TEXTURE_BUFFER, tess->vertexPositionsAsBufferTexture() );

    glEnable( GL_RASTERIZER_DISCARD );

    bool redo_meta = true;
    bool redo_triangulate = true;
    GLuint polygons_count[SURFACE_N];
    for( uint k=0; k<3 &&(redo_meta || redo_triangulate); k++ ) {

        // --- try to extract all triangles that should be part of the surfaces
        if( redo_meta ) {
            glBindVertexArray( tess->polygonVertexArray() );
            glUseProgram( m_meta_prog.get() );
            glUniform1i( m_meta_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );

            //glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, m_meta_counters );
            //glClearBufferData( GL_ATOMIC_COUNTER_BUFFER,
            //                   GL_R32UI,
            //                   GL_RED,
            //                   GL_UNSIGNED_INT,
            //                   NULL );
            //glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, 0 );
            //glBindBufferBase( GL_ATOMIC_COUNTER_BUFFER, 0, m_meta_counters );

            glBindTransformFeedback( GL_TRANSFORM_FEEDBACK, m_meta_xfb.get() );
            glBeginTransformFeedback( GL_POINTS );
            for( uint i=0; i<SURFACE_N; i++ ) {
                glBeginQueryIndexed( GL_PRIMITIVES_GENERATED, i, m_meta_query[i].get() );
            }
            glDrawArrays( GL_POINTS, 0, tess->polygonCount() );
            for( uint i=0; i<SURFACE_N; i++ ) {
                glEndQueryIndexed( GL_PRIMITIVES_GENERATED, i );
            }
            glEndTransformFeedback();
            redo_meta = false;
            redo_triangulate = true;

            //std::vector<GLuint> foo(6);
            //glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, m_meta_counters );
            //glGetBufferSubData( GL_ATOMIC_COUNTER_BUFFER,
            //                    0,
            //                    24,
            //                    foo.data() );
            //glBindBuffer( GL_ATOMIC_COUNTER_BUFFER, 0 );
            //std::cerr << "FOO: "
            //          << foo[0] << "\t"
            //          << foo[1] << "\t"
            //          << foo[2] << "\t"
            //          << foo[3] << "\t"
            //          << foo[4] << "\t"
            //          << foo[5] << "\n";
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
                    glDrawTransformFeedbackStream( GL_POINTS, m_meta_xfb.get(), i );
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
            if( m_meta_buf_N[i] < polygons_count[i] ) {
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

} // of namespace render
