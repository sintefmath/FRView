#include <siut/gl_utils/GLSLtools.hpp>
#include <siut/io_utils/snarf.hpp>
#include "Logger.hpp"
#include "GridTess.hpp"
#include "CellSelector.hpp"

HalfPlaneSelector::HalfPlaneSelector()
{
    m_prog = glCreateProgram();
    m_vs = siut::gl_utils::compileShader(
                siut::io_utils::snarfFile( "shaders/halfplane_select_vs.glsl" ),
                GL_VERTEX_SHADER,
                true );
    glAttachShader( m_prog, m_vs );
    const char* varyings[1] = {
        "selected"
    };
    glTransformFeedbackVaryings( m_prog, 1, varyings, GL_INTERLEAVED_ATTRIBS );
    siut::gl_utils::linkProgram( m_prog );

    m_loc_halfplane_eq = glGetUniformLocation( m_prog, "plane_equation" );


    GLint loc;
    glUseProgram( m_prog );
    loc = glGetUniformLocation( m_prog, "vertices" );
    glUniform1i( loc, 0 );
    loc = glGetUniformLocation( m_prog, "cell_corner" );
    glUniform1i( loc, 1 );

    glUseProgram( 0 );
    CHECK_GL;

    glGenQueries( 1, &m_query );
}

unsigned int
HalfPlaneSelector::select( GridTess& tess, const float* equation )
{
    glUseProgram( m_prog );
    glUniform4fv( m_loc_halfplane_eq, 1, equation );


    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, tess.vertexTexture() );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, tess.cellCornerTexture() );

    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, tess.cellSubsetBuffer() );

    glEnable( GL_RASTERIZER_DISCARD );
    glBeginQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_query );

    glBeginTransformFeedback( GL_POINTS );

    glDrawArrays( GL_POINTS, 0, (tess.cellCount()+31)/32 );

    glEndTransformFeedback( );
    glEndQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN );

    glDisable( GL_RASTERIZER_DISCARD );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0 );

    glUseProgram( 0 );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, 0 );
    CHECK_GL;

    GLuint result;
    glGetQueryObjectuiv( m_query,
                         GL_QUERY_RESULT,
                         &result );
    return result;
}
