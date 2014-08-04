#version 330
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
 * along with the FRView.  If not, see http://www.gnu.org/licenses/.
 */

layout(location=0)  in vec4 position;
layout(location=1)  in vec3 normal;
layout(location=2)  in vec3 color;

out FRAGMENT_IN {
    vec3 color;
    vec3 normal;
} fragment_in;

uniform mat4            MVP;
uniform mat4            MV;
uniform mat3            NM;

void
main()
{
    fragment_in.color  = color;
    fragment_in.normal = NM * normal;
    gl_Position = MVP*vec4( position.xyz, 1.f );
    
    
}
