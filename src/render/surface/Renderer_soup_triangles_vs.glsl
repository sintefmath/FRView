#version 420
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

layout(binding=2)   uniform samplerBuffer   field;
layout(binding=3)   uniform sampler1D       color_map;

uniform mat4    MVP;
uniform mat4    MV;
uniform mat3    NM;
uniform vec4    surface_color;
uniform bool    use_field;
uniform bool    log_map;
uniform vec2    field_remap;

void
main()
{
    vec4 p = MV * vec4(position.xyz, 1.f);


    vec3 color;
    if( use_field ) {
        // colorize using a field
        uint cid = cell & 0x0fffffffu;
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
        color = texture( color_map, value ).rgb;
    }
    else {
        color = surface_color.rgb;
    }
/*
    color = vec3( ((cell & 0x1u) == 0) ? 1.f : 0.f,
                  ((cell & 0x2u) == 0) ? 1.f : 0.f,
                  ((cell & 0x4u) == 0) ? 1.f : 0.f );
*/
    fragment_in.color    = vec4( surface_color.w*color, surface_color.w );
    fragment_in.normal   = NM * normal;
    fragment_in.position = (1.f/p.w)*p.xyz;
    gl_Position = MVP*vec4( position.xyz, 1.f );
}
