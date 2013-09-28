#version 330
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

uniform mat4 mvp;

out FI {
    vec2    obj_pos;
} out_v;

uniform mat4 unit_to_object;
uniform vec3 scale;

void main(void)
{
    vec4 oph = unit_to_object * vec4( position.xyz, 1.f );
    vec3 op = scale*(1.f/oph.w)*oph.xyz;
    if( position.w == 0.f ) {
        out_v.obj_pos = op.xy;
    }
    else if( position.w == 1.f ) {
        out_v.obj_pos = op.yz;
    }
    else {
        out_v.obj_pos = op.xz;
    }
//    out_v.obj_pos = (1.0/oph.w)*oph.xyz;
    gl_Position = mvp*vec4( position.xyz, 1.f );
}

