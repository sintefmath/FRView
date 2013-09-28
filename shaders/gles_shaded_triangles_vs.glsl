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

#ifdef GL_ES
precision highp float;
#endif
uniform mat4 MVP;
uniform mat3 NM;
attribute vec4 position;
varying vec3 normal;
varying vec3 color;
void
main()
{
    vec3 n = floor( position.xyz )-vec3( 2.0 );
    vec3 p = fract( position.xyz );
    normal = NM * n;
    float scalar = position.w;
    color = max( vec3( 0.0 ),
                     sin( vec3( 4.14*(scalar - 0.5),
                                3.14*(scalar),
                                4.14*(scalar + 0.5) ) ) );
    gl_Position = MVP * vec4( p, 1.0 );
}
