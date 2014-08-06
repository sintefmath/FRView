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

#include "render/RenderItem.hpp"
#include "render/surface/TriangleSoupRenderer.hpp"

namespace render {
namespace surface {

TriangleSoupRenderer::TriangleSoupRenderer( const std::string& defines,
                                            const std::string& fragment_source )
{

}

void
TriangleSoupRenderer::draw( const GLfloat*                    modelview,
                            const GLfloat*                    projection,
                            const GLsizei                     width,
                            const GLsizei                     height,
                            const std::vector<RenderItem>&    render_items )
{

}


} // namespace surfacae
} // namespace render
