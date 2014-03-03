#version 420
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
 * along with the FRView.  If not, see http://www.gnu.org/licenses/.
 */

layout(location=0) in vec4 position;
layout(std140, binding=0) uniform FragmenListBlock {
    int     fragment_count;
    int     fragment_alloc;
};

out vec2 normalized;

void
main()
{
    if( fragment_count <= fragment_alloc ) {
        normalized = 0.5f*(position.xy + vec2(1.f) );
        gl_Position = position;
    }
    else {
        normalized = vec2(0.f);
        gl_Position = vec4(0.f);
    }
}
