/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <siut2/gl_utils/GLSLtools.hpp>

#include "GridTess.hpp"
#include "GridTessSubset.hpp"
#include "GridTessSurf.hpp"
#include "GridTessSurfBuilder.hpp"

namespace resources {
    extern const std::string compact_common_gs;
    extern const std::string compact_vs;
    extern const std::string compact_subset_gs;
    extern const std::string compact_subset_boundary_gs;
    extern const std::string compact_fault_gs;
    extern const std::string compact_boundary_gs;
    extern const std::string compact_edges_vs;
    extern const std::string compact_edges_common_gs;

    extern const std::string compact_poly_vs;
    extern const std::string compact_poly_common_gs;

    extern const std::string compact_meta_vs;
    extern const std::string compact_meta_gs;
}

static const char* triangle_feedback[2] = {
    "cell",
    "indices"
};
static const char* edge_feedback[1] = {
    "cornerpoint_ix"
};

GridTessSurfBuilder::GridTessSurfBuilder()
{


    GLuint vs                 = siut2::gl_utils::compileShader( resources::compact_vs, GL_VERTEX_SHADER, true );
    GLuint subset_gs          = siut2::gl_utils::compileShader( resources::compact_common_gs + resources::compact_subset_gs, GL_GEOMETRY_SHADER, true );
    GLuint subset_boundary_gs = siut2::gl_utils::compileShader( resources::compact_common_gs + resources::compact_subset_boundary_gs, GL_GEOMETRY_SHADER, true );
    GLuint fault_gs           = siut2::gl_utils::compileShader( resources::compact_common_gs + resources::compact_fault_gs, GL_GEOMETRY_SHADER, true );
    GLuint boundary_gs        = siut2::gl_utils::compileShader( resources::compact_common_gs + resources::compact_boundary_gs, GL_GEOMETRY_SHADER, true );
    GLuint edge_vs            = siut2::gl_utils::compileShader( resources::compact_edges_vs, GL_VERTEX_SHADER, true );
    GLuint edge_subset_gs     = siut2::gl_utils::compileShader( "#define COMPACT_SUBSET\n" + resources::compact_edges_common_gs, GL_GEOMETRY_SHADER, true );
    GLuint edge_fault_gs      = siut2::gl_utils::compileShader( "#define COMPACT_FAULT\n" + resources::compact_edges_common_gs, GL_GEOMETRY_SHADER, true );
    GLuint edge_boundary_gs   = siut2::gl_utils::compileShader( "#define COMPACT_BOUNDARY\n" + resources::compact_edges_common_gs, GL_GEOMETRY_SHADER, true );

    m_pipeline[ PIPELINE_COMPACT_SUBSET ].m_edge_program = glCreateProgram();
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_SUBSET ].m_edge_program, edge_vs );
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_SUBSET ].m_edge_program, edge_subset_gs );
    m_pipeline[ PIPELINE_COMPACT_SUBSET ].m_triangle_program = glCreateProgram();
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_SUBSET ].m_triangle_program, vs );
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_SUBSET ].m_triangle_program, subset_gs );

    m_pipeline[ PIPELINE_COMPACT_SUBSET_BOUNDARY ].m_edge_program = glCreateProgram();
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_SUBSET_BOUNDARY ].m_edge_program, edge_vs );
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_SUBSET_BOUNDARY ].m_edge_program, edge_subset_gs );
    m_pipeline[ PIPELINE_COMPACT_SUBSET_BOUNDARY ].m_triangle_program = glCreateProgram();
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_SUBSET_BOUNDARY ].m_triangle_program, vs );
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_SUBSET_BOUNDARY ].m_triangle_program, subset_boundary_gs );

    m_pipeline[ PIPELINE_COMPACT_FAULT ].m_edge_program = glCreateProgram();
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_FAULT ].m_edge_program, edge_vs );
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_FAULT ].m_edge_program, edge_fault_gs );
    m_pipeline[ PIPELINE_COMPACT_FAULT ].m_triangle_program = glCreateProgram();
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_FAULT ].m_triangle_program, vs );
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_FAULT ].m_triangle_program, fault_gs );

    m_pipeline[ PIPELINE_COMPACT_BOUNDARY ].m_edge_program = glCreateProgram();
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_BOUNDARY ].m_edge_program, edge_vs );
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_BOUNDARY ].m_edge_program, edge_boundary_gs );
    m_pipeline[ PIPELINE_COMPACT_BOUNDARY ].m_triangle_program = glCreateProgram();
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_BOUNDARY ].m_triangle_program, vs );
    glAttachShader( m_pipeline[ PIPELINE_COMPACT_BOUNDARY ].m_triangle_program, boundary_gs );

    for( unsigned int i=0; i<PIPELINE_COMPACT_N; i++ ) {
        glTransformFeedbackVaryings( m_pipeline[ i ].m_edge_program, 1, edge_feedback, GL_SEPARATE_ATTRIBS );
        siut2::gl_utils::linkProgram( m_pipeline[ i ].m_edge_program );

        glTransformFeedbackVaryings( m_pipeline[ i ].m_triangle_program, 2, triangle_feedback, GL_SEPARATE_ATTRIBS );
        siut2::gl_utils::linkProgram( m_pipeline[ i ].m_triangle_program );
        m_pipeline[ i ].m_triangle_loc_flip = glGetUniformLocation( m_pipeline[ i ].m_triangle_program, "flip_faces" );
    }

    m_polygon_prog = 0;

    GLuint meta_vs = siut2::gl_utils::compileShader( resources::compact_meta_vs,
                                                     GL_VERTEX_SHADER, true );

    GLuint meta_subset_gs = siut2::gl_utils::compileShader( resources::compact_meta_gs,
                                                           GL_GEOMETRY_SHADER, true );
    static const char* meta_feedback[] = { "meta_subset", "meta_boundary", "meta_fault" };
    m_meta_prog = glCreateProgram();
    glAttachShader( m_meta_prog, meta_vs );
    glAttachShader( m_meta_prog, meta_subset_gs );
    glTransformFeedbackVaryings( m_meta_prog, 1, meta_feedback, GL_SEPARATE_ATTRIBS );
    siut2::gl_utils::linkProgram( m_meta_prog );
    m_meta_loc_flip = glGetUniformLocation( m_meta_prog, "flip_faces" );

    m_meta_buf_N = 1024;
    glGenBuffers( 1, &m_meta_buf );
    glBindBuffer( GL_ARRAY_BUFFER, m_meta_buf );
    glBufferData( GL_ARRAY_BUFFER,
                  sizeof(unsigned int)*4*m_meta_buf_N,
                  NULL,
                  GL_DYNAMIC_COPY );


    glGenVertexArrays( 1, &m_meta_vao );
    glBindVertexArray( m_meta_vao );
    glVertexAttribIPointer( 0, 4, GL_UNSIGNED_INT, 0, NULL );
    glEnableVertexAttribArray( 0 );

    glBindVertexArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER,0 );

    glGenQueries( SURFACE_N, m_meta_query );


    glDeleteShader( vs );
    glDeleteShader( subset_gs );
    glDeleteShader( subset_boundary_gs );
    glDeleteShader( fault_gs );
    glDeleteShader( boundary_gs );
    glDeleteShader( edge_vs );
    glDeleteShader( edge_subset_gs );
    CHECK_GL;
}

