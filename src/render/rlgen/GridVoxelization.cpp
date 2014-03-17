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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "utils/GLSLTools.hpp"
#include "utils/Logger.hpp"
#include "render/mesh/CellSetInterface.hpp"
#include "render/mesh/VertexPositionInterface.hpp"
#include "render/subset/Representation.hpp"
#include "render/rlgen/GridVoxelization.hpp"

namespace {
    const std::string package = "render.GridVoxelization";
}

namespace render {
    namespace rlgen {
        namespace glsl {
            extern const std::string GridVoxelization_compact_vs;
            extern const std::string GridVoxelization_compact_gs;
            extern const std::string GridVoxelization_splat_vs;
            extern const std::string GridVoxelization_splat_fs;
            extern const std::string GridVoxelization_splat_gs;
        }


GridVoxelization::GridVoxelization()
    : m_resolution(32)
{
    Logger log = getLogger( package + ".constructor" );
    
    glGenTextures( 1, &m_voxels_tex );
    glBindTexture( GL_TEXTURE_3D, m_voxels_tex );
    glTexImage3D( GL_TEXTURE_3D, 0, GL_R32UI,
                  m_resolution, m_resolution, m_resolution, 0,
                  GL_RED_INTEGER , GL_INT, NULL );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glBindTexture( GL_TEXTURE_3D, 0 );

    m_compacted_alloc = 1000;
    glGenBuffers( 1, &m_compacted_buf );
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_compacted_buf );
    glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                  (sizeof(GLfloat)*6+sizeof(GLuint))*m_compacted_alloc,
                  NULL,
                  GL_STREAM_DRAW );
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );

    glGenVertexArrays( 1, &m_compacted_vao );
    glBindVertexArray( m_compacted_vao );
    glBindBuffer( GL_ARRAY_BUFFER, m_compacted_buf );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6+sizeof(GLuint), NULL );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6+sizeof(GLuint), (GLvoid*)(sizeof(GLfloat)*3) );
    glEnableVertexAttribArray( 1 );
    glVertexAttribIPointer( 2, 1, GL_UNSIGNED_INT, sizeof(GLfloat)*6+sizeof(GLuint), (GLvoid*)(sizeof(GLfloat)*6) );
    glEnableVertexAttribArray( 2 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );


    glGenQueries( 1, &m_compacted_count_query );

    glGenFramebuffers( 1, &m_slice_fbo );

    m_voxelizer_program = glCreateProgram();

    GLuint vs = utils::compileShader( log, glsl::GridVoxelization_splat_vs, GL_VERTEX_SHADER );
    glAttachShader( m_voxelizer_program, vs );
    glDeleteShader( vs );

    GLuint gs = utils::compileShader( log, glsl::GridVoxelization_splat_gs, GL_GEOMETRY_SHADER );
    glAttachShader( m_voxelizer_program, gs );
    glDeleteShader( gs );

    GLuint fs = utils::compileShader( log, glsl::GridVoxelization_splat_fs, GL_FRAGMENT_SHADER );
    glAttachShader( m_voxelizer_program, fs );
    glDeleteShader( fs );

    utils::linkProgram( log, m_voxelizer_program );

    m_voxelizer_slice_loc = glGetUniformLocation( m_voxelizer_program, "slice" );

    GLuint sh;

    const char* compact_feedback[3] = {
        "bbmin",
        "bbmax",
        "cellid"
    };

    m_compact_program = glCreateProgram();
    sh = utils::compileShader( log, glsl::GridVoxelization_compact_vs, GL_VERTEX_SHADER );
    glAttachShader( m_compact_program, sh );
    glDeleteShader( sh );
    sh = utils::compileShader( log, glsl::GridVoxelization_compact_gs, GL_GEOMETRY_SHADER );
    glAttachShader( m_compact_program, sh );
    glDeleteShader( sh );

    glTransformFeedbackVaryings( m_compact_program,
                                 3, compact_feedback,
                                 GL_INTERLEAVED_ATTRIBS );
    utils::linkProgram( log, m_compact_program );
    m_compact_local_to_world_loc = glGetUniformLocation( m_compact_program, "local_to_world" );
}

GridVoxelization::~GridVoxelization()
{
    glDeleteTextures( 1, &m_voxels_tex );
    glDeleteFramebuffers( 1, &m_slice_fbo );
}


void
GridVoxelization::dimension( GLsizei* dim ) const
{
    dim[0] = m_resolution;
    dim[1] = m_resolution;
    dim[2] = m_resolution;
}

void
GridVoxelization::build( boost::shared_ptr<const mesh::CellSetInterface> cell_set,
                         boost::shared_ptr<const subset::Representation> cell_subset,
                         const GLfloat*                                  world_from_local )
{
    boost::shared_ptr<const mesh::VertexPositionInterface> vertices =
            boost::dynamic_pointer_cast<const mesh::VertexPositionInterface>( cell_set );
    if( !vertices ) {
        Logger log = getLogger( package + ".apply" );
        LOGGER_ERROR( log, "cell set does not implement VertexPositionInterface." );
        return;
    }

    Logger log = getLogger( package + ".build" );

    // transform to unit cube and discard unselected cells
    if( m_compacted_alloc < cell_set->cellCount() ) {
        m_compacted_alloc = 1.01*cell_set->cellCount();
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_compacted_buf );
        glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                      (sizeof(GLfloat)*6+sizeof(GLuint))*m_compacted_alloc,
                      NULL,
                      GL_STREAM_DRAW );
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );
    }

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, cell_subset->subsetAsTexture() );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, vertices->vertexPositionsAsBufferTexture() );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_BUFFER, cell_set->cellCornerTexture() );

    glUseProgram( m_compact_program );
    glUniformMatrix4fv( m_compact_local_to_world_loc, 1, GL_FALSE, world_from_local );

    glEnable( GL_RASTERIZER_DISCARD );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_compacted_buf );
    glBeginQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_compacted_count_query );
    glBeginTransformFeedback( GL_POINTS );
    glDrawArrays( GL_POINTS, 0, cell_set->cellCount() );
    glEndTransformFeedback();
    glEndQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN );
    glDisable( GL_RASTERIZER_DISCARD );

    GLuint compacted_count;
    glGetQueryObjectuiv( m_compacted_count_query,
                         GL_QUERY_RESULT,
                         &compacted_count );

    // populate slices
    glBindVertexArray( m_compacted_vao );
    glBindFramebuffer( GL_FRAMEBUFFER, m_slice_fbo );

    // make sure that the outer rim of voxels are always empty
    glViewport( 1, 1, m_resolution-2, m_resolution-2 );
    glUseProgram( m_voxelizer_program );

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );
    glClearColor( 0.f, 0.f, 0.f, 0.f );
    for(GLsizei i=0; i<m_resolution; i++ ) {
        glFramebufferTexture3D( GL_FRAMEBUFFER,
                                GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_3D,
                                m_voxels_tex,
                                0,
                                i );
        utils::checkFBO( log );
        glClear( GL_COLOR_BUFFER_BIT );
        if( (0 < i) && (i+1<m_resolution) ) {
            glUniform2f( m_voxelizer_slice_loc, (i-1.f)/(m_resolution-2.f), (i+0.f)/(m_resolution-2.f) );
            glDrawArrays( GL_POINTS, 0, compacted_count );
        }
    }
    glBindVertexArray( 0 );
}

    } // of namespace rlgen
} // of namespace render
