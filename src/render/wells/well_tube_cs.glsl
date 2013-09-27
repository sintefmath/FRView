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
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */


layout(vertices=2) out;

in VC {
    vec3 position;
    vec3 tangent;
    vec3 normal;
    vec3 color;
} prev[];

out CE {
    vec3 position;
    vec3 tangent;
    vec3 normal;
    vec3 binormal;
} next[];

patch out vec3 color;

void
main()
{
    next[ gl_InvocationID ].position = prev[ gl_InvocationID ].position;
    next[ gl_InvocationID ].tangent  = prev[ gl_InvocationID ].tangent;
    next[ gl_InvocationID ].normal   = normalize( prev[ gl_InvocationID ].normal );
    next[ gl_InvocationID ].binormal = normalize( cross( prev[ gl_InvocationID ].tangent,
                                                         prev[ gl_InvocationID ].normal ) );

    if( gl_InvocationID == 0 ) {
        color = prev[0].color;


        gl_TessLevelInner[ 0 ] = 40;
        gl_TessLevelInner[ 1 ] = 15;
        gl_TessLevelOuter[ 0 ] = 15;
        gl_TessLevelOuter[ 1 ] = 40;
        gl_TessLevelOuter[ 2 ] = 15;
        gl_TessLevelOuter[ 3 ] = 40;
    }

}