GridTessSurfBuilder::~GridTessSurfBuilder()
{
    for( unsigned int i=0; i<PIPELINE_COMPACT_N; i++ ) {
        glDeleteProgram( m_pipeline[i].m_triangle_program );
        glDeleteProgram( m_pipeline[i].m_edge_program );
    }
}

void
GridTessSurfBuilder::buildSubsetSurface( GridTessSurf*          surf,
                                         const GridTessSubset*  subset,
                                         const GridTess*        tess,
                                         bool                   flip_faces,
                                         bool                   geometric_edges )
{
#ifdef GPU_TRIANGULATOR
    if( tess->polygonCount() == 0 ) {
        return;
    }
    // Late shader builing; we don't know the max output until the tessellation
    // has been built.
    if( m_polygon_prog == 0 ) {
        GLsizei max_out = std::max( (GLsizei)3, tess->polygonMaxPolygonSize() )-2;
        std::stringstream o;
        o << "#define MAX_OUT " << max_out << "\n";
        GLuint poly_vs = siut2::gl_utils::compileShader( resources::compact_poly_vs, GL_VERTEX_SHADER, true );
        GLuint poly_subset_gs = siut2::gl_utils::compileShader( o.str() +
                                                                resources::compact_poly_common_gs,
                                                                GL_GEOMETRY_SHADER, true );
        m_polygon_prog = glCreateProgram();
        glAttachShader( m_polygon_prog, poly_vs );
        glAttachShader( m_polygon_prog, poly_subset_gs );
        glTransformFeedbackVaryings( m_polygon_prog, 2, triangle_feedback, GL_SEPARATE_ATTRIBS );
        siut2::gl_utils::linkProgram( m_polygon_prog );
        m_polygon_loc_flip = glGetUniformLocation( m_polygon_prog, "flip_faces" );
    }

    // Find all polygons that we want to render
    glBindVertexArray( tess->polygonVertexArray() );
    glUseProgram( m_meta_prog );
    glUniform1i( m_meta_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );
    glActiveTexture( GL_TEXTURE0 );
    if( subset != NULL ) {
        glBindTexture( GL_TEXTURE_BUFFER, subset->subsetAsTexture() );
    }
    GLsizei polygons;
    for( uint i=0; i<2; i++ ) {

        glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_meta_buf );

        glBeginTransformFeedback( GL_POINTS );
        for( uint i=0; i<SURFACE_N; i++ ) {
            glBeginQueryIndexed( GL_PRIMITIVES_GENERATED, i, m_meta_query[i] );
        }
        glDrawArrays( GL_POINTS, 0, tess->polygonCount() );
        for( uint i=0; i<SURFACE_N; i++ ) {
            glEndQueryIndexed( GL_PRIMITIVES_GENERATED, i );
        }
        glEndTransformFeedback();

        for( uint i=0; i<SURFACE_N; i++ ) {
            GLuint result;
            glGetQueryObjectuiv( m_meta_query[i],
                                GL_QUERY_RESULT,
                                &result );
            std::cerr << result << "\t";
        }
        std::cerr << "\n";

        GLuint result;
        glGetQueryObjectuiv( m_meta_query[0],
                            GL_QUERY_RESULT,
                            &result );
        polygons = result;
        if( polygons <= m_meta_buf_N ) {
            break;
        }
        else {
            m_meta_buf_N = 1.1f*polygons;
            glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_meta_buf );
            glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                          sizeof(unsigned int)*4*m_meta_buf_N,
                          NULL,
                          GL_DYNAMIC_COPY );
            glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );
        }
    }

    // Then, triangulate all selected polygons
    glBindVertexArray( m_meta_vao );
    glUseProgram( m_polygon_prog );
    glUniform1i( m_polygon_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->polygonNormalIndexTexture() );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->polygonVertexIndexTexture() );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_BUFFER, tess->vertexPositionsAsBufferTexture() );

    surf->populatePolygonBuffer( tess, polygons );

    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );

    glBindVertexArray( 0 );
