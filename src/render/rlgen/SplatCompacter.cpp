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

#include "utils/Logger.hpp"
#include "utils/GLSLTools.hpp"
#include "render/mesh/CellSetInterface.hpp"
#include "render/mesh/VertexPositionInterface.hpp"
#include "render/subset/Representation.hpp"
#include "render/rlgen/Splats.hpp"
#include "render/rlgen/SplatCompacter.hpp"
namespace {
const std::string package = "render.rlgen.SplatCompacter";
}

namespace render {
namespace rlgen {
namespace glsl {
    extern const std::string SplatCompacter_vs;
    extern const std::string SplatCompacter_gs;
}

SplatCompacter::SplatCompacter()
    : m_compacter( package + ".m_compacter" )
{
    m_compacter.addShaderStage( glsl::SplatCompacter_vs, GL_VERTEX_SHADER );
    m_compacter.addShaderStage( glsl::SplatCompacter_gs, GL_GEOMETRY_SHADER );
    
    const char* compact_feedback[3] = {
        "bbmin",
        "bbmax",
        "cellid"
    };
    glTransformFeedbackVaryings( m_compacter.get(),
                                 3, compact_feedback,
                                 GL_INTERLEAVED_ATTRIBS );

    m_compacter.link();
    m_local_to_world_loc = m_compacter.uniformLocation( "local_to_world" );
}

SplatCompacter::~SplatCompacter()
{
}

void
SplatCompacter::process( boost::shared_ptr<Splats>                        splats,
                         boost::shared_ptr<const mesh::VertexPositionInterface> vertices,
                         boost::shared_ptr<const mesh::CellSetInterface>  cells,
                         boost::shared_ptr<const subset::Representation>  subset,
                         const GLfloat*                                   world_from_local )
{
    if( !splats || !vertices || !cells || !subset ) {
        Logger log = getLogger( package + ".process" );
        LOGGER_FATAL( log, "Passed null pointers." );
        return;
    }
    
    splats->resize( cells->cellCount() );
    
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_BUFFER, subset->subsetAsTexture() );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_BUFFER, vertices->vertexPositionsAsBufferTexture() );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_BUFFER, cells->cellCornerTexture() );

    glUseProgram( m_compacter.get() );
    glUniformMatrix4fv( m_local_to_world_loc, 1, GL_FALSE, world_from_local );


    glEnable( GL_RASTERIZER_DISCARD );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, splats->buffer().get() );
    glBeginQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, splats->query().get() );
    glBeginTransformFeedback( GL_POINTS );

    glDrawArrays( GL_POINTS, 0, cells->cellCount() );
 
    glEndTransformFeedback();
    glEndQuery( GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN );
    glBindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0 );
    glDisable( GL_RASTERIZER_DISCARD );
    
}
    


} // of namespace rlgen
} // of namespace render
