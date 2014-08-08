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

layout(location=0)  in vec3 normal;
layout(location=1)  in uint cell;
layout(location=2)  in vec4 position;

out FRAGMENT_IN {
    flat vec4 color;
         vec3 normal;
         vec3 position;
} fragment_in;

uniform mat4    MVP;
uniform mat4    MV;
uniform mat3    NM;
uniform vec4    surface_color;

void
main()
{
    vec4 p = MV * vec4(position.xyz, 1.f);

    fragment_in.color    = vec4( surface_color.w*surface_color.rgb, surface_color.w );
    fragment_in.normal   = NM * normal;
    fragment_in.position = (1.f/p.w)*p.xyz;
    gl_Position = MVP*vec4( position.xyz, 1.f );
}
