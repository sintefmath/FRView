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
    flat vec3 col;
} go;

layout(binding=0)   uniform samplerBuffer   field;
layout(binding=1)   uniform sampler1D       color_map;
                    uniform float           slice;
                    uniform vec2            field_remap;
                    uniform bool            use_field;
                    uniform bool            log_map;
                    uniform vec3            surface_color;

void
main()
{
    if( (gi[0].bbmin.z <= slice ) && (slice <= gi[0].bbmax.z) ) {

        if( use_field ) {
            // colorize using a field
            uint cid = gi[0].cell & 0x0fffffffu;
            float value = texelFetch( field, int(cid) ).r;
            if( log_map ) {
                // field_remap.x = 1.0/min_value
                // field_remap.y = 1.0/log(max_value/min_value)
                value = log( value*field_remap.x )*field_remap.y;
            }
            else {
                // field_remap.x = min_value
                // field_remap.y = 1.0/(max_value-min_value)
                value = field_remap.y*( value - field_remap.x);
            }
            go.col = texture( color_map, value ).rgb;
        }
        else {
            go.col = surface_color;
        }
        
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
