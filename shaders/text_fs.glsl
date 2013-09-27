#version 330
#extension GL_ARB_shading_language_420pack : enable
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

layout(location=0)  out vec4                frag_color;
layout(binding=0)   uniform sampler2D       font;

in FS {
    vec2                    tpos;
}                           in_fs;

void main(void)
{
    float i = texelFetch( font, ivec2( in_fs.tpos ), 0 ).r;
    if( i == 0.f ) {
        discard;
    }
    frag_color = vec4( i );
}