#else
    applyPipeline( m_pipeline[ PIPELINE_COMPACT_SUBSET ],
                   surf,
                   subset,
                   tess,
                   flip_faces,
                   geometric_edges );
#endif
    /*
    CHECK_GL;
    glBindVertexArray( tess->triangleVertexArray() );
    glUseProgram( m_compact_subset_prog );
    glUniform1i( m_compact_subset_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, subset->texture() );
    surf->populateTriangleBuffer( tess );
    CHECK_GL;

    if( geometric_edges ) {
        glBindVertexArray( tess->edgeIndexVertexArrayObject() );
        glUseProgram( m_pipeline[ PIPELINE_COMPACT_SUBSET ].m_edge_program );
        surf->populateEdgeBuffer( tess );
    }
    else {
        surf->clearEdgeBuffer();
    }
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glUseProgram( 0 );
    glBindVertexArray( 0 );

    CHECK_GL;
*/
}



void
GridTessSurfBuilder::buildSubsetBoundarySurface( GridTessSurf*          surf,
                                                 const GridTessSubset*  subset,
                                                 const GridTess*        tess,
                                                 bool                   flip_faces,
                                                 bool                   geometric_edges )
{
#ifdef GPU_TRIANGULATOR
    return;
#else
    applyPipeline( m_pipeline[ PIPELINE_COMPACT_SUBSET_BOUNDARY ],
                   surf,
                   subset,
                   tess,
                   flip_faces,
                   geometric_edges );
/*
    glBindVertexArray( tess->triangleVertexArray() );

    glUseProgram( m_compact_subset_boundary_prog );
    glUniform1i( m_compact_subset_boundary_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, subset->texture() );

    surf->populateTriangleBuffer( tess );
    CHECK_GL;

    if( geometric_edges ) {
        glBindVertexArray( tess->edgeIndexVertexArrayObject() );
        glUseProgram( m_pipeline[ PIPELINE_COMPACT_BOUNDARY ].m_edge_program );
        surf->populateEdgeBuffer( tess );
    }
    else {
        surf->clearEdgeBuffer();
    }

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glUseProgram( 0 );

    glBindVertexArray( 0 );

    surf->clearEdgeBuffer();

    CHECK_GL;
*/
#endif
}

