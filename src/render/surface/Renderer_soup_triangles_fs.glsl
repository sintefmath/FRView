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

uniform bool  solid_pass;

in FRAGMENT_IN {
    flat vec4 color;
         vec3 normal;
         vec3 position;
} fragment_in;

vec4 colorize()
{
    vec3 v = normalize( vec3( 0.f, 0.f, 1.5f ) - fragment_in.position ); // towards eye
    vec3 l = normalize( vec3( 0.f, 1.5f, 1.5f ) - fragment_in.position );  // towards light
    vec3 h = normalize( v + l );                  // half-vector
    vec3 n = normalize( fragment_in.normal );
    float d = max( 0.3f, dot(n,l) );
    float s = pow( max( 0.f, dot(n,h) ), 50.f );

    vec4 color = fragment_in.color;
    color.rgb = clamp(
#ifdef SHADING_DIFFUSE_COMPONENT
                       d*color.rgb
#else
                       color.rgb
#endif
#ifdef SHADING_SPECULAR_COMPONENT
                       + vec3(color.a*s)
#endif
                , vec3(0.f), vec3(1.f) );

    return color;
}



