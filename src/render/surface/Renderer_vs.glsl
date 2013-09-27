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
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */


layout(location=0) in vec4 position;

out GI {
    vec3    obj_pos;
    vec2    scr_pos;
} in_g;

invariant gl_Position;

uniform mat4            MVP;
uniform mat4            MV;
uniform mat3            NM;
uniform vec2            screen_size;

void main(void)
{
    vec4 obj_pos = MV*vec4(position.xyz,1.f);

    gl_Position = MVP*vec4(position.xyz,1.f);
    in_g.obj_pos = (1.f/obj_pos.w)*obj_pos.xyz;
    in_g.scr_pos = screen_size*( (0.5f/gl_Position.w)*gl_Position.xy + vec2(0.5f) );
}
