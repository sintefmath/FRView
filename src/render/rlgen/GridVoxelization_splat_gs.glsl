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


layout(points, invocations=1) in;
layout(triangle_strip, max_vertices=4) out;
in GI {
    vec3 bbmin;
    vec3 bbmax;
    uint cell;
} gi[];

out FI {
    flat uint cell;
} go;

uniform vec2            slice;

void
main()
{
    if( (slice.x <= gi[0].bbmax.z ) && (gi[0].bbmin.z <= slice.y)  ) {
        go.cell = gi[0].cell;
        gl_Position = vec4( gi[0].bbmin.x, gi[0].bbmin.y, 0.f, 1.f );
        EmitVertex();
        gl_Position = vec4( gi[0].bbmax.x, gi[0].bbmin.y, 0.f, 1.f );
        EmitVertex();
        gl_Position = vec4( gi[0].bbmin.x, gi[0].bbmax.y, 0.f, 1.f );
        EmitVertex();
        gl_Position = vec4( gi[0].bbmax.x, gi[0].bbmax.y, 0.f, 1.f );
        EmitVertex();
    }
}
