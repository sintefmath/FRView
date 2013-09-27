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

#include <iostream>
#include "utils/GLSLTools.hpp"
#include "utils/Logger.hpp"
#include "GridVoxelization.hpp"
#include "render/GridField.hpp"
#include "render/rlgen/VoxelSurface.hpp"


namespace render {
    namespace rlgen {

        namespace glsl {
            extern const std::string VoxelSurface_hpmc_vs;
            extern const std::string VoxelSurface_hpmc_fetch;
        } // of namespace glsl

VoxelSurface::VoxelSurface()
{
    m_volume_dim[0] = ~0u;
    m_volume_dim[1] = ~0u;
    m_volume_dim[2] = ~0u;
    m_hpmc_c = HPMCcreateConstants( 4, 4 );
    m_hpmc_hp = NULL;
    m_hpmc_th = NULL;

    glGenBuffers( 1, &m_surface_buf );
    m_surface_alloc = 5000;
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_surface_buf );
    glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                  sizeof(GLfloat)*4*m_surface_alloc,
                  NULL,
                  GL_STREAM_READ );
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );
}

VoxelSurface::~VoxelSurface()
{
    if( m_hpmc_th != NULL ) {
        HPMCdestroyTraversalHandle( m_hpmc_th );
    }
    if( m_hpmc_hp != NULL ) {
        //HPMCdestroyHandle( m_hpmc_hp );
    }
    HPMCdestroyConstants( m_hpmc_c );
}

