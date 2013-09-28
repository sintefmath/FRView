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


layout( quads ) in;

in CE {
    vec3 position;
    vec3 tangent;
    vec3 normal;
    vec3 binormal;
} prev[];

patch in vec3 color;

out EF {
    vec2 param;
    vec3 normal;
    vec3 color;
} next;

#define M_2PI 6.28318530717958647692528676655900576

uniform mat4    projection;
uniform mat4    modelview;

void
main()
{


    float u = M_2PI*gl_TessCoord.x;
    float v = gl_TessCoord.y;


    vec3 a = normalize( mix( prev[0].normal, prev[1].normal, v ) );
    vec3 b = normalize( mix( prev[0].binormal, prev[1].binormal, v ) );
    vec3 d = cos( u )*a + sin( u )*b;

//    vec3 c = mix( prev[0].position, prev[1].position, v );


    vec3 c = ( 2.f*v*v*v - 3.f*v*v + 1.f )*prev[0].position
           + (     v*v*v - 2.f*v*v + v   )*(prev[0].tangent )
           + (-2.f*v*v*v + 3.f*v*v       )*prev[1].position
           + (     v*v*v -     v*v       )*(prev[1].tangent);



    next.normal = (modelview*vec4(d,0.f)).xyz;
    next.param = gl_TessCoord.xy;
    next.color = color;

    gl_Position = projection*(modelview*vec4(c + 0.003*d,1.f));

}
