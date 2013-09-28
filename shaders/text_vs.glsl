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

layout(location=0) in vec2  spos;
layout(location=1) in vec2  tpos;
layout(location=2) in vec3  position;


uniform mat4                mvp;
//uniform vec3                position;
uniform vec2                size;
uniform vec2                scale;

out FS {
    vec2                    tpos;
}                           out_fs;

void main(void)
{
    vec4 h = mvp * vec4( position, 1.f );
    vec3 p = (1.f/h.w)*h.xyz;
    vec2 sp = vec2( ivec2(size*p.xy) + vec2( 0.5f ));

    out_fs.tpos = tpos;
    gl_Position = vec4( scale*( sp.xy + spos),
                        p.z,
                        1.f);

}