void
GridTessSurfBuilder::buildFaultSurface( GridTessSurf*    surf,
                                        const GridTess*  tess,
                                        bool             flip_faces,
                                        bool             geometric_edges )
{
#ifdef GPU_TRIANGULATOR
    return;
#else
    applyPipeline( m_pipeline[ PIPELINE_COMPACT_FAULT ],
                   surf,
                   NULL,
                   tess,
                   flip_faces,
                   geometric_edges );
/*
    glBindVertexArray( tess->triangleVertexArray() );

    glUseProgram( m_compact_fault_prog );
    glUniform1i( m_compact_fault_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );

    surf->populateTriangleBuffer( tess );
    CHECK_GL;

    if( geometric_edges ) {
        glBindVertexArray( tess->edgeIndexVertexArrayObject() );
        glUseProgram( m_pipeline[ PIPELINE_COMPACT_FAULT ].m_edge_program );
        surf->populateEdgeBuffer( tess );
    }
    else {
        surf->clearEdgeBuffer();
    }
    glUseProgram( 0 );
    glBindVertexArray( 0 );

    surf->clearEdgeBuffer();
    CHECK_GL;
*/
#endif
}

void
GridTessSurfBuilder::buildBoundarySurface( GridTessSurf*    surf,
                                           const GridTess*  tess,
                                           bool             flip_faces,
                                           bool             geometric_edges )
{
#ifdef GPU_TRIANGULATOR
    return;
#else
    applyPipeline( m_pipeline[ PIPELINE_COMPACT_BOUNDARY ],
                   surf,
                   NULL,
                   tess,
                   flip_faces,
                   geometric_edges );
/*
    glBindVertexArray( tess->triangleVertexArray() );

    glUseProgram( m_compact_boundary_prog );
    glUniform1i( m_compact_boundary_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );

    surf->populateTriangleBuffer( tess );
    if( geometric_edges ) {
        glBindVertexArray( tess->edgeIndexVertexArrayObject() );
        glUseProgram( m_pipeline[ PIPELINE_COMPACT_BOUNDARY ].m_edge_program );
        surf->populateEdgeBuffer( tess );
    }
    else {
        surf->clearEdgeBuffer();
    }

    glUseProgram( 0 );
    glBindVertexArray( 0 );

    surf->clearEdgeBuffer();

    CHECK_GL;
*/
#endif
}


void
GridTessSurfBuilder::applyPipeline( PipelineData&          pipeline,
                                    GridTessSurf*          surf,
                                    const GridTessSubset*  subset,
                                    const GridTess*        tess,
                                    bool                   flip_faces,
                                    bool                   geometric_edges )
{

    CHECK_GL;
    glBindVertexArray( tess->triangleIndexTupleVertexArray() );
    glUseProgram( pipeline.m_triangle_program );
    glUniform1i( pipeline.m_triangle_loc_flip, flip_faces ? GL_TRUE : GL_FALSE );
    glActiveTexture( GL_TEXTURE0 );
    if( subset != NULL ) {
        glBindTexture( GL_TEXTURE_BUFFER, subset->subsetAsTexture() );
    }
    surf->populateTriangleBuffer( tess );
    glActiveTexture( GL_TEXTURE0 );
    if( subset != NULL ) {
        glBindTexture( GL_TEXTURE_BUFFER, subset->subsetAsTexture() );
    }
    if( geometric_edges ) {
        glBindVertexArray( tess->edgeIndexTupleVertexArray() );
        glUseProgram( pipeline.m_edge_program );
        surf->populateEdgeBuffer( tess );
    }
    else {
        surf->clearEdgeBuffer();
    }
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glUseProgram( 0 );
    glBindVertexArray( 0 );
    CHECK_GL;
}