void
VoxelSurface::build( boost::shared_ptr<const GridVoxelization> voxels,
                     boost::shared_ptr<const GridField> field )
{
    
    Logger log = getLogger( "VoxelSurface.build" );
    { 
        GLenum gl_error = glGetError();
        if( gl_error != GL_NONE ) {
            do {
                LOGGER_WARN( log, "Entered with GL error " << glewGetErrorString( gl_error ) );
                gl_error = glGetError();
            } while( gl_error != GL_NONE );
        }
    }
    
    
    GLsizei voxel_dim[3];
    voxels->dimension( voxel_dim );


    if( (voxel_dim[0] != m_volume_dim[0]) ||
        (voxel_dim[1] != m_volume_dim[1]) ||
        (voxel_dim[2] != m_volume_dim[2]) )
    {
        // release old instances
        if( m_hpmc_th != NULL ) {
            HPMCdestroyTraversalHandle( m_hpmc_th );
        }
        if( m_hpmc_hp != NULL ) {
//            HPMCdestroyHandle();
        }

        m_volume_dim[0] = voxel_dim[0];
        m_volume_dim[1] = voxel_dim[1];
        m_volume_dim[2] = voxel_dim[2];


        // set up histopyramid
        m_hpmc_hp = HPMCcreateHistoPyramid( m_hpmc_c );
        HPMCsetLatticeSize( m_hpmc_hp,
                            voxel_dim[0],
                            voxel_dim[1],
                            voxel_dim[2] );
        HPMCsetGridSize( m_hpmc_hp,
                         voxel_dim[0]-1,
                         voxel_dim[1]-1,
                         voxel_dim[2]-1 );
        HPMCsetGridExtent( m_hpmc_hp, 1.f, 1.f, 1.f );
        HPMCsetFieldCustom( m_hpmc_hp,
                            glsl::VoxelSurface_hpmc_fetch.c_str(),
                            0,
                            GL_FALSE );

        HPMCsetFieldAsBinary( m_hpmc_hp );

        GLuint builder = HPMCgetBuilderProgram( m_hpmc_hp );
        if( builder == 0 ) {
            LOGGER_ERROR( log, "HPMC provided builder program = 0." );
        }
        
        { 
            GLenum gl_error = glGetError();
            if( gl_error != GL_NONE ) {
                do {
                    LOGGER_WARN( log, "A Entered with GL error " << glewGetErrorString( gl_error ) );
                    gl_error = glGetError();
                } while( gl_error != GL_NONE );
            }
        }

        glUseProgram( builder );
        glUniform1i( glGetUniformLocation( builder, "voxels" ), 1 );

        { 
            GLenum gl_error = glGetError();
            if( gl_error != GL_NONE ) {
                do {
                    LOGGER_WARN( log, "B Entered with GL error " << glewGetErrorString( gl_error ) );
                    gl_error = glGetError();
                } while( gl_error != GL_NONE );
            }
        }


        

        // set up extraction pass
        m_hpmc_th = HPMCcreateTraversalHandle( m_hpmc_hp );
        m_extraction_program = glCreateProgram();

        char* traversal_code = HPMCgetTraversalShaderFunctions( m_hpmc_th );
        if( traversal_code == NULL ) {
            LOGGER_FATAL( log, "Failed to get HPMC traversal shader functions." );
            return;
        }
        
        GLuint vs = utils::compileShader( log,
                                          glsl::VoxelSurface_hpmc_vs +
                                          traversal_code,
                                          GL_VERTEX_SHADER );
        free( traversal_code );
        glAttachShader( m_extraction_program, vs );
        glDeleteShader( vs );
        const char* varyings[1] = {
            "position"
        };
        glTransformFeedbackVaryings( m_extraction_program, 1, varyings, GL_INTERLEAVED_ATTRIBS );
        utils::linkProgram( log, m_extraction_program );
        glUseProgram( m_extraction_program );
        m_extraction_loc_scale = glGetUniformLocation( m_extraction_program, "scale" );
        m_extraction_loc_shift = glGetUniformLocation( m_extraction_program, "shift" );
        glUniform1i( glGetUniformLocation( m_extraction_program, "voxels"), 3 );
        glUniform1i( glGetUniformLocation( m_extraction_program, "field"), 4 );
        if(!HPMCsetTraversalHandleProgram( m_hpmc_th, m_extraction_program, 0, 1, 2 )) {
            LOGGER_ERROR( log, "Failed to set HPMC extraction program"  );
        }
    }

    // Bind voxel texture to sampler 1
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_3D, voxels->voxelTexture() );
    HPMCbuildHistopyramid( m_hpmc_hp, 0.5f );

    GLsizei N = HPMCacquireNumberOfVertices( m_hpmc_hp );
    //LOGGER_DEBUG( log, "HPMC reported " << N << " vertices in iso-surface" );
    if( N == 0 ) {
        m_surface_host.clear();
        return;
    }
    else if( m_surface_alloc < N ) {
        m_surface_alloc = 1.1f*N;
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_surface_buf );
        glBufferData( GL_TRANSFORM_FEEDBACK_BUFFER,
                      sizeof(GLfloat)*4*m_surface_alloc,
                      NULL,
                      GL_STREAM_READ );
        glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );
    }


    HPMCbuildHistopyramid( m_hpmc_hp, 0.5f );
    glUseProgram( m_extraction_program );


    glActiveTexture( GL_TEXTURE3 );
    glBindTexture( GL_TEXTURE_3D, voxels->voxelTexture() );
    glActiveTexture( GL_TEXTURE4 );
    if( field->hasData() ) {
        glUniform1i( glGetUniformLocation( m_extraction_program, "use_field"), GL_TRUE );
        glUniform2f( glGetUniformLocation( m_extraction_program, "field_remap"),
                     field->minValue(),
                     1.f/(field->maxValue()-field->minValue()) );
        glBindTexture( GL_TEXTURE_BUFFER, field->texture() );
    }
    else {
        glUniform1i( glGetUniformLocation( m_extraction_program, "use_field"), GL_FALSE );
        glBindTexture( GL_TEXTURE_BUFFER, 0 );

    }

    // The unit cube is scaled to the interior of voxels (to make sure that the
    // geometry is capped). We need to scale the extracted geometry such that
    // the interior of voxels are scaled out to the unit cube.
    GLfloat scale[3] = { voxel_dim[0]/(voxel_dim[0]-2.f),
                         voxel_dim[1]/(voxel_dim[1]-2.f),
                         voxel_dim[2]/(voxel_dim[2]-2.f) };
    GLfloat shift[3] = { -1.f/voxel_dim[0],
                         -1.f/voxel_dim[1],
                         -1.f/voxel_dim[2] };
    glUniform3fv( m_extraction_loc_scale, 1, scale );
    glUniform3fv( m_extraction_loc_shift, 1, shift );

    glEnable( GL_RASTERIZER_DISCARD );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_surface_buf );
    HPMCextractVerticesTransformFeedback( m_hpmc_th );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0 );
    glDisable( GL_RASTERIZER_DISCARD );

    m_surface_host.resize( 4*N );
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, m_surface_buf );
    glGetBufferSubData( GL_TRANSFORM_FEEDBACK_BUFFER,
                        0,
                        sizeof(GLfloat)*4*N,
                        m_surface_host.data() );
    glBindBuffer( GL_TRANSFORM_FEEDBACK_BUFFER, 0 );

//    for(size_t i=0; i<m_surface_host.size(); i++ ) {
//        std::cerr << m_surface_host[i] << "\t";
//    }


}

    } // of namespace rlgen
} // of namespace render
