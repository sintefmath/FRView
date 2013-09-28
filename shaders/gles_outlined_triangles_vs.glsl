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

uniform mat4 MVP;
attribute vec3 position;
varying vec3 barycentric;
void
main()
{
    vec3 p = position;
    int i = int( floor(p.x) );
    p.x = fract( p.x );

    barycentric = vec3( i == 0, i==1, i==2 );
    gl_Position = MVP * vec4( p, 1.0 );
}
