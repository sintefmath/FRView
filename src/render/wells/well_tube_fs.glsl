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


in EF {
    vec2 param;
    vec3 normal;
    vec3 color;
} prev;

layout(location=0)  out vec3    frag_color;

void
main()
{
    vec3 v = normalize( vec3( 0.f, 0.f, 1.5f )  ); // towards eye
    vec3 l = normalize( vec3( 1.5f, 1.5f, 1.5f )  );  // towards light
    vec3 h = normalize( v + l );                  // half-vector
    vec3 n = normalize( prev.normal );
    float d = max( 0.5f, dot(n,l) );
    float s = pow( max( 0.f, dot(n,h) ), 50.f );


//    frag_color = d*vec3( prev.param.x, prev.param.y, 0.8f) + vec3(s);
frag_color = d*vec3( prev.color ) + vec3(s);
//frag_color = vec3(1.0, 0.0, 0.0);

//    frag_color = vec3( prev.param.x, prev.param.y, 0.8f);

}
